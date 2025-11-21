#include "board.h"
#include "system_info.h"
#include "settings.h"
#include "display/display.h"
#include "display/oled_display.h"
#include "assets/lang_config.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_chip_info.h>
#include <esp_random.h>

#define TAG "Board"

/**
 * @brief Board构造函数
 *
 * 初始化Board对象，从NVS设置中读取或生成设备UUID。
 * 如果NVS中没有保存UUID，则生成一个新的UUID并保存到NVS中。
 */
Board::Board()
{
    // 创建settings对象用于访问NVS存储，"board"是命名空间，true表示读写模式
    Settings settings("board", true);

    // 尝试从NVS中获取已保存的UUID
    uuid_ = settings.GetString("uuid");

    // 如果UUID为空，说明是第一次启动设备，需要生成新的UUID
    if (uuid_.empty())
    {
        uuid_ = GenerateUuid();            // 生成新的UUID
        settings.SetString("uuid", uuid_); // 保存UUID到NVS
    }

    // 打印设备UUID和板子型号信息到日志
    ESP_LOGI(TAG, "UUID=%s SKU=%s", uuid_.c_str(), BOARD_NAME);
}

/**
 * @brief 生成UUID v4格式的唯一标识符
 *
 * 使用ESP32硬件随机数生成器创建符合UUID v4标准的唯一标识符。
 * UUID v4格式：xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 * 其中4表示版本号，y只能是8、9、A或B（表示变体）
 *
 * @return 返回生成的UUID字符串
 */
std::string Board::GenerateUuid()
{
    // UUID v4 需要 16 字节的随机数据
    uint8_t uuid[16];

    // 使用 ESP32 的硬件随机数生成器生成随机数据
    esp_fill_random(uuid, sizeof(uuid));

    // 设置版本 (版本 4) 和变体位
    uuid[6] = (uuid[6] & 0x0F) | 0x40; // 版本 4 (4位在第7字节的高4位)
    uuid[8] = (uuid[8] & 0x3F) | 0x80; // 变体 1 (2位在第9字节的高2位)

    // 将字节转换为标准的 UUID 字符串格式
    char uuid_str[37]; // UUID字符串长度为36字符+1个结束符
    snprintf(uuid_str, sizeof(uuid_str),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3],                          // 第1段：4个字节
             uuid[4], uuid[5],                                            // 第2段：2个字节
             uuid[6], uuid[7],                                            // 第3段：2个字节
             uuid[8], uuid[9],                                            // 第4段：2个字节
             uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]); // 第5段：6个字节

    return std::string(uuid_str);
}

/**
 * @brief 获取电池电量信息
 *
 * 默认实现返回false，表示该板子不支持电池电量检测。
 * 具体板子实现可以重写此函数以提供实际的电池信息。
 *
 * @param level 用于存储电池电量百分比的引用参数
 * @param charging 用于存储是否正在充电状态的引用参数
 * @param discharging 用于存储是否正在放电状态的引用参数
 * @return 成功获取电池信息返回true，否则返回false
 */
bool Board::GetBatteryLevel(int &level, bool &charging, bool &discharging)
{
    return false; // 默认不支持电池电量检测
}

/**
 * @brief 获取ESP32芯片温度
 *
 * 默认实现返回false，表示该板子不支持温度检测。
 * 具体板子实现可以重写此函数以提供实际的温度信息。
 *
 * @param esp32temp 用于存储温度值的引用参数
 * @return 成功获取温度返回true，否则返回false
 */
bool Board::GetTemperature(float &esp32temp)
{
    return false; // 默认不支持温度检测
}

/**
 * @brief 获取显示设备
 *
 * 默认实现返回NoDisplay对象，表示该板子没有显示设备。
 * 具体板子实现可以重写此函数以提供实际的显示设备。
 *
 * @return 返回指向Display对象的指针
 */
Display *Board::GetDisplay()
{
    static NoDisplay display; // 使用静态变量确保单例
    return &display;          // 返回无显示设备的实例
}

/**
 * @brief 获取摄像头设备
 *
 * 默认实现返回nullptr，表示该板子没有摄像头设备。
 * 具体板子实现可以重写此函数以提供实际的摄像头设备。
 *
 * @return 返回指向Camera对象的指针，如果无摄像头则返回nullptr
 */
Camera *Board::GetCamera()
{
    return nullptr; // 默认无摄像头设备
}

