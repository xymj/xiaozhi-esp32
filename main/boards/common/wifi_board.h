#ifndef WIFI_BOARD_H
#define WIFI_BOARD_H

#include "board.h"

/**
 * @brief WiFi功能开发板基类
 *
 * WifiBoard类继承自Board基类，为支持WiFi功能的开发板提供通用接口和实现。
 * 该类封装了WiFi网络配置、连接管理、电源管理等核心功能。
 * 作为抽象基类，具体的开发板实现需要继承此类并实现相关虚函数。
 */
class WifiBoard : public Board
{
protected:
    // WiFi配置模式标志，true表示设备处于WiFi配置模式（配网模式）
    bool wifi_config_mode_ = false;

    /**
     * @brief 进入WiFi配置模式
     *
     * 当设备需要进行WiFi网络配置时调用此函数，通常在首次启动或WiFi连接失败时触发。
     * 在配置模式下，设备会创建WiFi热点供用户连接并输入网络凭证。
     */
    void EnterWifiConfigMode();

    /**
     * @brief 获取开发板特定的JSON配置信息
     *
     * 重写基类的虚函数，返回当前开发板特有的配置信息JSON字符串。
     * 包含WiFi相关的配置参数和状态信息。
     *
     * @return 返回开发板特定配置信息的JSON字符串
     */
    virtual std::string GetBoardJson() override;

public:
    /**
     * @brief WifiBoard构造函数
     *
     * 初始化WiFi开发板对象，设置默认状态和参数。
     */
    WifiBoard();

    /**
     * @brief 获取开发板类型标识
     *
     * 重写基类的虚函数，返回当前开发板的类型标识字符串。
     *
     * @return 返回开发板类型字符串
     */
    virtual std::string GetBoardType() override;

    /**
     * @brief 启动网络连接
     *
     * 初始化并启动WiFi网络连接功能，包括WiFi驱动初始化、
     * 网络参数配置和连接建立等操作。
     */
    virtual void StartNetwork() override;

    /**
     * @brief 获取网络接口对象
     *
     * 重写基类的虚函数，返回当前开发板的网络接口实例。
     * 用于进行网络通信和数据传输操作。
     *
     * @return 返回网络接口对象指针
     */
    virtual NetworkInterface *GetNetwork() override;

    /**
     * @brief 获取网络状态图标标识
     *
     * 重写基类的虚函数，返回表示当前网络连接状态的图标标识。
     * 用于在显示屏上显示网络连接状态。
     *
     * @return 返回网络状态图标字符串
     */
    virtual const char *GetNetworkStateIcon() override;

    /**
     * @brief 设置电源管理模式
     *
     * 重写基类的虚函数，启用或禁用WiFi模块的电源节省模式。
     * 在电源节省模式下，WiFi模块会降低功耗但可能影响响应速度。
     *
     * @param enabled true表示启用电源节省模式，false表示禁用
     */
    virtual void SetPowerSaveMode(bool enabled) override;

    /**
     * @brief 重置WiFi配置
     *
     * 清除已保存的WiFi网络配置信息，使设备重新进入配网模式。
     * 通常在用户需要重新配置网络或网络配置错误时调用。
     */
    virtual void ResetWifiConfiguration();

    /**
     * @brief 获取音频编解码器对象
     *
     * 重写基类的虚函数，返回当前开发板的音频编解码器实例。
     * 对于纯WiFi功能开发板，通常不包含音频硬件，因此返回nullptr。
     *
     * @return 返回音频编解码器对象指针，若不支持则返回nullptr
     */
    virtual AudioCodec *GetAudioCodec() override { return nullptr; }

    /**
     * @brief 获取设备状态JSON信息
     *
     * 重写基类的虚函数，返回包含设备当前状态信息的JSON字符串。
     * 包括WiFi连接状态、IP地址、信号强度等网络相关信息。
     *
     * @return 返回设备状态信息的JSON字符串
     */
    virtual std::string GetDeviceStatusJson() override;
};

#endif // WIFI_BOARD_H