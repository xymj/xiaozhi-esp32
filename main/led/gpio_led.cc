#include "gpio_led.h"
#include "application.h"
#include "device_state.h"
#include <esp_log.h>

#define TAG "GpioLed"

// LED亮度级别定义
#define DEFAULT_BRIGHTNESS 50 ///< 默认亮度值(50%)
#define HIGH_BRIGHTNESS 100   ///< 高亮度值(100%)
#define LOW_BRIGHTNESS 10     ///< 低亮度值(10%)

#define IDLE_BRIGHTNESS 5        ///< 空闲状态亮度值(5%)
#define SPEAKING_BRIGHTNESS 75   ///< 说话状态亮度值(75%)
#define UPGRADING_BRIGHTNESS 25  ///< 升级状态亮度值(25%)
#define ACTIVATING_BRIGHTNESS 35 ///< 激活状态亮度值(35%)

// 无限闪烁标识
#define BLINK_INFINITE -1 ///< 无限闪烁标识，用于表示持续闪烁

// LEDC配置参数
#define LEDC_LS_TIMER LEDC_TIMER_1         ///< LEDC定时器编号
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE   ///< LEDC低速模式
#define LEDC_LS_CH0_CHANNEL LEDC_CHANNEL_0 ///< LEDC通道0
#define LEDC_DUTY (8191)                   ///< 最大占空比值(13位分辨率的最大值)
#define LEDC_FADE_TIME (1000)              ///< 淡入淡出时间(毫秒)

/**
 * @brief 构造函数，使用默认参数初始化GPIO LED控制器
 *
 * 委托构造函数，使用默认的LEDC定时器和通道配置初始化指定GPIO引脚上的LED控制器。
 *
 * @param gpio 控制LED的GPIO引脚号
 */
GpioLed::GpioLed(gpio_num_t gpio)
    : GpioLed(gpio, 0, LEDC_LS_TIMER, LEDC_LS_CH0_CHANNEL)
{
}

/**
 * @brief 构造函数，指定GPIO和输出反转设置
 *
 * 委托构造函数，初始化指定GPIO引脚上的LED控制器，并设置输出是否反转。
 *
 * @param gpio 控制LED的GPIO引脚号
 * @param output_invert 输出反转标志，1表示反转，0表示不反转
 */
GpioLed::GpioLed(gpio_num_t gpio, int output_invert)
    : GpioLed(gpio, output_invert, LEDC_LS_TIMER, LEDC_LS_CH0_CHANNEL)
{
}

/**
 * @brief 构造函数，完全自定义参数初始化GPIO LED控制器
 *
 * 使用指定的GPIO引脚、输出反转设置、LEDC定时器和通道来初始化LED控制器。
 * 配置包括LEDC定时器、通道、PWM频率、占空比分辨率等参数。
 *
 * @param gpio 控制LED的GPIO引脚号
 * @param output_invert 输出反转标志，1表示反转，0表示不反转
 * @param timer_num LEDC定时器编号
 * @param channel LEDC通道编号
 */