/**
 * @brief 获取LED控制器
 *
 * 默认实现返回NoLed对象，表示该板子没有LED设备。
 * 具体板子实现可以重写此函数以提供实际的LED控制器。
 *
 * @return 返回指向Led对象的指针
 */
Led *Board::GetLed()
{
    static NoLed led; // 使用静态变量确保单例
    return &led;      // 返回无LED设备的实例
}

/**
 * @brief 获取系统信息的JSON格式数据
 *
 * 构建包含设备完整系统信息的JSON字符串，包括：
 * - 版本信息
 * - Flash和PSRAM大小
 * - 最小空闲堆大小
 * - MAC地址
 * - UUID
 * - 芯片信息
 * - 应用程序信息
 * - 分区表信息
 * - OTA信息
 * - 显示信息
 * - 板子特定信息
 *
 * @return 返回包含系统信息的JSON字符串
 */
std::string Board::GetSystemInfoJson()
{
    /*
        JSON格式示例：
        {
            "version": 2,
            "flash_size": 4194304,
            "psram_size": 0,
            "minimum_free_heap_size": 123456,
            "mac_address": "00:00:00:00:00:00",
            "uuid": "00000000-0000-0000-0000-000000000000",
            "chip_model_name": "esp32s3",
            "chip_info": {
                "model": 1,
                "cores": 2,
                "revision": 0,
                "features": 0
            },
            "application": {
                "name": "my-app",
                "version": "1.0.0",
                "compile_time": "2021-01-01T00:00:00Z"
                "idf_version": "4.2-dev"
                "elf_sha256": ""
            },
            "partition_table": [
                "app": {
                    "label": "app",
                    "type": 1,
                    "subtype": 2,
                    "address": 0x10000,
                    "size": 0x100000
                }
            ],
            "ota": {
                "label": "ota_0"
            },
            "board": {
                ...
            }
        }
    */

    // R"(...)" 是原始字符串字面量（Raw String Literal）的语法,它允许您以一种更直观的方式定义包含特殊字符的字符串，而无需使用转义序列。基本语法是：R"delimiter(content)delimiter"
    //     其中：
    //      R 是原始字符串字面量的前缀
    //      delimiter 是可选的分隔符（可以是任意字符序列，但不能包含空格、括号、反斜杠或控制字符）
    //      content 是字符串的实际内容
    //      开始和结束的分隔符必须完全匹配
    // 为什么使用原始字符串字面量,使用原始字符串字面量的好处：
    //  避免转义字符：
    //      传统方式需要大量转义
    //      std::string json1 = "{\"version\":2,\"language\":\"" + std::string(Lang::CODE) + "\",";
    //      // 原始字符串方式更清晰
    //      std::string json2 = R"({"version":2,"language":")" + std::string(Lang::CODE) + R"(",)";
    //  提高可读性：
    //      包含正则表达式的字符串
    //      std::string regex1 = "\\d+\\.\\d+";  // 传统方式
    //      std::string regex2 = R"(\d+\.\d+)";  // 原始字符串方式

    //      // 包含文件路径的字符串
    //      std::string path1 = "C:\\Users\\Documents\\file.txt";  // 传统方式
    //      std::string path2 = R"(C:\Users\Documents\file.txt)";  // 原始字符串方式
    //  处理多行字符串：
    //     std::string multiline = R"(
    //      {
    //          "name": "example",
    //          "value": 42
    //      }
    //     )";

    //  开始构建JSON字符串，包含版本号和语言代码
    std::string json = R"({"version":2,"language":")" + std::string(Lang::CODE) + R"(",)";

    // 添加Flash大小信息
    json += R"("flash_size":)" + std::to_string(SystemInfo::GetFlashSize()) + R"(,)";

    // 添加最小空闲堆大小信息
    json += R"("minimum_free_heap_size":")" + std::to_string(SystemInfo::GetMinimumFreeHeapSize()) + R"(",)";

    // 添加MAC地址信息
    json += R"("mac_address":")" + SystemInfo::GetMacAddress() + R"(",)";

    // 添加UUID信息
    json += R"("uuid":")" + uuid_ + R"(",)";

    // 添加芯片型号名称
    json += R"("chip_model_name":")" + SystemInfo::GetChipModelName() + R"(",)";

    // 获取并添加芯片信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    json += R"("chip_info":{)";
    json += R"("model":)" + std::to_string(chip_info.model) + R"(,)";
    json += R"("cores":)" + std::to_string(chip_info.cores) + R"(,)";
    json += R"("revision":)" + std::to_string(chip_info.revision) + R"(,)";
    json += R"("features":)" + std::to_string(chip_info.features) + R"(},)";

    // 获取并添加应用程序信息
    auto app_desc = esp_app_get_description();
    json += R"("application":{)";
    json += R"("name":")" + std::string(app_desc->project_name) + R"(",)";
    json += R"("version":")" + std::string(app_desc->version) + R"(",)";
    json += R"("compile_time":")" + std::string(app_desc->date) + R"(T)" + std::string(app_desc->time) + R"(Z",)";
    json += R"("idf_version":")" + std::string(app_desc->idf_ver) + R"(",)";

    // 添加应用程序ELF文件的SHA256哈希值
    char sha256_str[65];
    for (int i = 0; i < 32; i++)
    {
        snprintf(sha256_str + i * 2, sizeof(sha256_str) - i * 2, "%02x", app_desc->app_elf_sha256[i]);
    }
    json += R"("elf_sha256":")" + std::string(sha256_str) + R"(")";
    json += R"(},)";

    // 添加分区表信息
    json += R"("partition_table": [)";
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it)
    {
        const esp_partition_t *partition = esp_partition_get(it);
        json += R"({)";
        json += R"("label":")" + std::string(partition->label) + R"(",)";
        json += R"("type":)" + std::to_string(partition->type) + R"(,)";
        json += R"("subtype":)" + std::to_string(partition->subtype) + R"(,)";
        json += R"("address":)" + std::to_string(partition->address) + R"(,)";
        json += R"("size":)" + std::to_string(partition->size) + R"(},)";
        ;
        it = esp_partition_next(it);
    }
    json.pop_back(); // 移除最后一个逗号
    json += R"(],)";

    // 添加OTA信息
    json += R"("ota":{)";
    auto ota_partition = esp_ota_get_running_partition();
    json += R"("label":")" + std::string(ota_partition->label) + R"(")";
    json += R"(},)";

    // 添加显示信息
    auto display = GetDisplay();
    if (display)
    {
        json += R"("display":{)";
        // 检查是否为OLED单色显示屏
        if (dynamic_cast<OledDisplay *>(display))
        {
            json += R"("monochrome":)" + std::string("true") + R"(,)";
        }
        else
        {
            json += R"("monochrome":)" + std::string("false") + R"(,)";
        }
        json += R"("width":)" + std::to_string(display->width()) + R"(,)";
        json += R"("height":)" + std::to_string(display->height()) + R"(,)";
        json.pop_back(); // 移除最后一个逗号
    }
    json += R"(},)";

    // 添加板子特定信息（由具体板子实现提供）
    json += R"("board":)" + GetBoardJson();

    // 关闭JSON对象
    json += R"(})";
    return json;
}

