#ifndef _GPIO_LED_H_
#define _GPIO_LED_H_

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "led.h"
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_timer.h>
#include <atomic>
#include <mutex>

/**
 * @brief GPIO控制的LED灯类
 *
 * GpioLed类是Led基类的具体实现，用于控制通过GPIO引脚连接的普通LED灯。
 * 该类使用ESP-IDF的LEDC（LED PWM控制器）外设来实现LED亮度控制和淡入淡出效果。
 * 支持设置LED亮度、开关控制、闪烁效果和淡入淡出动画等功能。
 *
 * 该类根据设备状态自动调整LED显示效果，如启动、WiFi配置、空闲、连接、监听、说话、升级等状态。
 */
class GpioLed : public Led
{
public:
   /**
    * @brief 构造函数，使用默认参数初始化GPIO LED控制器
    *
    * 使用默认的LEDC定时器和通道配置初始化指定GPIO引脚上的LED控制器。
    *
    * @param gpio 控制LED的GPIO引脚号
    */
   GpioLed(gpio_num_t gpio);

   /**
    * @brief 构造函数，指定GPIO和输出反转设置
    *
    * 初始化指定GPIO引脚上的LED控制器，并设置输出是否反转。
    *
    * @param gpio 控制LED的GPIO引脚号
    * @param output_invert 输出反转标志，1表示反转，0表示不反转
    */
   GpioLed(gpio_num_t gpio, int output_invert);

   /**
    * @brief 构造函数，完全自定义参数初始化GPIO LED控制器
    *
    * 使用指定的GPIO引脚、输出反转设置、LEDC定时器和通道来初始化LED控制器。
    *
    * @param gpio 控制LED的GPIO引脚号
    * @param output_invert 输出反转标志，1表示反转，0表示不反转
    * @param timer_num LEDC定时器编号
    * @param channel LEDC通道编号
    */
   GpioLed(gpio_num_t gpio, int output_invert, ledc_timer_t timer_num, ledc_channel_t channel);

   /**
    * @brief 析构函数，释放LED控制器资源
    *
    * 停止并删除LED闪烁定时器，卸载LEDC淡入淡出功能。
    */
   virtual ~GpioLed();

   /**
    * @brief 当设备状态改变时调用此函数更新LED显示
    *
    * 根据设备的当前状态（如启动、WiFi配置、空闲、连接、监听、说话、升级等）
    * 来设置LED的亮度和显示效果。
    *
    * 不同状态对应的LED效果：
    * - 启动状态：默认亮度LED快速闪烁(100ms间隔)
    * - WiFi配置：默认亮度LED慢速闪烁(500ms间隔)
    * - 空闲状态：低亮度常亮
    * - 连接状态：默认亮度常亮
    * - 监听状态：根据是否有语音检测显示不同亮度的LED，并使用淡入淡出效果
    * - 说话状态：高亮度常亮
    * - 升级状态：中等亮度LED快速闪烁(100ms间隔)
    * - 激活状态：中等亮度LED慢速闪烁(500ms间隔)
    */
   void OnStateChanged() override;

   /**
    * @brief 打开LED
    *
    * 将LED设置为当前配置的亮度并点亮。
    * 如果正在闪烁或淡入淡出则停止这些效果。
    */
   void TurnOn();

   /**
    * @brief 关闭LED
    *
    * 关闭LED显示。
    * 如果正在闪烁或淡入淡出则停止这些效果。
    */
   void TurnOff();

   /**
    * @brief 设置LED亮度
    *
    * 设置LED的亮度级别，范围0-100。
    * 该设置不会立即生效，需要调用TurnOn()才会应用。
    *
    * @param brightness LED亮度百分比(0-100)
    */
   void SetBrightness(uint8_t brightness);

private:
   std::mutex mutex_;                         ///< 互斥锁，用于保护LED访问的线程安全
   TaskHandle_t blink_task_ = nullptr;        ///< 闪烁任务句柄（当前未使用）
   ledc_channel_config_t ledc_channel_ = {0}; ///< LEDC通道配置结构体
   bool ledc_initialized_ = false;            ///< LEDC是否已初始化标志
   uint32_t duty_ = 0;                        ///< 当前设置的PWM占空比值
   int blink_counter_ = 0;                    ///< 闪烁计数器，控制闪烁次数
   int blink_interval_ms_ = 0;                ///< 闪烁间隔时间（毫秒）
   esp_timer_handle_t blink_timer_ = nullptr; ///< 用于控制LED闪烁的定时器句柄
   bool fade_up_ = true;                      ///< 淡入淡出方向标志，true表示淡入，false表示淡出

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
    * @brief 启动淡入淡出任务
    *
    * 启动LED的淡入淡出动画效果。
    * LED会在最大亮度和关闭状态之间循环淡入淡出。
    */
   void StartFadeTask();

   /**
    * @brief 淡入淡出结束回调函数
    *
    * 当LEDC淡入淡出效果结束时调用此函数，切换淡入淡出方向并开始下一个动画。
    */
   void OnFadeEnd();

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
   static bool IRAM_ATTR FadeCallback(const ledc_cb_param_t *param, void *user_arg);
};

#endif // _GPIO_LED_H_