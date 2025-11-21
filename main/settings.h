#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <nvs_flash.h>

/**
 * @brief 设置管理类，用于管理非易失性存储(NVS)中的配置项
 *
 * 该类提供了对ESP32 NVS(Non-Volatile Storage)的封装，支持字符串、整数和布尔值类型的
 * 配置项读写操作。所有设置按命名空间进行组织，确保不同模块的配置项不会冲突。
 */
class Settings
{
public:
    /**
     * @brief 构造函数，初始化指定命名空间的设置
     *
     * @param ns 命名空间名称，用于区分不同模块的配置项
     * @param read_write 是否以读写模式打开，false表示只读模式
     */
    Settings(const std::string &ns, bool read_write = false);
    /**
     * @brief
     * 在C++中，const std::string& 表示一个常量引用（const reference），它有以下几个重要特征：
     * 引用（&）：引用是变量的别名，不创建新的对象副本，而是直接引用原始对象
     * 常量（const）：表示该引用不能被用来修改所引用的对象
     *
     * 让我详细解释一下在函数参数中使用 const std::string& 的原因和好处：
     * 1. 避免不必要的对象拷贝
     *  当使用 std::string 作为参数传递时（值传递），会创建一个完整的副本：
     * // 值传递 - 会创建副本（效率低）void SetString(std::string key, std::string value);
     * 而使用引用传递不会创建副本：
     * // 引用传递 - 不创建副本（效率高）void SetString(const std::string& key, const std::string& value);
     * 2. 防止意外修改
     *  使用 const 关键字确保函数内部不能修改传入的字符串：
     *  void SetString(const std::string& key, const std::string& value) {
     *      // key = "new_value";  // 编译错误！不能修改const引用
     *      // 这样保证了参数的安全性
     *  }
     * 3. 支持多种类型的参数
     *  使用 const std::string& 可以接受多种类型的参数：
     *  Settings settings("my_namespace", true);
     *
     * // 可以直接传字符串字面量
     * settings.SetString("device_id", "ABC123");
     *
     * // 可以传std::string对象
     * std::string key = "wifi_ssid";
     * std::string value = "MyNetwork";
     * settings.SetString(key, value);
     *
     * // 可以传const char*
     * const char* password = "secret";
     * settings.SetString("wifi_password", password);
     */

    /**
     * @brief 析构函数，自动提交更改并关闭NVS句柄
     *
     * 如果是以读写模式打开且有未提交的更改，会自动调用nvs_commit提交更改，
     * 然后调用nvs_close关闭NVS句柄。
     */
    ~Settings();

    /**
     * @brief 获取字符串类型的配置项
     *
     * @param key 配置项的键名
     * @param default_value 默认值，当键不存在或读取失败时返回
     * @return 返回配置项的字符串值或默认值
     */
    std::string GetString(const std::string &key, const std::string &default_value = "");

    /**
     * @brief 设置字符串类型的配置项
     *
     * @param key 配置项的键名
     * @param value 要设置的字符串值
     *
     * @note 仅在以读写模式打开时有效，否则仅记录警告日志
     */
    void SetString(const std::string &key, const std::string &value);

    /**
     * @brief 获取整数类型的配置项
     *
     * @param key 配置项的键名
     * @param default_value 默认值，当键不存在或读取失败时返回
     * @return 返回配置项的整数值或默认值
     */
    int32_t GetInt(const std::string &key, int32_t default_value = 0);

    /**
     * @brief 设置整数类型的配置项
     *
     * @param key 配置项的键名
     * @param value 要设置的整数值
     *
     * @note 仅在以读写模式打开时有效，否则仅记录警告日志
     */
    void SetInt(const std::string &key, int32_t value);

    /**
     * @brief 获取布尔类型的配置项
     *
     * @param key 配置项的键名
     * @param default_value 默认值，当键不存在或读取失败时返回
     * @return 返回配置项的布尔值或默认值
     */
    bool GetBool(const std::string &key, bool default_value = false);

    /**
     * @brief 设置布尔类型的配置项
     *
     * @param key 配置项的键名
     * @param value 要设置的布尔值
     *
     * @note 仅在以读写模式打开时有效，否则仅记录警告日志
     */
    void SetBool(const std::string &key, bool value);

    /**
     * @brief 删除指定的配置项
     *
     * @param key 要删除的配置项键名
     *
     * @note 仅在以读写模式打开时有效，否则仅记录警告日志
     */
    void EraseKey(const std::string &key);

    /**
     * @brief 删除命名空间中的所有配置项
     *
     * @note 仅在以读写模式打开时有效，否则仅记录警告日志
     */
    void EraseAll();

private:
    std::string ns_;              ///< 命名空间名称
    nvs_handle_t nvs_handle_ = 0; ///< NVS句柄，用于访问NVS存储
    bool read_write_ = false;     ///< 是否以读写模式打开
    bool dirty_ = false;          ///< 标记是否有未提交的更改
};

#endif