#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// 音频输入采样率设置为24kHz
#define AUDIO_INPUT_SAMPLE_RATE 24000

// 音频输出采样率设置为24kHz
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// 音频输入参考电压设置，true表示使用内部参考
#define AUDIO_INPUT_REFERENCE true

// I2S音频接口GPIO引脚定义
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_2  // I2S主时钟引脚
#define AUDIO_I2S_GPIO_WS GPIO_NUM_47   // I2S字选择/左右声道选择引脚
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_17 // I2S位时钟引脚
#define AUDIO_I2S_GPIO_DIN GPIO_NUM_16  // I2S数据输入引脚
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_15 // I2S数据输出引脚

// 音频编解码器相关GPIO引脚和地址配置
#define AUDIO_CODEC_PA_PIN GPIO_NUM_46                    // 功放使能引脚
#define AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_8                // I2C数据线引脚
#define AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_18               // I2C时钟线引脚
#define AUDIO_CODEC_ES8311_ADDR ES8311_CODEC_DEFAULT_ADDR // ES8311编解码器I2C地址
#define AUDIO_CODEC_ES7210_ADDR ES7210_CODEC_DEFAULT_ADDR // ES7210编解码器I2C地址

// LED和按键GPIO配置
#define BUILTIN_LED_GPIO GPIO_NUM_NC        // 板载LED引脚，NC表示未连接
#define BOOT_BUTTON_GPIO GPIO_NUM_0         // 启动按键引脚
#define VOLUME_UP_BUTTON_GPIO GPIO_NUM_NC   // 音量增加按键，NC表示未连接
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC // 音量减小按键，NC表示未连接

// 显示屏参数配置
#define DISPLAY_WIDTH 320     // 显示屏宽度(像素)
#define DISPLAY_HEIGHT 240    // 显示屏高度(像素)
#define DISPLAY_MIRROR_X true // X轴镜像显示
#define DISPLAY_MIRROR_Y true // Y轴镜像显示
#define DISPLAY_SWAP_XY false // 是否交换X/Y轴

// 显示屏偏移量配置
#define DISPLAY_OFFSET_X 0 // X轴显示偏移量
#define DISPLAY_OFFSET_Y 0 // Y轴显示偏移量

// 显示屏背光控制配置
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_45     // 背光控制引脚
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false // 背光输出是否反相

#endif // _BOARD_CONFIG_H_