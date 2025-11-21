#include "circular_strip.h"
#include "application.h"
#include <esp_log.h>

#define TAG "CircularStrip"

// 无限闪烁标识
#define BLINK_INFINITE -1 ///< 无限闪烁标识，用于表示持续闪烁

/**
 * @brief 构造函数，初始化环形LED灯带控制器
 *
 * 初始化指定GPIO引脚上的LED灯带控制器，配置为WS2812类型的LED灯带。
 * 同时创建用于控制LED动画效果的定时器。
 *
 * @param gpio 控制LED灯带的GPIO引脚号
 * @param max_leds LED灯带上的LED数量
 */
CircularStrip::CircularStrip(gpio_num_t gpio, uint8_t max_leds) : max_leds_(max_leds)
{
    // 如果GPIO未连接，应该使用NoLed类而不是这个类
    assert(gpio != GPIO_NUM_NC);

    // 调整颜色向量大小以匹配LED数量
    colors_.resize(max_leds_);

    // 配置LED灯带参数
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = gpio;                                      // 设置LED控制引脚
    strip_config.max_leds = max_leds_;                                       // 设置LED数量
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB; // GRB颜色格式
    strip_config.led_model = LED_MODEL_WS2812;                               // WS2812 LED型号

    // 配置RMT（远程控制模块）参数
    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz RMT时钟分辨率

    // 创建LED灯带设备并检查错误
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_));
    led_strip_clear(led_strip_); // 清除LED显示

    // 配置LED动画定时器参数
    esp_timer_create_args_t strip_timer_args = {
        // 定时器回调函数，使用lambda表达式捕获this指针
        .callback = [](void *arg)
        {
            auto strip = static_cast<CircularStrip *>(arg);
            // 使用锁保护临界区，确保线程安全
            std::lock_guard<std::mutex> lock(strip->mutex_);
            // 如果有回调函数则执行
            if (strip->strip_callback_ != nullptr)
            {
                strip->strip_callback_();
            }
        },
        .arg = this,                       // 回调函数参数，传递this指针
        .dispatch_method = ESP_TIMER_TASK, // 在任务上下文中执行定时器回调
        .name = "strip_timer",             // 定时器名称
        .skip_unhandled_events = false,    // 不跳过未处理的事件
    };

    // 创建定时器并检查错误
    ESP_ERROR_CHECK(esp_timer_create(&strip_timer_args, &strip_timer_));
}

/**
 * @brief 析构函数，释放LED灯带控制器资源
 *
 * 停止并删除LED动画定时器，释放LED灯带资源。
 */
CircularStrip::~CircularStrip()
{
    esp_timer_stop(strip_timer_); // 停止定时器
    if (led_strip_ != nullptr)
    {
        led_strip_del(led_strip_); // 删除LED灯带设备
    }
}

/**
 * @brief 设置所有LED颜色
 *
 * 将LED灯带上的所有LED设置为指定颜色并立即显示。
 * 如果正在执行动画效果则停止这些效果。
 *
 * @param color 要设置的颜色值
 */
void CircularStrip::SetAllColor(StripColor color)
{
    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(strip_timer_); // 停止动画定时器

    // 设置所有LED的颜色并更新显示
    for (int i = 0; i < max_leds_; i++)
    {
        colors_[i] = color;                                                     // 保存颜色到内部缓存
        led_strip_set_pixel(led_strip_, i, color.red, color.green, color.blue); // 设置LED像素颜色
    }
    led_strip_refresh(led_strip_); // 刷新显示
}

/**
 * @brief 设置单个LED颜色
 *
 * 将LED灯带上的指定索引的LED设置为指定颜色并立即显示。
 * 如果正在执行动画效果则停止这些效果。
 *
 * @param index LED的索引位置(0到max_leds_-1)
 * @param color 要设置的颜色值
 */
