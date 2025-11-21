#include "wifi_board.h"

#include "display.h"
#include "application.h"
#include "system_info.h"
#include "settings.h"
#include "assets/lang_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_network.h>
#include <esp_log.h>

#include <font_awesome.h>
#include <wifi_station.h>
#include <wifi_configuration_ap.h>
#include <ssid_manager.h>
#include "afsk_demod.h"

static const char *TAG = "WifiBoard";

/**
 * @brief WifiBoard构造函数
 *
 * 初始化WiFi开发板对象，从NVS设置中读取是否强制进入AP模式。
 * 如果force_ap设置为1，则重置为0并准备进入WiFi配置模式。
 */
WifiBoard::WifiBoard()
{
    // 创建WiFi设置对象，用于读取和保存WiFi相关配置
    Settings settings("wifi", true);

    // 检查是否设置了强制AP模式（用于重新配置WiFi）
    wifi_config_mode_ = settings.GetInt("force_ap") == 1;

    // 如果设置了强制AP模式，记录日志并重置标志
    if (wifi_config_mode_)
    {
        ESP_LOGI(TAG, "force_ap is set to 1, reset to 0");
        settings.SetInt("force_ap", 0); // 重置标志位
    }
}

/**
 * @brief 获取开发板类型标识
 *
 * 返回当前开发板的类型标识字符串"wifi"。
 *
 * @return 返回开发板类型字符串"wifi"
 */
std::string WifiBoard::GetBoardType()
{
    return "wifi";
}

/**
 * @brief 进入WiFi配置模式
 *
 * 当设备需要进行WiFi网络配置时调用此函数。该函数会：
 * 1. 设置设备状态为WiFi配置中
 * 2. 启动WiFi配置热点(AP)
 * 3. 显示配置提示信息
 * 4. 等待用户通过网页或音频方式进行配置
 * 5. 配置完成后重启设备
 */
