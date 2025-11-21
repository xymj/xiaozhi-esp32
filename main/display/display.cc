#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <font_awesome.h>

#include "display.h"
#include "board.h"
#include "application.h"
#include "audio_codec.h"
#include "settings.h"
#include "assets/lang_config.h"

#define TAG "Display"

/**
 * @brief Display构造函数
 *
 * 初始化Display对象。这是一个空实现，具体的初始化工作由派生类完成。
 * 派生类可以在自己的构造函数中初始化显示设备的宽度、高度等属性。
 */
Display::Display()
{
}

/**
 * @brief Display析构函数
 *
 * 虚析构函数确保通过基类指针删除派生类对象时能正确调用派生类的析构函数。
 * 这是一个空实现，具体的清理工作由派生类完成。
 */
Display::~Display()
{
}

/**
 * @brief 设置设备状态信息
 *
 * 在显示屏上显示设备当前状态信息，如"正在连接WiFi"、"已连接"等。
 * 当前实现只是将状态信息记录到日志中，具体的显示实现由派生类完成。
 *
 * @param status 状态信息字符串
 */
void Display::SetStatus(const char *status)
{
    ESP_LOGW(TAG, "SetStatus: %s", status);
}

/**
 * @brief 显示通知消息（std::string版本）
 *
 * 在显示屏上临时显示一条通知消息，指定显示持续时间。
 * 该函数是ShowNotification(const char*, int)的重载版本，将std::string转换为C风格字符串后调用另一个版本。
 *
 * @param notification 通知消息内容
 * @param duration_ms 显示持续时间（毫秒），默认3000ms
 */
void Display::ShowNotification(const std::string &notification, int duration_ms)
{
    ShowNotification(notification.c_str(), duration_ms);
}

/**
 * @brief 显示通知消息（C风格字符串版本）
 *
 * 在显示屏上临时显示一条通知消息，指定显示持续时间。
 * 当前实现只是将通知信息记录到日志中，具体的显示实现由派生类完成。
 *
 * @param notification 通知消息内容
 * @param duration_ms 显示持续时间（毫秒），默认3000ms
 */
void Display::ShowNotification(const char *notification, int duration_ms)
{
    ESP_LOGW(TAG, "ShowNotification: %s", notification);
}

/**
 * @brief 更新状态栏显示
 *
 * 刷新显示屏顶部状态栏的信息显示。
 * 这是一个空实现，具体的更新逻辑由派生类完成。
 *
 * @param update_all 是否更新所有状态信息，false表示只更新变化部分
 */
void Display::UpdateStatusBar(bool update_all)
{
}

/**
 * @brief 设置显示表情
 *
 * 在显示屏上显示指定的表情图标或动画。
 * 当前实现只是将表情信息记录到日志中，具体的显示实现由派生类完成。
 *
 * @param emotion 表情标识符字符串
 */
void Display::SetEmotion(const char *emotion)
{
    ESP_LOGW(TAG, "SetEmotion: %s", emotion);
}

/**
 * @brief 设置聊天消息显示
 *
 * 在显示屏上显示聊天对话内容，区分发送者角色（用户或AI）。
 * 当前实现只是将聊天信息记录到日志中，具体的显示实现由派生类完成。
 *
 * @param role 发送者角色，如"user"或"assistant"
 * @param content 消息内容
 */
void Display::SetChatMessage(const char *role, const char *content)
{
    ESP_LOGW(TAG, "Role:%s", role);
    ESP_LOGW(TAG, "     %s", content);
}

/**
 * @brief 设置显示主题
 *
 * 应用指定的主题样式到显示界面。
 * 该函数会保存当前主题指针，并将主题名称保存到NVS存储中，以便设备重启后能恢复主题设置。
 *
 * @param theme 指向Theme对象的指针
 */
void Display::SetTheme(Theme *theme)
{
    current_theme_ = theme;

    // 创建display命名空间的设置对象，true表示以读写模式打开
    Settings settings("display", true);

    // 将主题名称保存到NVS存储中
    settings.SetString("theme", theme->name());
}

/**
 * @brief 设置省电模式
 *
 * 启用或禁用显示屏的省电模式，可能包括降低亮度、关闭背光等操作。
 * 当前实现只是将省电模式状态记录到日志中，具体的省电模式实现由派生类完成。
 *
 * @param on true表示启用省电模式，false表示禁用
 */
void Display::SetPowerSaveMode(bool on)
{
    ESP_LOGW(TAG, "SetPowerSaveMode: %d", on);
}