void CircularStrip::SetSingleColor(uint8_t index, StripColor color)
{
    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(strip_timer_); // 停止动画定时器

    // 设置指定LED的颜色并更新显示
    colors_[index] = color;                                                     // 保存颜色到内部缓存
    led_strip_set_pixel(led_strip_, index, color.red, color.green, color.blue); // 设置LED像素颜色
    led_strip_refresh(led_strip_);                                              // 刷新显示
}

/**
 * @brief LED闪烁效果
 *
 * 启动LED灯带的闪烁动画效果，所有LED同时亮灭。
 *
 * @param color 闪烁时的LED颜色
 * @param interval_ms 闪烁间隔时间（毫秒）
 */
void CircularStrip::Blink(StripColor color, int interval_ms)
{
    // 设置所有LED的颜色
    for (int i = 0; i < max_leds_; i++)
    {
        colors_[i] = color;
    }

    // 启动闪烁动画任务
    StartStripTask(interval_ms, [this]()
                   {
                       static bool on = true; // 静态变量记录当前状态（亮或灭）
                       if (on)
                       {
                           // 点亮所有LED
                           for (int i = 0; i < max_leds_; i++)
                           {
                               led_strip_set_pixel(led_strip_, i, colors_[i].red, colors_[i].green, colors_[i].blue);
                           }
                           led_strip_refresh(led_strip_);
                       }
                       else
                       {
                           // 关闭所有LED
                           led_strip_clear(led_strip_);
                       }
                       on = !on; // 切换状态
                   });
}

/**
 * @brief 淡出效果
 *
 * 启动LED灯带的淡出动画效果，逐渐降低所有LED的亮度直到完全熄灭。
 *
 * @param interval_ms 淡出动画间隔时间（毫秒）
 */
void CircularStrip::FadeOut(int interval_ms)
{
    // 启动淡出动画任务
    StartStripTask(interval_ms, [this]()
                   {
        bool all_off = true; // 标记所有LED是否都已关闭
        
        // 逐个降低LED亮度
        for (int i = 0; i < max_leds_; i++) {
            colors_[i].red /= 2;   // 红色分量减半
            colors_[i].green /= 2; // 绿色分量减半
            colors_[i].blue /= 2;  // 蓝色分量减半
            
            // 检查是否还有LED未完全关闭
            if (colors_[i].red != 0 || colors_[i].green != 0 || colors_[i].blue != 0) {
                all_off = false;
            }
            
            // 设置当前LED颜色
            led_strip_set_pixel(led_strip_, i, colors_[i].red, colors_[i].green, colors_[i].blue);
        }
        
        // 如果所有LED都已关闭，则清除显示并停止定时器
        if (all_off) {
            led_strip_clear(led_strip_);
            esp_timer_stop(strip_timer_);
        } else {
            // 否则刷新显示
            led_strip_refresh(led_strip_);
        } });
}

/**
 * @brief LED呼吸效果
 *
 * 启动LED灯带的呼吸动画效果，在指定的最低和最高颜色之间渐变。
 *
 * @param low 呼吸效果的最低颜色值
 * @param high 呼吸效果的最高颜色值
 * @param interval_ms 呼吸动画间隔时间（毫秒）
 */