void WifiBoard::EnterWifiConfigMode()
{
    // 获取应用程序实例
    auto &application = Application::GetInstance();

    // 设置设备状态为WiFi配置中
    application.SetDeviceState(kDeviceStateWifiConfiguring);

    // 获取WiFi配置AP实例并启动
    auto &wifi_ap = WifiConfigurationAp::GetInstance();
    wifi_ap.SetLanguage(Lang::CODE);  // 设置语言
    wifi_ap.SetSsidPrefix("Xiaozhi"); // 设置SSID前缀
    wifi_ap.Start();                  // 启动AP

    // 等待1.5秒以显示开发板信息
    vTaskDelay(pdMS_TO_TICKS(1500));

    // 构造WiFi配置提示信息
    std::string hint = Lang::Strings::CONNECT_TO_HOTSPOT; // "手机连接热点 "
    hint += wifi_ap.GetSsid();                            // 添加AP的SSID
    hint += Lang::Strings::ACCESS_VIA_BROWSER;            // "，浏览器访问 "
    hint += wifi_ap.GetWebServerUrl();                    // 添加Web服务器URL

    // 发出WiFi配置提示通知
    application.Alert(
        Lang::Strings::WIFI_CONFIG_MODE, // 标题："配网模式"
        hint.c_str(),                    // 提示信息
        "gear",                          // 图标
        Lang::Sounds::OGG_WIFICONFIG     // 提示音
    );

// 如果启用声波WiFi配置功能
#if CONFIG_USE_ACOUSTIC_WIFI_PROVISIONING
    // 获取显示设备和音频编解码器
    auto display = Board::GetInstance().GetDisplay();
    auto codec = Board::GetInstance().GetAudioCodec();

    // 确定音频输入通道数
    int channel = 1;
    if (codec)
    {
        channel = codec->input_channels();
    }

    // 记录日志并开始接收音频中的WiFi凭证
    ESP_LOGI(TAG, "Start receiving WiFi credentials from audio, input channels: %d", channel);
    audio_wifi_config::ReceiveWifiCredentialsFromAudio(&application, &wifi_ap, display, channel);
#endif

    // 无限等待直到配置完成并重启
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * @brief 启动网络连接
 *
 * 初始化并启动WiFi网络连接功能，包括：
 * 1. 检查是否需要进入配置模式
 * 2. 扫描并连接已配置的WiFi网络
 * 3. 连接失败时自动进入配置模式
 */
void WifiBoard::StartNetwork()
{
    // 检查是否需要强制进入WiFi配置模式
    if (wifi_config_mode_)
    {
        EnterWifiConfigMode();
        return;
    }

    // 检查是否有已配置的WiFi网络，如果没有则进入配置模式
    auto &ssid_manager = SsidManager::GetInstance();
    auto ssid_list = ssid_manager.GetSsidList();
    if (ssid_list.empty())
    {
        wifi_config_mode_ = true;
        EnterWifiConfigMode();
        return;
    }

    // 获取WiFi站点实例并设置回调函数
    auto &wifi_station = WifiStation::GetInstance();

    // 设置扫描开始回调：显示扫描WiFi提示
    wifi_station.OnScanBegin([this]()
                             {
                                 auto display = Board::GetInstance().GetDisplay();
                                 display->ShowNotification(Lang::Strings::SCANNING_WIFI, 30000); // "扫描 Wi-Fi..."
                             });

    // 设置连接回调：显示正在连接提示
    wifi_station.OnConnect([this](const std::string &ssid)
                           {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECT_TO;  // "连接 "
        notification += ssid;
        notification += "...";
        display->ShowNotification(notification.c_str(), 30000); });

    // 设置连接成功回调：显示已连接提示
    wifi_station.OnConnected([this](const std::string &ssid)
                             {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECTED_TO;  // "已连接 "
        notification += ssid;
        notification += "...";
        display->ShowNotification(notification.c_str(), 30000); });

    // 启动WiFi站点
    wifi_station.Start();

    // 等待WiFi连接，超时时间60秒
    if (!wifi_station.WaitForConnected(60 * 1000))
    {
        // 连接失败，停止WiFi站点并进入配置模式
        wifi_station.Stop();
        wifi_config_mode_ = true;
        EnterWifiConfigMode();
        return;
    }
}

/**
 * @brief 获取网络接口对象
 *
 * 返回ESP32网络接口实例，用于进行网络通信。
 *
 * @return 返回网络接口对象指针
 */
NetworkInterface *WifiBoard::GetNetwork()
{
    static EspNetwork network; // 使用静态变量确保单例
    return &network;           // 返回网络接口实例
}

/**
 * @brief 获取网络状态图标标识
 *
 * 根据WiFi连接状态和信号强度返回相应的图标标识。
 *
 * @return 返回网络状态图标字符串
 */
const char *WifiBoard::GetNetworkStateIcon()
{
    // 如果处于WiFi配置模式，返回WiFi图标
    if (wifi_config_mode_)
    {
        return FONT_AWESOME_WIFI;
    }

    // 获取WiFi站点实例
    auto &wifi_station = WifiStation::GetInstance();

    // 如果未连接WiFi，返回WiFi斜杠图标
    if (!wifi_station.IsConnected())
    {
        return FONT_AWESOME_WIFI_SLASH;
    }

    // 根据信号强度返回不同图标
    int8_t rssi = wifi_station.GetRssi();
    if (rssi >= -60)
    {
        return FONT_AWESOME_WIFI; // 信号强
    }
    else if (rssi >= -70)
    {
        return FONT_AWESOME_WIFI_FAIR; // 信号中等
    }
    else
    {
        return FONT_AWESOME_WIFI_WEAK; // 信号弱
    }
}

/**
 * @brief 获取开发板特定的JSON配置信息
 *
 * 返回当前开发板特有的配置信息JSON字符串，包括：
 * - 开发板类型和名称
 * - WiFi连接信息（SSID、RSSI、信道、IP地址）
 * - MAC地址
 *
 * @return 返回开发板特定配置信息的JSON字符串
 */
std::string WifiBoard::GetBoardJson()
{
    // 获取WiFi站点实例
    auto &wifi_station = WifiStation::GetInstance();

    // 开始构建JSON字符串
    std::string board_json = R"({)";
    board_json += R"("type":")" + std::string(BOARD_TYPE) + R"(",)"; // 开发板类型
    board_json += R"("name":")" + std::string(BOARD_NAME) + R"(",)"; // 开发板名称

    // 如果不是配置模式，添加WiFi连接信息
    if (!wifi_config_mode_)
    {
        board_json += R"("ssid":")" + wifi_station.GetSsid() + R"(",)";                     // SSID
        board_json += R"("rssi":)" + std::to_string(wifi_station.GetRssi()) + R"(,)";       // RSSI
        board_json += R"("channel":)" + std::to_string(wifi_station.GetChannel()) + R"(,)"; // 信道
        board_json += R"("ip":")" + wifi_station.GetIpAddress() + R"(",)";                  // IP地址
    }

    // 添加MAC地址
    board_json += R"("mac":")" + SystemInfo::GetMacAddress() + R"(")";
    board_json += R"(})";

    return board_json;
}

/**
 * @brief 设置电源管理模式
 *
 * 启用或禁用WiFi模块的电源节省模式。
 *
 * @param enabled true表示启用电源节省模式，false表示禁用
 */
void WifiBoard::SetPowerSaveMode(bool enabled)
{
    // 获取WiFi站点实例并设置电源管理模式
    auto &wifi_station = WifiStation::GetInstance();
    wifi_station.SetPowerSaveMode(enabled);
}

/**
 * @brief 重置WiFi配置
 *
 * 清除已保存的WiFi网络配置信息，使设备重新进入配网模式。
 * 通过设置force_ap标志并重启设备实现。
 */
void WifiBoard::ResetWifiConfiguration()
{
    // 设置强制AP模式标志
    {
        Settings settings("wifi", true);
        settings.SetInt("force_ap", 1); // 设置标志位为1
    }

    // 显示进入WiFi配置模式提示
    GetDisplay()->ShowNotification(Lang::Strings::ENTERING_WIFI_CONFIG_MODE); // "进入配网模式..."

    // 等待1秒后重启设备
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 重启设备
    esp_restart();
}

/**
 * @brief 获取设备状态JSON信息
 *
 * 返回包含设备当前状态信息的JSON字符串，包括：
 * - 音频扬声器状态（音量）
 * - 屏幕状态（亮度、主题）
 * - 电池状态（电量、充电状态）
 * - 网络状态（类型、SSID、信号强度）
 * - 芯片状态（温度）
 *
 * @return 返回设备状态信息的JSON字符串
 */
std::string WifiBoard::GetDeviceStatusJson()
{
    /*
     * 返回设备状态JSON
     *
     * 返回的JSON结构如下：
     * {
     *     "audio_speaker": {
     *         "volume": 70
     *     },
     *     "screen": {
     *         "brightness": 100,
     *         "theme": "light"
     *     },
     *     "battery": {
     *         "level": 50,
     *         "charging": true
     *     },
     *     "network": {
     *         "type": "wifi",
     *         "ssid": "Xiaozhi",
     *         "rssi": -60
     *     },
     *     "chip": {
     *         "temperature": 25
     *     }
     * }
     */

    // 获取开发板实例
    auto &board = Board::GetInstance();

    // 创建JSON根对象
    auto root = cJSON_CreateObject();

    // 音频扬声器状态
    auto audio_speaker = cJSON_CreateObject();
    auto audio_codec = board.GetAudioCodec();
    if (audio_codec)
    {
        // 如果有音频编解码器，添加音量信息
        cJSON_AddNumberToObject(audio_speaker, "volume", audio_codec->output_volume());
    }
    cJSON_AddItemToObject(root, "audio_speaker", audio_speaker);

    // 屏幕状态
    auto backlight = board.GetBacklight();
    auto screen = cJSON_CreateObject();
    if (backlight)
    {
        // 添加屏幕亮度信息
        cJSON_AddNumberToObject(screen, "brightness", backlight->brightness());
    }
    auto display = board.GetDisplay();
    if (display && display->height() > 64)
    { // 仅对LCD显示屏
        // 添加屏幕主题信息
        auto theme = display->GetTheme();
        if (theme != nullptr)
        {
            cJSON_AddStringToObject(screen, "theme", theme->name().c_str());
        }
    }
    cJSON_AddItemToObject(root, "screen", screen);

    // 电池状态
    int battery_level = 0;
    bool charging = false;
    bool discharging = false;
    if (board.GetBatteryLevel(battery_level, charging, discharging))
    {
        // 如果支持电池检测，添加电池信息
        cJSON *battery = cJSON_CreateObject();
        cJSON_AddNumberToObject(battery, "level", battery_level); // 电量百分比
        cJSON_AddBoolToObject(battery, "charging", charging);     // 充电状态
        cJSON_AddItemToObject(root, "battery", battery);
    }

    // 网络状态
    auto network = cJSON_CreateObject();
    auto &wifi_station = WifiStation::GetInstance();
    cJSON_AddStringToObject(network, "type", "wifi");                         // 网络类型
    cJSON_AddStringToObject(network, "ssid", wifi_station.GetSsid().c_str()); // SSID

    // 根据RSSI值确定信号强度等级
    int rssi = wifi_station.GetRssi();
    if (rssi >= -60)
    {
        cJSON_AddStringToObject(network, "signal", "strong"); // 信号强
    }
    else if (rssi >= -70)
    {
        cJSON_AddStringToObject(network, "signal", "medium"); // 信号中等
    }
    else
    {
        cJSON_AddStringToObject(network, "signal", "weak"); // 信号弱
    }
    cJSON_AddItemToObject(root, "network", network);

    // 芯片状态
    float esp32temp = 0.0f;
    if (board.GetTemperature(esp32temp))
    {
        // 如果支持温度检测，添加温度信息
        auto chip = cJSON_CreateObject();
        cJSON_AddNumberToObject(chip, "temperature", esp32temp); // 芯片温度
        cJSON_AddItemToObject(root, "chip", chip);
    }

    // 将JSON对象转换为字符串并返回
    auto json_str = cJSON_PrintUnformatted(root);
    std::string json(json_str);
    cJSON_free(json_str); // 释放cJSON分配的内存
    cJSON_Delete(root);   // 删除JSON对象
    return json;
}