#ifndef _SYSTEM_INFO_H_
#define _SYSTEM_INFO_H_

#include <string>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>

/**
 * @brief 系统信息工具类
 *
 * SystemInfo类提供了一系列静态方法，用于获取ESP32系统的各种信息，
 * 包括Flash存储、堆内存、MAC地址、芯片型号等硬件和系统状态信息。
 * 同时还提供系统诊断功能，如任务CPU使用率、任务列表和堆内存统计信息。
 */
class SystemInfo
{
public:
    /**
     * @brief 获取Flash存储大小
     *
     * 查询ESP32芯片的Flash存储总容量，以字节为单位。
     *
     * @return 返回Flash存储大小（字节）
     */
    static size_t GetFlashSize();

    /**
     * @brief 获取最小空闲堆大小
     *
     * 获取系统启动以来的最小空闲堆内存大小，用于监控内存使用情况和检测内存泄漏。
     * 这个值反映了系统运行过程中堆内存使用的峰值情况。
     *
     * @return 返回最小空闲堆大小（字节）
     */
    static size_t GetMinimumFreeHeapSize();

    /**
     * @brief 获取当前空闲堆大小
     *
     * 获取当前系统中可用的空闲堆内存大小，实时反映系统内存使用状况。
     *
     * @return 返回当前空闲堆大小（字节）
     */
    static size_t GetFreeHeapSize();

    /**
     * @brief 获取设备MAC地址
     *
     * 获取ESP32设备的WiFi接口MAC地址，格式为"xx:xx:xx:xx:xx:xx"。
     * MAC地址是设备的唯一硬件标识符。
     *
     * @return 返回MAC地址字符串
     */
    static std::string GetMacAddress();

    /**
     * @brief 获取芯片型号名称
     *
     * 获取ESP32芯片的型号名称，如"esp32"、"esp32s3"等。
     * 用于识别当前运行的硬件平台。
     *
     * @return 返回芯片型号名称字符串
     */
    static std::string GetChipModelName();

    /**
     * @brief 获取User-Agent字符串
     *
     * 生成HTTP请求使用的User-Agent标识字符串，包含芯片型号、SDK版本等信息。
     * 用于向服务器标识客户端设备和软件版本。
     *
     * @return 返回User-Agent字符串
     */
    static std::string GetUserAgent();

    /**
     * @brief 打印任务CPU使用率
     *
     * 统计并打印各个FreeRTOS任务的CPU使用率信息。
     * 通过在指定时间间隔内采样任务运行时间来计算使用率。
     *
     * @param xTicksToWait 统计采样时间间隔（RTOS滴答数）
     * @return ESP_OK表示成功，其他值表示错误
     */
    static esp_err_t PrintTaskCpuUsage(TickType_t xTicksToWait);

    /**
     * @brief 打印任务列表
     *
     * 打印系统中所有FreeRTOS任务的状态信息，包括：
     * - 任务名称
     * - 任务状态（运行、就绪、阻塞、挂起、删除）
     * - 任务优先级
     * - 剩余栈空间
     * - 任务编号
     */
    static void PrintTaskList();

    /**
     * @brief 打印堆内存统计信息
     *
     * 打印详细的堆内存使用统计信息，包括：
     * - 总堆大小
     * - 当前空闲内存
     * - 最小空闲内存
     * - 内部SRAM堆信息
     * - PSRAM堆信息（如果可用）
     */
    static void PrintHeapStats();
};

#endif // _SYSTEM_INFO_H_