// 我来详细解释一下C++中`dynamic_cast`和`static_cast`的作用和区别。

// ### C++中的类型转换操作符

// 在C++中，有四种类型转换操作符：`static_cast`、`dynamic_cast`、`const_cast`和`reinterpret_cast`。这里我们重点讨论前两种。

// ### static_cast的作用

// `static_cast`是C++中最常用的类型转换操作符，它在编译时进行类型检查和转换。

// #### static_cast的主要用途：

// 1. **基本数据类型之间的转换**
// 2. **指针和引用的向上转换**（子类到父类）
// 3. **指针和引用的向下转换**（父类到子类，但不安全）
// 4. **void*指针与其他类型指针的转换**
// 5. **枚举类型与整型之间的转换**

// ```cpp
// // 基本数据类型转换
// double d = 3.14;
// int i = static_cast<int>(d);  // 3

// // 指针向上转换（安全）
// class Base {};
// class Derived : public Base {};

// Derived* d = new Derived();
// Base* b = static_cast<Base*>(d);  // 向上转换，安全

// // 指针向下转换（不安全，需要程序员确保正确性）
// Base* b2 = new Base();
// Derived* d2 = static_cast<Derived*>(b2);  // 编译通过，但运行时可能出错
// ```

// ### dynamic_cast的作用

// `dynamic_cast`是专门用于多态类型转换的操作符，它在运行时进行类型检查。

// #### dynamic_cast的主要用途：

// 1. **安全的向下转换**（父类到子类）
// 2. **交叉转换**（兄弟类之间）
// 3. **验证对象的实际类型**

// ```cpp
// class Base {
// public:
//     virtual void foo() {}  // 必须有虚函数才能使用dynamic_cast
// };

