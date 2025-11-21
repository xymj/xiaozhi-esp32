#ifndef _SINGLE_LED_H_
#define _SINGLE_LED_H_

#include "led.h"
#include <driver/gpio.h>
#include <led_strip.h>
#include <esp_timer.h>
#include <atomic>
#include <mutex>

/**
 * @brief 单个LED灯控制类
 *
 * SingleLed类是Led基类的具体实现，用于控制单个WS2812等数字LED灯。
 * 该类支持设置LED颜色、开关控制、闪烁效果等功能，并根据设备状态自动调整LED显示。
 *
 * 该类使用了ESP-IDF的LED Strip驱动和定时器功能来实现LED控制。
 */
class SingleLed : public Led
{
public:
    /**
     * @brief 构造函数，初始化单个LED控制器
     *
     * 初始化指定GPIO引脚上的LED控制器，配置为WS2812类型的LED灯带。
     * 同时创建用于控制LED闪烁效果的定时器。
     *
     * @param gpio 控制LED的GPIO引脚号
     */
    SingleLed(gpio_num_t gpio);

    /**
     * @brief 析构函数，释放LED控制器资源
     *
     * 停止并删除LED闪烁定时器，释放LED灯带资源。
     */
    virtual ~SingleLed();

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
    void OnStateChanged() override;

private:
    std::mutex mutex_;                         ///< 互斥锁，用于保护LED访问的线程安全
    TaskHandle_t blink_task_ = nullptr;        ///< 闪烁任务句柄（当前未使用）
    led_strip_handle_t led_strip_ = nullptr;   ///< LED灯带句柄，用于控制LED硬件
    uint8_t r_ = 0, g_ = 0, b_ = 0;            ///< 当前设置的RGB颜色值
    int blink_counter_ = 0;                    ///< 闪烁计数器，控制闪烁次数
    int blink_interval_ms_ = 0;                ///< 闪烁间隔时间（毫秒）
    esp_timer_handle_t blink_timer_ = nullptr; ///< 用于控制LED闪烁的定时器句柄

    /**
     * @brief 启动LED闪烁任务
     *
     * 启动指定次数和间隔的LED闪烁效果。
     *
     * @param times 闪烁次数，-1表示无限闪烁
     * @param interval_ms 闪烁间隔时间（毫秒）
     */
    void StartBlinkTask(int times, int interval_ms);

    /**
     * @brief LED闪烁定时器回调函数
     *
     * 定时器触发时调用此函数，控制LED的亮灭切换。
     * 当闪烁计数器减到0时停止定时器。
     */
    void OnBlinkTimer();

    /**
     * @brief 单次闪烁
     *
     * 执行一次短暂的LED闪烁（100ms间隔）。
     */
    void BlinkOnce();

    /**
     * @brief 指定次数闪烁
     *
     * 执行指定次数的LED闪烁。
     *
     * @param times 闪烁次数
     * @param interval_ms 闪烁间隔时间（毫秒）
     */
    void Blink(int times, int interval_ms);

    /**
     * @brief 持续闪烁
     *
     * 启动无限循环的LED闪烁效果。
     *
     * @param interval_ms 闪烁间隔时间（毫秒）
     */
    void StartContinuousBlink(int interval_ms);

    /**
     * @brief 打开LED
     *
     * 将LED设置为当前配置的颜色并点亮。
     * 如果正在闪烁则停止闪烁定时器。
     */
    void TurnOn();

    /**
     * @brief 关闭LED
     *
     * 关闭LED显示并清除颜色。
     * 如果正在闪烁则停止闪烁定时器。
     */
    void TurnOff();

    /**
     * @brief 设置LED颜色
     *
     * 设置LED的RGB颜色值，但不立即显示，需要调用TurnOn()才会点亮。
     *
     * @param r 红色分量值(0-255)
     * @param g 绿色分量值(0-255)
     * @param b 蓝色分量值(0-255)
     */
    void SetColor(uint8_t r, uint8_t g, uint8_t b);
};

#endif // _SINGLE_LED_H_