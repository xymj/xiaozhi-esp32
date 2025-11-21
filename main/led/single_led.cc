#include "single_led.h"
#include "application.h"
#include <esp_log.h>

#define TAG "SingleLed"

// 默认亮度值定义
#define DEFAULT_BRIGHTNESS 4 ///< 默认LED亮度值
#define HIGH_BRIGHTNESS 16   ///< 高亮度值（用于语音检测时）
#define LOW_BRIGHTNESS 2     ///< 低亮度值（用于待机状态）

// 无限闪烁标识
#define BLINK_INFINITE -1 ///< 无限闪烁标识，用于表示持续闪烁

/**
 * @brief 构造函数，初始化单个LED控制器
 *
 * 初始化指定GPIO引脚上的LED控制器，配置为WS2812类型的LED灯带。
 * 同时创建用于控制LED闪烁效果的定时器。
 *
 * @param gpio 控制LED的GPIO引脚号
 */
SingleLed::SingleLed(gpio_num_t gpio)
{
    // 如果GPIO未连接，应该使用NoLed类而不是这个类
    assert(gpio != GPIO_NUM_NC);

    // 配置LED灯带参数
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = gpio;                                      // 设置LED控制引脚
    strip_config.max_leds = 1;                                               // 只控制一个LED
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB; // GRB颜色格式
    strip_config.led_model = LED_MODEL_WS2812;                               // WS2812 LED型号

    // 配置RMT（远程控制模块）参数
    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz RMT时钟分辨率

    // 创建LED灯带设备并检查错误
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_));
    led_strip_clear(led_strip_); // 清除LED显示

    // 配置LED闪烁定时器参数
    esp_timer_create_args_t blink_timer_args = {
        // 定时器回调函数，使用lambda表达式捕获this指针
        .callback = [](void *arg)
        {
            auto led = static_cast<SingleLed *>(arg);
            led->OnBlinkTimer(); // 调用定时器处理函数
        },
        .arg = this,                       // 回调函数参数，传递this指针
        .dispatch_method = ESP_TIMER_TASK, // 在任务上下文中执行定时器回调
        .name = "blink_timer",             // 定时器名称
        .skip_unhandled_events = false,    // 不跳过未处理的事件
    };

    // 创建定时器并检查错误
    ESP_ERROR_CHECK(esp_timer_create(&blink_timer_args, &blink_timer_));
}

/**
 * @brief 析构函数，释放LED控制器资源
 *
 * 停止并删除LED闪烁定时器，释放LED灯带资源。
 */
SingleLed::~SingleLed()
{
    esp_timer_stop(blink_timer_); // 停止定时器
    if (led_strip_ != nullptr)
    {
        led_strip_del(led_strip_); // 删除LED灯带设备
    }
}

/**
 * @brief 设置LED颜色
 *
 * 设置LED的RGB颜色值，但不立即显示，需要调用TurnOn()才会点亮。
 *
 * @param r 红色分量值(0-255)
 * @param g 绿色分量值(0-255)
 * @param b 蓝色分量值(0-255)
 */
void SingleLed::SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    r_ = r;
    g_ = g;
    b_ = b;
}

/**
 * @brief 打开LED
 *
 * 将LED设置为当前配置的颜色并点亮。
 * 如果正在闪烁则停止闪烁定时器。
 */
