#include "system_info.h"

#include <freertos/task.h>
#include <esp_log.h>
#include <esp_flash.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <esp_app_desc.h>
#include <esp_ota_ops.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "esp_wifi_remote.h"
#endif

#define TAG "SystemInfo"

/**
 * @brief 获取Flash存储大小
 *
 * 查询ESP32芯片的Flash存储总容量，通过调用ESP-IDF的esp_flash_get_size函数实现。
 * 如果获取失败，会记录错误日志并返回0。
 *
 * @return 返回Flash存储大小（字节），失败时返回0
 */
size_t SystemInfo::GetFlashSize()
{
    uint32_t flash_size; // 存储Flash大小的变量

    // 调用ESP-IDF API获取Flash大小，NULL表示使用默认Flash芯片
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get flash size"); // 记录错误日志
        return 0;                                  // 获取失败返回0
    }

    // 将uint32_t类型转换为size_t并返回
    return (size_t)flash_size;
}

/**
 * @brief 获取最小空闲堆大小
 *
 * 获取系统启动以来的最小空闲堆内存大小，用于监控内存使用情况和检测内存泄漏。
 * 直接调用ESP-IDF的esp_get_minimum_free_heap_size函数实现。
 *
 * @return 返回最小空闲堆大小（字节）
 */
size_t SystemInfo::GetMinimumFreeHeapSize()
{
    // 直接调用ESP-IDF API获取最小空闲堆大小
    return esp_get_minimum_free_heap_size();
}

/**
 * @brief 获取当前空闲堆大小
 *
 * 获取当前系统中可用的空闲堆内存大小，实时反映系统内存使用状况。
 * 直接调用ESP-IDF的esp_get_free_heap_size函数实现。
 *
 * @return 返回当前空闲堆大小（字节）
 */
size_t SystemInfo::GetFreeHeapSize()
{
    // 直接调用ESP-IDF API获取当前空闲堆大小
    return esp_get_free_heap_size();
}

/**
 * @brief 获取设备MAC地址
 *
 * 获取ESP32设备的WiFi接口MAC地址，格式为"xx:xx:xx:xx:xx:xx"。
 * 根据不同的芯片平台使用相应的API获取MAC地址。
 *
 * @return 返回MAC地址字符串
 */
std::string SystemInfo::GetMacAddress()
{
    uint8_t mac[6]; // 存储MAC地址的字节数组

    // 根据芯片类型选择不同的MAC地址获取方式
#if CONFIG_IDF_TARGET_ESP32P4
    // ESP32P4平台使用WiFi远程接口获取MAC地址
    esp_wifi_get_mac(WIFI_IF_STA, mac);
#else
    // 其他平台使用标准接口获取WiFi STA接口的MAC地址
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
#endif

    // 将MAC地址字节格式化为字符串（xx:xx:xx:xx:xx:xx格式）
    char mac_str[18]; // MAC地址字符串缓冲区（17字符+1结束符）
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // 转换为std::string并返回
    return std::string(mac_str);
}

/**
 * @brief 获取芯片型号名称
 *
 * 获取ESP32芯片的型号名称，如"esp32"、"esp32s3"等。
 * 通过读取编译时配置的CONFIG_IDF_TARGET宏定义实现。
 *
 * @return 返回芯片型号名称字符串
 */
std::string SystemInfo::GetChipModelName()
{
    // 直接返回编译时配置的芯片型号名称
    return std::string(CONFIG_IDF_TARGET);
}

/**
 * @brief 获取User-Agent字符串
 *
 * 生成HTTP请求使用的User-Agent标识字符串，包含板子名称和应用程序版本信息。
 * 用于向服务器标识客户端设备和软件版本。
 *
 * @return 返回User-Agent字符串
 */
std::string SystemInfo::GetUserAgent()
{
    // 获取应用程序描述信息（包含版本等信息）
    auto app_desc = esp_app_get_description();

    // 构造User-Agent字符串：板子名称/应用程序版本
    auto user_agent = std::string(BOARD_NAME "/") + app_desc->version;

    return user_agent;
}

/**
 * @brief 打印任务CPU使用率
 *
 * 统计并打印各个FreeRTOS任务的CPU使用率信息。
 * 通过在指定时间间隔内采样任务运行时间来计算使用率。
 *
 * @param xTicksToWait 统计采样时间间隔（RTOS滴答数）
 * @return ESP_OK表示成功，其他值表示错误
 */
