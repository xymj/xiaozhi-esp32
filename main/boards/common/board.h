#ifndef BOARD_H
#define BOARD_H

#include <http.h>
#include <web_socket.h>
#include <mqtt.h>
#include <udp.h>
#include <string>
#include <network_interface.h>

#include "led/led.h"
#include "backlight.h"
#include "camera.h"
#include "assets.h"

/**
 * @brief 创建板级实例的工厂函数
 * @return 返回指向Board实例的void指针
 */
void *create_board();

class AudioCodec;
class Display;

/**
 * @brief Board类定义了硬件板级接口的抽象基类
 *
 * Board类提供了对各种硬件组件的访问接口，包括LED、音频编解码器、显示、摄像头等。
 * 该类采用单例模式设计，通过GetInstance()方法获取实例。
 */
class Board
{
private:
    /**
     * @brief Board类禁用拷贝构造函数和赋值操作，保证单例模式创建的 Board 实例的唯一性
     *
     * 这是C++中继承机制的特性：
     * 继承了禁用声明：当一个类继承另一个类时，它会继承基类的所有成员，包括禁用的拷贝构造函数和赋值操作符声明。
     * 无法合成默认版本：由于基类(Board)已经显式删除了这些函数，编译器无法为派生类合成默认的拷贝构造函数和赋值操作符。
     * 编译时错误：如果尝试拷贝或赋值派生类的对象，编译器会报错，因为没有可用的拷贝构造函数或赋值操作符。
     */
    Board(const Board &) = delete;            // 禁用拷贝构造函数
    Board &operator=(const Board &) = delete; // 禁用赋值操作

    /**
     * @brief Protected成员的作用域
     * 类内部访问：类的成员函数可以访问自身的protected成员
     * 派生类访问：派生类（子类）可以访问基类的protected成员
     * 类外部禁止访问：类的外部代码不能直接访问protected成员，即类实例对象不可访问
     */
protected:
    /**
     * @brief Board构造函数
     * 初始化Board对象
     */
    Board();

    /**
     * @brief 生成设备唯一标识UUID
     * @return 返回生成的UUID字符串
     */
    std::string GenerateUuid();

    // 软件生成的设备唯一标识
    std::string uuid_;

public:
    /**
     * @brief 获取Board单例实例
     * 使用工厂模式创建具体的Board实例
     * @return 返回Board实例的引用
     */
    static Board &GetInstance()
    {
        static Board *instance = static_cast<Board *>(create_board());
        return *instance;
    }

    /**
     * @brief Board析构函数
     */
    virtual ~Board() = default;

    /**
     * @brief 获取板子类型
     * @return 返回板子类型的字符串描述
     */
    virtual std::string GetBoardType() = 0;

    /**
     * @brief 获取设备UUID
     * @return 返回设备的唯一标识符
     */
    virtual std::string GetUuid() { return uuid_; }

    /**
     * @brief 获取背光控制器
     * @return 返回指向Backlight对象的指针，如果无背光则返回nullptr
     */
    virtual Backlight *GetBacklight() { return nullptr; }

    /**
     * @brief 获取LED控制器
     * @return 返回指向Led对象的指针
     */
    virtual Led *GetLed();

    /**
     * @brief 获取音频编解码器
     * @return 返回指向AudioCodec对象的指针
     */
    virtual AudioCodec *GetAudioCodec() = 0;

    /**
     * @brief 获取ESP32芯片温度
     * @param esp32temp 用于存储温度值的引用参数
     * @return 成功获取温度返回true，否则返回false
     */
    virtual bool GetTemperature(float &esp32temp);

    /**
     * @brief 获取显示设备
     * @return 返回指向Display对象的指针
     */
    virtual Display *GetDisplay();

    /**
     * @brief 获取摄像头设备
     * @return 返回指向Camera对象的指针
     */
    virtual Camera *GetCamera();

    /**
     * @brief 获取网络接口
     * @return 返回指向NetworkInterface对象的指针
     */
    virtual NetworkInterface *GetNetwork() = 0;

    /**
     * @brief 启动网络连接
     */
    virtual void StartNetwork() = 0;

    /**
     * @brief 获取网络状态图标
     * @return 返回表示网络状态的图标字符串
     */
    virtual const char *GetNetworkStateIcon() = 0;

    /**
     * @brief 获取电池电量信息
     * @param level 用于存储电池电量百分比的引用参数
     * @param charging 用于存储是否正在充电状态的引用参数
     * @param discharging 用于存储是否正在放电状态的引用参数
     * @return 成功获取电池信息返回true，否则返回false
     */
    virtual bool GetBatteryLevel(int &level, bool &charging, bool &discharging);

    /**
     * @brief 获取系统信息的JSON格式数据
     * @return 返回包含系统信息的JSON字符串
     */
    virtual std::string GetSystemInfoJson();

    /**
     * @brief 设置省电模式
     * @param enabled true表示启用省电模式，false表示禁用
     */
    virtual void SetPowerSaveMode(bool enabled) = 0;

    /**
     * @brief 获取板子信息的JSON格式数据
     * @return 返回包含板子信息的JSON字符串
     */
    virtual std::string GetBoardJson() = 0;

    /**
     * @brief 获取设备状态的JSON格式数据
     * @return 返回包含设备状态信息的JSON字符串
     */
    virtual std::string GetDeviceStatusJson() = 0;
};

/**
 * @brief 声明板级类的宏定义
 * 用于在具体板级实现中声明create_board工厂函数
 * @param BOARD_CLASS_NAME 具体的板级类名
 */
#define DECLARE_BOARD(BOARD_CLASS_NAME) \
    void *create_board()                \
    {                                   \
        return new BOARD_CLASS_NAME();  \
    }

#endif // BOARD_H