// class Derived1 : public Base {
// public:
//     void derived1_func() {}
// };

// class Derived2 : public Base {
// public:
//     void derived2_func() {}
// };

// // 安全的向下转换
// Base* base = new Derived1();

// // 使用static_cast（不安全）
// Derived1* d1_static = static_cast<Derived1*>(base);  // 编译通过，但不验证类型

// // 使用dynamic_cast（安全）
// Derived1* d1_dynamic = dynamic_cast<Derived1*>(base);  // 成功，返回有效指针
// Derived2* d2_dynamic = dynamic_cast<Derived2*>(base);  // 失败，返回nullptr

// if (d1_dynamic) {
//     d1_dynamic->derived1_func();  // 安全调用
// }

// if (d2_dynamic == nullptr) {
//     // 转换失败
// }
// ```

// ### static_cast和dynamic_cast的主要区别

// | 特性 | static_cast | dynamic_cast |
// |------|-------------|--------------|
// | **检查时机** | 编译时 | 运行时 |
// | **性能** | 高（无运行时开销） | 低（有运行时开销） |
// | **安全性** | 低（不验证运行时类型） | 高（验证运行时类型） |
// | **多态支持** | 不支持 | 支持 |
// | **虚函数要求** | 不需要 | 需要至少一个虚函数 |
// | **失败处理** | 未定义行为 | 指针返回nullptr，引用抛出异常 |

// ### 实际应用示例

// ```cpp
// #include <iostream>
// #include <typeinfo>

// class Animal {
// public:
//     virtual void make_sound() = 0;  // 纯虚函数，确保多态
//     virtual ~Animal() {}
// };

// class Dog : public Animal {
// public:
//     void make_sound() override {
//         std::cout << "Woof!" << std::endl;
//     }

//     void wag_tail() {
//         std::cout << "Wagging tail!" << std::endl;
//     }
// };

// class Cat : public Animal {
// public:
//     void make_sound() override {
//         std::cout << "Meow!" << std::endl;
//     }

//     void purr() {
//         std::cout << "Purring..." << std::endl;
//     }
// };

// void process_animal(Animal* animal) {
//     // 使用dynamic_cast安全地检查实际类型
//     if (Dog* dog = dynamic_cast<Dog*>(animal)) {
//         dog->make_sound();  // 调用虚函数
//         dog->wag_tail();    // 调用Dog特有函数
//     }
//     else if (Cat* cat = dynamic_cast<Cat*>(animal)) {
//         cat->make_sound();  // 调用虚函数
//         cat->purr();        // 调用Cat特有函数
//     }
//     else {
//         animal->make_sound();  // 通用处理
//     }

//     // 如果使用static_cast（不推荐）
//     // Dog* dog = static_cast<Dog*>(animal);  // 即使animal是Cat也会转换，危险！
// }

// int main() {
//     Animal* dog = new Dog();
//     Animal* cat = new Cat();

//     process_animal(dog);  // 正确识别为Dog
//     process_animal(cat);  // 正确识别为Cat

//     delete dog;
//     delete cat;

//     return 0;
// }
// ```

// ### 在您的代码中的可能应用

// 虽然在您提供的代码中没有直接使用`dynamic_cast`或`static_cast`，但在处理多态对象时可能会用到：

// ```cpp
// // 假设在其他文件中有类似这样的代码
// Display* display = GetDisplay();

// // 使用dynamic_cast检查具体类型
// if (OledDisplay* oled = dynamic_cast<OledDisplay*>(display)) {
//     // 这是一个OLED显示屏
//     // 可以调用OledDisplay特有的方法
// }
// else if (LcdDisplay* lcd = dynamic_cast<LcdDisplay*>(display)) {
//     // 这是一个LCD显示屏
//     // 可以调用LcdDisplay特有的方法
// }
// ```

// ### 总结

// 1. **static_cast**：
//    - 编译时转换，性能高
//    - 适用于已知类型关系的转换
//    - 不进行运行时类型检查，需要程序员确保安全性

// 2. **dynamic_cast**：
//    - 运行时转换，性能较低
//    - 专门用于多态类型的安全转换
//    - 进行运行时类型检查，提供安全保障
//    - 要求类具有虚函数

// 选择使用哪个转换操作符取决于您的具体需求：如果需要高性能且确定转换是安全的，使用`static_cast`；如果需要在运行时安全地处理多态类型，使用`dynamic_cast`。