void CircularStrip::Breathe(StripColor low, StripColor high, int interval_ms)
{
    // Lambda表达式的语法和捕获列表的含义：
    // Lambda表达式的基本语法
    //  Lambda表达式的基本语法形式为： [capture](parameters) -> return_type { body }
    //     其中：
    //      [capture] 是捕获列表
    //      (parameters) 是参数列表
    //      -> return_type 是返回类型（可选）
    //      { body } 是函数体

    // 捕获列表的含义:
    // 捕获列表用于指定Lambda函数可以访问其定义所在作用域中的哪些变量
    //  这里的 [this, low, high] 表示：
    //      this - 捕获当前对象的this指针，使得Lambda函数可以访问当前类的成员变量和成员函数
    //      low - 按值捕获变量low，即把low的值复制到Lambda函数中
    //      high - 按值捕获变量high，即把high的值复制到Lambda函数中
    // 不同的捕获方式
    //  按值捕获（值拷贝）：[low, high]  // 按值捕获low和high变量
    //  按引用捕获：[&low, &high]  // 按引用捕获low和high变量
    //  混合捕获：[this, low, &high]  // 按值捕获this和low，按引用捕获high
    //  默认捕获： [=]  // 按值捕获所有外部变量
    //           [&]  // 按引用捕获所有外部变量

    // 启动呼吸动画任务
    StartStripTask(interval_ms, [this, low, high]()
                   {
        static bool increase = true;        // 静态变量记录当前变化方向（增加或减少）
        static StripColor color = low;      // 静态变量记录当前颜色
        
        // 根据变化方向调整颜色
        if (increase) {
            // 增加颜色分量直到达到最高值
            if (color.red < high.red) {
                color.red++;
            }
            if (color.green < high.green) {
                color.green++;
            }
            if (color.blue < high.blue) {
                color.blue++;
            }
            
            // 如果达到最高值，则改变变化方向
            if (color.red == high.red && color.green == high.green && color.blue == high.blue) {
                increase = false;
            }
        } else {
            // 减少颜色分量直到达到最低值
            if (color.red > low.red) {
                color.red--;
            }
            if (color.green > low.green) {
                color.green--;
            }
            if (color.blue > low.blue) {
                color.blue--;
            }
            
            // 如果达到最低值，则改变变化方向
            if (color.red == low.red && color.green == low.green && color.blue == low.blue) {
                increase = true;
            }
        }
        
        // 设置所有LED的颜色并刷新显示
        for (int i = 0; i < max_leds_; i++) {
            led_strip_set_pixel(led_strip_, i, color.red, color.green, color.blue);
        }
        led_strip_refresh(led_strip_); });
}

/**
 * @brief LED滚动效果
 *
 * 启动LED灯带的滚动动画效果，一段长度的LED以指定颜色在灯带上滚动。
 *
 * @param low 滚动背景的LED颜色
 * @param high 滚动段的LED颜色
 * @param length 滚动段的LED数量
 * @param interval_ms 滚动动画间隔时间（毫秒）
 */
void CircularStrip::Scroll(StripColor low, StripColor high, int length, int interval_ms)
{
    // 初始化所有LED为背景颜色
    for (int i = 0; i < max_leds_; i++)
    {
        colors_[i] = low;
    }

    // 启动滚动动画任务
    StartStripTask(interval_ms, [this, low, high, length]()
                   {
        static int offset = 0; // 静态变量记录滚动偏移量
        
        // 将所有LED设置为背景颜色
        for (int i = 0; i < max_leds_; i++) {
            colors_[i] = low;
        }
        
        // 设置滚动段的LED为高亮颜色
        for (int j = 0; j < length; j++) {
            int i = (offset + j) % max_leds_; // 计算滚动段LED的索引（环形）
            colors_[i] = high;
        }
        
        // 更新所有LED的显示
        for (int i = 0; i < max_leds_; i++) {
            led_strip_set_pixel(led_strip_, i, colors_[i].red, colors_[i].green, colors_[i].blue);
        }
        led_strip_refresh(led_strip_);
        
        // 更新滚动偏移量（环形）
        offset = (offset + 1) % max_leds_; });
}

/**
 * @brief 启动LED动画任务
 *
 * 启动指定间隔时间的LED动画效果，通过回调函数实现具体的动画逻辑。
 *
 * @param interval_ms 动画间隔时间（毫秒）
 * @param cb 动画效果的回调函数
 */
void CircularStrip::StartStripTask(int interval_ms, std::function<void()> cb)
{
    if (led_strip_ == nullptr)
    {
        return; // 如果LED设备未初始化则直接返回
    }

    // 使用锁保护临界区，确保线程安全
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(strip_timer_); // 停止当前定时器

    strip_callback_ = cb;                                       // 设置回调函数
    esp_timer_start_periodic(strip_timer_, interval_ms * 1000); // 启动周期性定时器（微秒）
}

