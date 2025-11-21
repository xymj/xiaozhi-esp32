#ifndef _CIRCULAR_STRIP_H_
#define _CIRCULAR_STRIP_H_

#include "led.h"
#include <driver/gpio.h>
#include <led_strip.h>
#include <esp_timer.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>

// 默认亮度值定义
#define DEFAULT_BRIGHTNESS 32 ///< 默认LED亮度值
#define LOW_BRIGHTNESS 4      ///< 低亮度值

/**
 * @brief LED灯带颜色结构体
 *
 * 用于表示LED灯带中单个LED的颜色值，包含红、绿、蓝三个分量。
 */
struct StripColor
{
    uint8_t red = 0;   ///< 红色分量值(0-255)
    uint8_t green = 0; ///< 绿色分量值(0-255)
    uint8_t blue = 0;  ///< 蓝色分量值(0-255)
};

/**
 * @brief 环形LED灯带控制类
 *
 * CircularStrip类是Led基类的具体实现，用于控制环形排列的WS2812等数字LED灯带。
 * 该类支持设置LED颜色、亮度控制、闪烁、呼吸、滚动等多种视觉效果。
 *
 * 该类使用了ESP-IDF的LED Strip驱动和定时器功能来实现LED控制，
 * 并根据设备状态自动调整LED显示效果。
 */
class CircularStrip : public Led
{
public:
    /**
     * @brief 构造函数，初始化环形LED灯带控制器
     *
     * 初始化指定GPIO引脚上的LED灯带控制器，配置为WS2812类型的LED灯带。
     * 同时创建用于控制LED动画效果的定时器。
     *
     * @param gpio 控制LED灯带的GPIO引脚号
     * @param max_leds LED灯带上的LED数量
     */
    CircularStrip(gpio_num_t gpio, uint8_t max_leds);

    /**
     * @brief 析构函数，释放LED灯带控制器资源
     *
     * 停止并删除LED动画定时器，释放LED灯带资源。
     */
    virtual ~CircularStrip();

    /**
     * @brief 当设备状态改变时调用此函数更新LED显示
     *
     * 根据设备的当前状态（如启动、WiFi配置、空闲、连接、监听、说话、升级等）
     * 来设置LED的颜色和动画效果。
     *
     * 不同状态对应的LED效果：
     * - 启动状态：蓝色LED滚动效果(3个LED长度，100ms间隔)
     * - WiFi配置：蓝白色LED慢速闪烁(500ms间隔)
     * - 空闲状态：淡出效果
     * - 连接状态：蓝白色常亮
     * - 监听状态：红色常亮
     * - 说话状态：绿色常亮
     * - 升级状态：绿色LED快速闪烁(100ms间隔)
     * - 激活状态：绿色LED慢速闪烁(500ms间隔)
     */
    void OnStateChanged() override;

    /**
     * @brief 设置LED亮度
     *
     * 设置LED的默认亮度和低亮度值，并重新应用当前设备状态对应的LED效果。
     *
     * @param default_brightness 默认亮度值
     * @param low_brightness 低亮度值
     */
    void SetBrightness(uint8_t default_brightness, uint8_t low_brightness);

    /**
     * @brief 设置所有LED颜色
     *
     * 将LED灯带上的所有LED设置为指定颜色并立即显示。
     * 如果正在执行动画效果则停止这些效果。
     *
     * @param color 要设置的颜色值
     */
    void SetAllColor(StripColor color);

    /**
     * @brief 设置单个LED颜色
     *
     * 将LED灯带上的指定索引的LED设置为指定颜色并立即显示。
     * 如果正在执行动画效果则停止这些效果。
     *
     * @param index LED的索引位置(0到max_leds_-1)
     * @param color 要设置的颜色值
     */
    void SetSingleColor(uint8_t index, StripColor color);

    /**
     * @brief LED闪烁效果
     *
     * 启动LED灯带的闪烁动画效果，所有LED同时亮灭。
     *
     * @param color 闪烁时的LED颜色
     * @param interval_ms 闪烁间隔时间（毫秒）
     */
    void Blink(StripColor color, int interval_ms);

    /**
     * @brief LED呼吸效果
     *
     * 启动LED灯带的呼吸动画效果，在指定的最低和最高颜色之间渐变。
     *
     * @param low 呼吸效果的最低颜色值
     * @param high 呼吸效果的最高颜色值
     * @param interval_ms 呼吸动画间隔时间（毫秒）
     */
    void Breathe(StripColor low, StripColor high, int interval_ms);

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
    void Scroll(StripColor low, StripColor high, int length, int interval_ms);

private:
    std::mutex mutex_;                               ///< 互斥锁，用于保护LED访问的线程安全
    TaskHandle_t blink_task_ = nullptr;              ///< 闪烁任务句柄（当前未使用）
    led_strip_handle_t led_strip_ = nullptr;         ///< LED灯带句柄，用于控制LED硬件
    int max_leds_ = 0;                               ///< LED灯带上的LED数量
    std::vector<StripColor> colors_;                 ///< 存储每个LED当前颜色的向量
    int blink_counter_ = 0;                          ///< 闪烁计数器（当前未使用）
    int blink_interval_ms_ = 0;                      ///< 闪烁间隔时间（毫秒）（当前未使用）
    esp_timer_handle_t strip_timer_ = nullptr;       ///< 用于控制LED动画的定时器句柄
    std::function<void()> strip_callback_ = nullptr; ///< 当前动画效果的回调函数

    uint8_t default_brightness_ = DEFAULT_BRIGHTNESS; ///< 默认亮度值
    uint8_t low_brightness_ = LOW_BRIGHTNESS;         ///< 低亮度值

    /**
     * @brief 启动LED动画任务
     *
     * 启动指定间隔时间的LED动画效果，通过回调函数实现具体的动画逻辑。
     *
     * @param interval_ms 动画间隔时间（毫秒）
     * @param cb 动画效果的回调函数
     */
    void StartStripTask(int interval_ms, std::function<void()> cb);

    /**
     * @brief 彩虹效果（未实现）
     *
     * 预留的彩虹效果函数，当前未在代码中实现。
     *
     * @param low 彩虹效果的最低颜色值
     * @param high 彩虹效果的最高颜色值
     * @param interval_ms 彩虹动画间隔时间（毫秒）
     */
    void Rainbow(StripColor low, StripColor high, int interval_ms);

    /**
     * @brief 淡出效果
     *
     * 启动LED灯带的淡出动画效果，逐渐降低所有LED的亮度直到完全熄灭。
     *
     * @param interval_ms 淡出动画间隔时间（毫秒）
     */
    void FadeOut(int interval_ms);
};

#endif // _CIRCULAR_STRIP_H_