void SingleLed::TurnOn()
{
    if (led_strip_ == nullptr)
    {
        return; // 如果LED设备未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_);                   // 停止闪烁定时器
    led_strip_set_pixel(led_strip_, 0, r_, g_, b_); // 设置LED像素颜色
    led_strip_refresh(led_strip_);                  // 刷新显示
}

/**
 * @brief 关闭LED
 *
 * 关闭LED显示并清除颜色。
 * 如果正在闪烁则停止闪烁定时器。
 */
void SingleLed::TurnOff()
{
    if (led_strip_ == nullptr)
    {
        return; // 如果LED设备未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_); // 停止闪烁定时器
    led_strip_clear(led_strip_);  // 清除LED显示
}

/**
 * @brief 单次闪烁
 *
 * 执行一次短暂的LED闪烁（100ms间隔）。
 */
void SingleLed::BlinkOnce()
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
void SingleLed::Blink(int times, int interval_ms)
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
void SingleLed::StartContinuousBlink(int interval_ms)
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
void SingleLed::StartBlinkTask(int times, int interval_ms)
{
    if (led_strip_ == nullptr)
    {
        return; // 如果LED设备未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(blink_timer_); // 停止当前定时器

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
void SingleLed::OnBlinkTimer()
{
    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    blink_counter_--; // 递减闪烁计数器

    // 根据计数器奇偶性决定LED亮灭
    if (blink_counter_ & 1)
    { // 奇数时点亮LED
        led_strip_set_pixel(led_strip_, 0, r_, g_, b_);
        led_strip_refresh(led_strip_);
    }
    else
    { // 偶数时关闭LED
        led_strip_clear(led_strip_);

        // 如果计数器为0且不是无限闪烁，则停止定时器
        if (blink_counter_ == 0)
        {
            esp_timer_stop(blink_timer_);
        }
    }
}

/**
 * @brief 当设备状态改变时调用此函数更新LED显示
 *
 * 根据设备的当前状态（如启动、WiFi配置、空闲、连接、监听、说话、升级等）
 * 来设置LED的颜色和闪烁模式。
 *
 * 不同状态对应的LED效果：
 * - 启动状态：蓝色LED快速闪烁(100ms间隔)
 * - WiFi配置：蓝色LED慢速闪烁(500ms间隔)
 * - 空闲状态：关闭LED
 * - 连接状态：常亮蓝色LED
 * - 监听状态：根据是否有语音检测显示不同亮度的红色LED
 * - 说话状态：常亮绿色LED
 * - 升级状态：绿色LED快速闪烁(100ms间隔)
 * - 激活状态：绿色LED慢速闪烁(500ms间隔)
 */
void SingleLed::OnStateChanged()
{
    auto &app = Application::GetInstance();   // 获取应用程序实例
    auto device_state = app.GetDeviceState(); // 获取当前设备状态

    // 根据设备状态设置不同的LED效果
    switch (device_state)
    {
    case kDeviceStateStarting:              // 启动状态
        SetColor(0, 0, DEFAULT_BRIGHTNESS); // 蓝色LED
        StartContinuousBlink(100);          // 快速闪烁(100ms间隔)
        break;

    case kDeviceStateWifiConfiguring:       // WiFi配置状态
        SetColor(0, 0, DEFAULT_BRIGHTNESS); // 蓝色LED
        StartContinuousBlink(500);          // 慢速闪烁(500ms间隔)
        break;

    case kDeviceStateIdle: // 空闲状态
        TurnOff();         // 关闭LED
        break;

    case kDeviceStateConnecting:            // 连接状态
        SetColor(0, 0, DEFAULT_BRIGHTNESS); // 蓝色LED
        TurnOn();                           // 常亮
        break;

    case kDeviceStateListening:    // 监听状态
    case kDeviceStateAudioTesting: // 音频测试状态
        if (app.IsVoiceDetected())
        {                                    // 如果检测到语音
            SetColor(HIGH_BRIGHTNESS, 0, 0); // 高亮度红色LED
        }
        else
        {                                   // 如果未检测到语音
            SetColor(LOW_BRIGHTNESS, 0, 0); // 低亮度红色LED
        }
        TurnOn(); // 常亮
        break;

    case kDeviceStateSpeaking:              // 说话状态
        SetColor(0, DEFAULT_BRIGHTNESS, 0); // 绿色LED
        TurnOn();                           // 常亮
        break;

    case kDeviceStateUpgrading:             // 升级状态
        SetColor(0, DEFAULT_BRIGHTNESS, 0); // 绿色LED
        StartContinuousBlink(100);          // 快速闪烁(100ms间隔)
        break;

    case kDeviceStateActivating:            // 激活状态
        SetColor(0, DEFAULT_BRIGHTNESS, 0); // 绿色LED
        StartContinuousBlink(500);          // 慢速闪烁(500ms间隔)
        break;

    default: // 未知状态
        ESP_LOGW(TAG, "Unknown led strip event: %d", device_state);
        return;
    }
}