/**
 * @brief 设置LED亮度
 *
 * 设置LED的默认亮度和低亮度值，并重新应用当前设备状态对应的LED效果。
 *
 * @param default_brightness 默认亮度值
 * @param low_brightness 低亮度值
 */
void CircularStrip::SetBrightness(uint8_t default_brightness, uint8_t low_brightness)
{
    default_brightness_ = default_brightness; // 设置默认亮度
    low_brightness_ = low_brightness;         // 设置低亮度
    OnStateChanged();                         // 重新应用当前状态的LED效果
}

/**
 * @brief 当设备状态改变时调用此函数更新LED显示
 *
 * 根据设备的当前状态（如启动、WiFi配置、空闲、连接、监听、说话、升级等）
 * 来设置LED的颜色和动画效果。
 *
 * 不同状态对应的LED效果：
 * - 启动状态：蓝色LED滚动效果(3个LED长度，100ms间隔)
 * - WiFi配置状态：蓝白色LED慢速闪烁(500ms间隔)
 * - 空闲状态：淡出效果
 * - 连接状态：蓝白色常亮
 * - 监听状态：红色常亮
 * - 说话状态：绿色常亮
 * - 升级状态：绿色LED快速闪烁(100ms间隔)
 * - 激活状态：绿色LED慢速闪烁(500ms间隔)
 */
void CircularStrip::OnStateChanged()
{
    auto &app = Application::GetInstance();   // 获取应用程序实例
    auto device_state = app.GetDeviceState(); // 获取当前设备状态

    // 根据设备状态设置不同的LED效果
    switch (device_state)
    {
    case kDeviceStateStarting:
    {                                                                              // 启动状态
        StripColor low = {0, 0, 0};                                                // 背景黑色
        StripColor high = {low_brightness_, low_brightness_, default_brightness_}; // 蓝色滚动段
        Scroll(low, high, 3, 100);                                                 // 启动滚动效果(3个LED长度，100ms间隔)
        break;
    }

    case kDeviceStateWifiConfiguring:
    {                                                                               // WiFi配置状态
        StripColor color = {low_brightness_, low_brightness_, default_brightness_}; // 蓝白色
        Blink(color, 500);                                                          // 慢速闪烁(500ms间隔)
        break;
    }

    case kDeviceStateIdle: // 空闲状态
        FadeOut(50);       // 启动淡出效果
        break;

    case kDeviceStateConnecting:
    {                                                                               // 连接状态
        StripColor color = {low_brightness_, low_brightness_, default_brightness_}; // 蓝白色
        SetAllColor(color);                                                         // 常亮蓝白色
        break;
    }

    case kDeviceStateListening: // 监听状态
    case kDeviceStateAudioTesting:
    {                                                                               // 音频测试状态
        StripColor color = {default_brightness_, low_brightness_, low_brightness_}; // 红色
        SetAllColor(color);                                                         // 常亮红色
        break;
    }

    case kDeviceStateSpeaking:
    {                                                                               // 说话状态
        StripColor color = {low_brightness_, default_brightness_, low_brightness_}; // 绿色
        SetAllColor(color);                                                         // 常亮绿色
        break;
    }

    case kDeviceStateUpgrading:
    {                                                                               // 升级状态
        StripColor color = {low_brightness_, default_brightness_, low_brightness_}; // 绿色
        Blink(color, 100);                                                          // 快速闪烁(100ms间隔)
        break;
    }

    case kDeviceStateActivating:
    {                                                                               // 激活状态
        StripColor color = {low_brightness_, default_brightness_, low_brightness_}; // 绿色
        Blink(color, 500);                                                          // 慢速闪烁(500ms间隔)
        break;
    }

    default: // 未知状态
        ESP_LOGW(TAG, "Unknown led strip event: %d", device_state);
        return;
    }
}