GpioLed::GpioLed(gpio_num_t gpio, int output_invert, ledc_timer_t timer_num, ledc_channel_t channel)
{
    // 如果GPIO未连接，应该使用NoLed类而不是这个类
    assert(gpio != GPIO_NUM_NC);

    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    // 配置LEDC定时器参数
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // PWM占空比分辨率为13位
    ledc_timer.freq_hz = 4000;                      // PWM信号频率为4kHz
    ledc_timer.speed_mode = LEDC_LS_MODE;           // 使用低速模式
    ledc_timer.timer_num = timer_num;               // 定时器编号
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;             // 自动选择时钟源

    // 应用LEDC定时器配置并检查错误
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 配置LEDC通道参数
    ledc_channel_.channel = channel,                              // 通道编号
        ledc_channel_.duty = 0,                                   // 初始占空比为0(关闭)
        ledc_channel_.gpio_num = gpio,                            // GPIO引脚号
        ledc_channel_.speed_mode = LEDC_LS_MODE,                  // 低速模式
        ledc_channel_.hpoint = 0,                                 // Hpoint值为0
        ledc_channel_.timer_sel = timer_num,                      // 关联的定时器编号
        ledc_channel_.flags.output_invert = output_invert & 0x01, // 输出反转标志

        // 应用LEDC通道配置
        ledc_channel_config(&ledc_channel_);

    // 初始化LEDC淡入淡出功能
    ledc_fade_func_install(0);

    // 注册LEDC回调函数，当淡入淡出完成时调用FadeCallback
    ledc_cbs_t ledc_callbacks = {
        .fade_cb = FadeCallback // 淡入淡出完成回调函数
    };
    ledc_cb_register(ledc_channel_.speed_mode, ledc_channel_.channel, &ledc_callbacks, this);

    // 配置LED闪烁定时器参数
    esp_timer_create_args_t blink_timer_args = {
        // 定时器回调函数，使用lambda表达式捕获this指针
        .callback = [](void *arg)
        {
            auto led = static_cast<GpioLed *>(arg);
            led->OnBlinkTimer(); // 调用定时器处理函数
        },
        .arg = this,                       // 回调函数参数，传递this指针
        .dispatch_method = ESP_TIMER_TASK, // 在任务上下文中执行定时器回调
        .name = "Blink Timer",             // 定时器名称
        .skip_unhandled_events = false,    // 不跳过未处理的事件
    };

    // 创建定时器并检查错误
    ESP_ERROR_CHECK(esp_timer_create(&blink_timer_args, &blink_timer_));

    ledc_initialized_ = true; // 标记LEDC已初始化完成
}

/**
 * @brief 析构函数，释放LED控制器资源
 *
 * 停止并删除LED闪烁定时器，卸载LEDC淡入淡出功能。
 */
GpioLed::~GpioLed()
{
    esp_timer_stop(blink_timer_); // 停止闪烁定时器
    if (ledc_initialized_)
    {
        ledc_fade_stop(ledc_channel_.speed_mode, ledc_channel_.channel); // 停止淡入淡出
        ledc_fade_func_uninstall();                                      // 卸载淡入淡出功能
    }
}

/**
 * @brief 设置LED亮度
 *
 * 设置LED的亮度级别，范围0-100。
 * 该设置不会立即生效，需要调用TurnOn()才会应用。
 *
 * @param brightness LED亮度百分比(0-100)
 */
void GpioLed::SetBrightness(uint8_t brightness)
{
    if (brightness == 100)
    {
        duty_ = LEDC_DUTY; // 如果亮度为100%，使用最大占空比
    }
    else
    {
        duty_ = brightness * LEDC_DUTY / 100; // 按比例计算占空比
    }
}

/**
 * @brief 打开LED
 *
 * 将LED设置为当前配置的亮度并点亮。
 * 如果正在闪烁或淡入淡出则停止这些效果。
 */
