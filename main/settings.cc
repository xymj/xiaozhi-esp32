#include "settings.h"

#include <esp_log.h>
#include <nvs_flash.h>

#define TAG "Settings"

/**
 * @brief 构造函数，初始化指定命名空间的设置
 *
 * 打开指定命名空间的NVS存储，根据read_write参数决定是以只读还是读写模式打开。
 *
 * @param ns 命名空间名称
 * @param read_write 是否以读写模式打开
 */
Settings::Settings(const std::string &ns, bool read_write) : ns_(ns), read_write_(read_write)
{
    nvs_open(ns.c_str(), read_write_ ? NVS_READWRITE : NVS_READONLY, &nvs_handle_);
}

/**
 * @brief 析构函数，自动提交更改并关闭NVS句柄
 *
 * 如果是以读写模式打开且有未提交的更改，会自动调用nvs_commit提交更改，
 * 然后调用nvs_close关闭NVS句柄。
 */
Settings::~Settings()
{
    if (nvs_handle_ != 0)
    {
        if (read_write_ && dirty_)
        {
            ESP_ERROR_CHECK(nvs_commit(nvs_handle_));
        }
        nvs_close(nvs_handle_);
    }
}

/**
 * @brief 获取字符串类型的配置项
 *
 * 首先检查NVS句柄是否有效，然后通过两次调用nvs_get_str来获取字符串值：
 * 第一次调用获取所需缓冲区大小，第二次调用获取实际数据。
 *
 * @param key 配置项的键名
 * @param default_value 默认值
 * @return 返回配置项的字符串值或默认值
 */
std::string Settings::GetString(const std::string &key, const std::string &default_value)
{
    if (nvs_handle_ == 0)
    {
        return default_value;
    }

    size_t length = 0;
    // 第一次调用获取所需缓冲区大小
    if (nvs_get_str(nvs_handle_, key.c_str(), nullptr, &length) != ESP_OK)
    {
        return default_value;
    }

    std::string value;
    value.resize(length);
    // 第二次调用获取实际数据
    ESP_ERROR_CHECK(nvs_get_str(nvs_handle_, key.c_str(), value.data(), &length));
    // 移除可能存在的尾随空字符
    while (!value.empty() && value.back() == '\0')
    {
        value.pop_back();
    }
    return value;
}

/**
 * @brief 设置字符串类型的配置项
 *
 * @param key 配置项的键名
 * @param value 要设置的字符串值
 *
 * @note 仅在以读写模式打开时有效
 */
void Settings::SetString(const std::string &key, const std::string &value)
{
    if (read_write_)
    {
        ESP_ERROR_CHECK(nvs_set_str(nvs_handle_, key.c_str(), value.c_str()));
        dirty_ = true; // 标记有未提交的更改
    }
    else
    {
        ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
    }
}

/**
 * @brief 获取整数类型的配置项
 *
 * @param key 配置项的键名
 * @param default_value 默认值
 * @return 返回配置项的整数值或默认值
 */
int32_t Settings::GetInt(const std::string &key, int32_t default_value)
{
    if (nvs_handle_ == 0)
    {
        return default_value;
    }

    int32_t value;
    if (nvs_get_i32(nvs_handle_, key.c_str(), &value) != ESP_OK)
    {
        return default_value;
    }
    return value;
}

/**
 * @brief 设置整数类型的配置项
 *
 * @param key 配置项的键名
 * @param value 要设置的整数值
 *
 * @note 仅在以读写模式打开时有效
 */
void Settings::SetInt(const std::string &key, int32_t value)
{
    if (read_write_)
    {
        ESP_ERROR_CHECK(nvs_set_i32(nvs_handle_, key.c_str(), value));
        dirty_ = true; // 标记有未提交的更改
    }
    else
    {
        ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
    }
}

/**
 * @brief 获取布尔类型的配置项
 *
 * 在NVS中布尔值以uint8_t类型存储，0表示false，非0表示true。
 *
 * @param key 配置项的键名
 * @param default_value 默认值
 * @return 返回配置项的布尔值或默认值
 */
bool Settings::GetBool(const std::string &key, bool default_value)
{
    if (nvs_handle_ == 0)
    {
        return default_value;
    }

    uint8_t value;
    if (nvs_get_u8(nvs_handle_, key.c_str(), &value) != ESP_OK)
    {
        return default_value;
    }
    return value != 0;
}

/**
 * @brief 设置布尔类型的配置项
 *
 * 在NVS中布尔值以uint8_t类型存储，true存储为1，false存储为0。
 *
 * @param key 配置项的键名
 * @param value 要设置的布尔值
 *
 * @note 仅在以读写模式打开时有效
 */
void Settings::SetBool(const std::string &key, bool value)
{
    if (read_write_)
    {
        ESP_ERROR_CHECK(nvs_set_u8(nvs_handle_, key.c_str(), value ? 1 : 0));
        dirty_ = true; // 标记有未提交的更改
    }
    else
    {
        ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
    }
}

/**
 * @brief 删除指定的配置项
 *
 * @param key 要删除的配置项键名
 *
 * @note 仅在以读写模式打开时有效
 */
void Settings::EraseKey(const std::string &key)
{
    if (read_write_)
    {
        auto ret = nvs_erase_key(nvs_handle_, key.c_str());
        if (ret != ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_ERROR_CHECK(ret);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
    }
}

/**
 * @brief 删除命名空间中的所有配置项
 *
 * @note 仅在以读写模式打开时有效
 */
void Settings::EraseAll()
{
    if (read_write_)
    {
        ESP_ERROR_CHECK(nvs_erase_all(nvs_handle_));
    }
    else
    {
        ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
    }
}