esp_err_t SystemInfo::PrintTaskCpuUsage(TickType_t xTicksToWait)
{
#define ARRAY_SIZE_OFFSET 5 // 数组大小偏移量，用于避免数组大小不足

    // 任务状态结构体指针，用于存储任务状态信息
    TaskStatus_t *start_array = NULL, *end_array = NULL;

    // 数组大小变量
    UBaseType_t start_array_size, end_array_size;

    // 运行时间计数器，用于记录采样开始和结束时的系统运行时间
    configRUN_TIME_COUNTER_TYPE start_run_time, end_run_time;

    esp_err_t ret;               // 函数返回值
    uint32_t total_elapsed_time; // 总的采样时间间隔

    // 分配数组以存储当前任务状态
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;               // 获取当前任务数并增加偏移量
    start_array = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * start_array_size); // 分配内存
    if (start_array == NULL)
    {
        ret = ESP_ERR_NO_MEM; // 内存分配失败
        goto exit;            // 跳转到退出处理
    }

    // 获取当前任务状态
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0)
    {
        ret = ESP_ERR_INVALID_SIZE; // 获取任务状态失败
        goto exit;
    }

    // 延迟指定时间间隔，用于采样
    vTaskDelay(xTicksToWait);

    // 分配数组以存储延迟后的任务状态
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = (TaskStatus_t *)malloc(sizeof(TaskStatus_t) * end_array_size);
    if (end_array == NULL)
    {
        ret = ESP_ERR_NO_MEM; // 内存分配失败
        goto exit;
    }

    // 获取延迟后的任务状态
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0)
    {
        ret = ESP_ERR_INVALID_SIZE; // 获取任务状态失败
        goto exit;
    }

    // 计算总的采样时间间隔（运行时间统计时钟周期单位）
    total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0)
    {
        ret = ESP_ERR_INVALID_STATE; // 无效状态
        goto exit;
    }

    // 打印表头
    printf("| Task | Run Time | Percentage\n");

    // 匹配开始和结束时的任务状态，计算每个任务的CPU使用率
    for (int i = 0; i < start_array_size; i++)
    {
        int k = -1; // 用于存储匹配到的任务索引

        // 在结束数组中查找与开始数组中任务匹配的项
        for (int j = 0; j < end_array_size; j++)
        {
            if (start_array[i].xHandle == end_array[j].xHandle)
            {
                k = j;
                // 标记任务已匹配，通过覆盖句柄实现
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }

        // 检查是否找到匹配的任务
        if (k >= 0)
        {
            // 计算任务在采样期间的运行时间
            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;

            // 计算任务CPU使用百分比（考虑多核情况）
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * CONFIG_FREERTOS_NUMBER_OF_CORES);

            // 打印任务名称、运行时间和使用百分比
            printf("| %-16s | %8lu | %4lu%%\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time);
        }
    }

    // 打印在采样期间被删除的任务
    for (int i = 0; i < start_array_size; i++)
    {
        if (start_array[i].xHandle != NULL)
        {
            printf("| %s | Deleted\n", start_array[i].pcTaskName);
        }
    }

    // 打印在采样期间创建的任务
    for (int i = 0; i < end_array_size; i++)
    {
        if (end_array[i].xHandle != NULL)
        {
            printf("| %s | Created\n", end_array[i].pcTaskName);
        }
    }

    ret = ESP_OK; // 设置成功返回值

exit: // 通用返回路径
    // 释放分配的内存
    free(start_array);
    free(end_array);

    return ret; // 返回结果
}

/**
 * @brief 打印任务列表
 *
 * 打印系统中所有FreeRTOS任务的状态信息，包括任务名称、状态、优先级、剩余栈空间等。
 * 通过调用FreeRTOS的vTaskList函数实现。
 */
void SystemInfo::PrintTaskList()
{
    char buffer[1000]; // 用于存储任务列表信息的缓冲区

    // 调用FreeRTOS API获取任务列表信息
    vTaskList(buffer);

    // 打印任务列表信息到日志
    ESP_LOGI(TAG, "Task list: \n%s", buffer);
}

/**
 * @brief 打印堆内存统计信息
 *
 * 打印详细的堆内存使用统计信息，包括当前空闲内存和最小空闲内存。
 * 重点关注内部SRAM堆的使用情况。
 */
void SystemInfo::PrintHeapStats()
{
    // 获取内部SRAM堆的当前空闲大小
    int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    // 获取内部SRAM堆的最小空闲大小
    int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);

    // 打印堆内存统计信息到日志
    ESP_LOGI(TAG, "free sram: %u minimal sram: %u", free_sram, min_free_sram);
}