void GpioLed::TurnOn()
{
    if (!ledc_initialized_)
    {
        return; // 如果LEDC未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);                                          // 停止闪烁定时器
    ledc_fade_stop(ledc_channel_.speed_mode, ledc_channel_.channel);       // 停止淡入淡出
    ledc_set_duty(ledc_channel_.speed_mode, ledc_channel_.channel, duty_); // 设置占空比
    ledc_update_duty(ledc_channel_.speed_mode, ledc_channel_.channel);     // 更新占空比
}

/**
 * @brief 关闭LED
 *
 * 关闭LED显示。
 * 如果正在闪烁或淡入淡出则停止这些效果。
 */
void GpioLed::TurnOff()
{
    if (!ledc_initialized_)
    {
        return; // 如果LEDC未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);                                      // 停止闪烁定时器
    ledc_fade_stop(ledc_channel_.speed_mode, ledc_channel_.channel);   // 停止淡入淡出
    ledc_set_duty(ledc_channel_.speed_mode, ledc_channel_.channel, 0); // 设置占空比为0
    ledc_update_duty(ledc_channel_.speed_mode, ledc_channel_.channel); // 更新占空比
}

/**
 * @brief 单次闪烁
 *
 * 执行一次短暂的LED闪烁（100ms间隔）。
 */
void GpioLed::BlinkOnce()
{
    Blink(1, 100); // 闪烁1次，间隔100ms
}

/**
 * @brief 指定次数闪烁
 *
 * 执行指定次数的LED闪烁。
 *
 * @param times 闪烁次数
 * @param interval_ms 闪烁间隔时间（毫秒）
 */
void GpioLed::Blink(int times, int interval_ms)
{
    StartBlinkTask(times, interval_ms); // 启动闪烁任务
}

/**
 * @brief 持续闪烁
 *
 * 启动无限循环的LED闪烁效果。
 *
 * @param interval_ms 闪烁间隔时间（毫秒）
 */
void GpioLed::StartContinuousBlink(int interval_ms)
{
    StartBlinkTask(BLINK_INFINITE, interval_ms); // 启动无限闪烁任务
}

/**
 * @brief 启动LED闪烁任务
 *
 * 启动指定次数和间隔的LED闪烁效果。
 *
 * @param times 闪烁次数，-1表示无限闪烁
 * @param interval_ms 闪烁间隔时间（毫秒）
 */
void GpioLed::StartBlinkTask(int times, int interval_ms)
{
    if (!ledc_initialized_)
    {
        return; // 如果LEDC未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);                                    // 停止当前定时器
    ledc_fade_stop(ledc_channel_.speed_mode, ledc_channel_.channel); // 停止淡入淡出

    blink_counter_ = times * 2;                                 // 闪烁计数器设为次数的2倍（亮和灭各一次为一个完整闪烁）
    blink_interval_ms_ = interval_ms;                           // 设置闪烁间隔时间
    esp_timer_start_periodic(blink_timer_, interval_ms * 1000); // 启动周期性定时器（微秒）
}

/**
 * @brief LED闪烁定时器回调函数
 *
 * 定时器触发时调用此函数，控制LED的亮灭切换。
 * 当闪烁计数器减到0时停止定时器。
 */
void GpioLed::OnBlinkTimer()
{
    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    blink_counter_--; // 递减闪烁计数器

    // 根据计数器奇偶性决定LED亮灭
    if (blink_counter_ & 1)
    { // 奇数时点亮LED
        ledc_set_duty(ledc_channel_.speed_mode, ledc_channel_.channel, duty_);
    }
    else
    { // 偶数时关闭LED
        ledc_set_duty(ledc_channel_.speed_mode, ledc_channel_.channel, 0);

        // 如果计数器为0且不是无限闪烁，则停止定时器
        if (blink_counter_ == 0)
        {
            esp_timer_stop(blink_timer_);
        }
    }
    ledc_update_duty(ledc_channel_.speed_mode, ledc_channel_.channel); // 更新占空比
}

/**
 * @brief 启动淡入淡出任务
 *
 * 启动LED的淡入淡出动画效果。
 * LED会在最大亮度和关闭状态之间循环淡入淡出。
 */
void GpioLed::StartFadeTask()
{
    if (!ledc_initialized_)
    {
        return; // 如果LEDC未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);                                    // 停止闪烁定时器
    ledc_fade_stop(ledc_channel_.speed_mode, ledc_channel_.channel); // 停止当前淡入淡出
    fade_up_ = true;                                                 // 设置淡入方向

    // 配置淡入效果，从0到最大占空比
    ledc_set_fade_with_time(ledc_channel_.speed_mode,
                            ledc_channel_.channel, LEDC_DUTY, LEDC_FADE_TIME);

    // 启动淡入淡出，不等待完成
    ledc_fade_start(ledc_channel_.speed_mode,
                    ledc_channel_.channel, LEDC_FADE_NO_WAIT);
}

/**
 * @brief 淡入淡出结束回调函数
 *
 * 当LEDC淡入淡出效果结束时调用此函数，切换淡入淡出方向并开始下一个动画。
 */
void GpioLed::OnFadeEnd()
{
    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    fade_up_ = !fade_up_; // 切换淡入淡出方向

    // 配置下一个淡入淡出效果
    ledc_set_fade_with_time(ledc_channel_.speed_mode,
                            ledc_channel_.channel, fade_up_ ? LEDC_DUTY : 0, LEDC_FADE_TIME);

    // 启动淡入淡出，不等待完成
    ledc_fade_start(ledc_channel_.speed_mode,
                    ledc_channel_.channel, LEDC_FADE_NO_WAIT);
}

/**
 * @brief LEDC淡入淡出回调函数
 *
 * LEDC外设淡入淡出完成时的中断回调函数。
 * 当淡入淡出事件结束时调用OnFadeEnd()函数。
 *
 * @param param LEDC回调参数结构体指针
 * @param user_arg 用户自定义参数（指向GpioLed对象的指针）
 * @return 返回true表示处理成功
 */
bool IRAM_ATTR GpioLed::FadeCallback(const ledc_cb_param_t *param, void *user_arg)
{
    // 检查是否是淡入淡出结束事件
    if (param->event == LEDC_FADE_END_EVT)
    {
        auto led = static_cast<GpioLed *>(user_arg);
        led->OnFadeEnd(); // 调用淡入淡出结束处理函数
    }
    return true;
}

/**
 * @brief 当设备状态改变时调用此函数更新LED显示
 *
 * 根据设备的当前状态（如启动、WiFi配置、空闲、连接、监听、说话、升级等）
 * 来设置LED的亮度和显示效果。
 *
 * 不同状态对应的LED效果：
 * - 启动状态：默认亮度LED快速闪烁(100ms间隔)
 * - WiFi配置状态：默认亮度LED慢速闪烁(500ms间隔)
 * - 空闲状态：低亮度常亮
 * - 连接状态：默认亮度常亮
 * - 监听状态：根据是否有语音检测显示不同亮度的LED，并使用淡入淡出效果
 * - 说话状态：高亮度常亮
 * - 升级状态：中等亮度LED快速闪烁(100ms间隔)
 * - 激活状态：中等亮度LED慢速闪烁(500ms间隔)
 */
void GpioLed::OnStateChanged()
{
    auto &app = Application::GetInstance();   // 获取应用程序实例
    auto device_state = app.GetDeviceState(); // 获取当前设备状态

    // 根据设备状态设置不同的LED效果
    switch (device_state)
    {
    case kDeviceStateStarting:             // 启动状态
        SetBrightness(DEFAULT_BRIGHTNESS); // 设置默认亮度
        StartContinuousBlink(100);         // 快速闪烁(100ms间隔)
        break;

    case kDeviceStateWifiConfiguring:      // WiFi配置状态
        SetBrightness(DEFAULT_BRIGHTNESS); // 设置默认亮度
        StartContinuousBlink(500);         // 慢速闪烁(500ms间隔)
        break;

    case kDeviceStateIdle:              // 空闲状态
        SetBrightness(IDLE_BRIGHTNESS); // 设置低亮度
        TurnOn();                       // 常亮
        // TurnOff();                          // 注释掉的关闭选项
        break;

    case kDeviceStateConnecting:           // 连接状态
        SetBrightness(DEFAULT_BRIGHTNESS); // 设置默认亮度
        TurnOn();                          // 常亮
        break;

    case kDeviceStateListening:    // 监听状态
    case kDeviceStateAudioTesting: // 音频测试状态
        if (app.IsVoiceDetected())
        {                                   // 如果检测到语音
            SetBrightness(HIGH_BRIGHTNESS); // 设置高亮度
        }
        else
        {                                  // 如果未检测到语音
            SetBrightness(LOW_BRIGHTNESS); // 设置低亮度
        }
        // TurnOn();                           // 注释掉的常亮选项
        StartFadeTask(); // 启动淡入淡出效果
        break;

    case kDeviceStateSpeaking:              // 说话状态
        SetBrightness(SPEAKING_BRIGHTNESS); // 设置说话亮度
        TurnOn();                           // 常亮
        break;

    case kDeviceStateUpgrading:              // 升级状态
        SetBrightness(UPGRADING_BRIGHTNESS); // 设置升级亮度
        StartContinuousBlink(100);           // 快速闪烁(100ms间隔)
        break;

    case kDeviceStateActivating:              // 激活状态
        SetBrightness(ACTIVATING_BRIGHTNESS); // 设置激活亮度
        StartContinuousBlink(500);            // 慢速闪烁(500ms间隔)
        break;

    default: // 未知状态
        ESP_LOGE(TAG, "Unknown gpio led event: %d", device_state);
        return;
    }
}