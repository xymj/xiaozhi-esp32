#ifndef DISPLAY_H
#define DISPLAY_H

#include "emoji_collection.h"

#ifndef CONFIG_USE_EMOTE_MESSAGE_STYLE
#define HAVE_LVGL 1
#include <lvgl.h>
#endif

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_pm.h>

#include <string>
#include <chrono>

/**
 * @brief 主题类，用于管理显示界面的主题样式
 *
 * Theme类是一个简单的主题管理类，存储主题的名称信息。
 * 可以通过继承此类来实现更复杂主题功能。
 */
class Theme
{
public:
    /**
     * @brief 构造函数，初始化主题名称
     *
     * @param name 主题名称
     */
    Theme(const std::string &name) : name_(name) {}

    /**
     * @brief 虚析构函数，确保派生类能正确析构
     */
    virtual ~Theme() = default;

    /**
     * @brief 获取主题名称
     *
     * @return 返回主题名称的常量引用
     *
     * 1. 内联展开建议
     *      inline 关键字最主要的作用是向编译器建议将函数调用替换为函数体本身，避免函数调用的开销：
     * // 不使用inline - 函数调用有开销
     *      std::string name() const { return name_; }
     * // 使用inline - 编译器可能直接展开代码
     *      inline std::string name() const { return name_; }
     * 当调用内联函数时：
     *      Theme theme("dark");
     *      std::string n = theme.name(); // 可能直接替换为: std::string n = theme.name_;
     *
     * 2. 解决多重定义问题
     *     在头文件中定义函数时，如果函数在多个编译单元中被包含，会导致链接错误。使用 inline 可以解决这个问题：
     * // theme.h - 如果不使用inline，多个.cpp文件包含此头文件时会链接错误
     * class Theme {
     *  public:
     *      // inline确保即使在多个编译单元中定义也不会出错
     *      inline std::string name() const { return name_; }
     * private:
     *       std::string name_;
     * };
     *
     *3. 在类内部定义的函数默认为inline
     *     需要注意的是，在类内部定义的函数（包括在类声明中直接实现的函数）默认就是内联的：
     * class Theme {
     * public:
     * // 这两个函数都是内联的
     * std::string name() const { return name_; }           // 隐式inline
     * inline std::string getName() const { return name_; } // 显式inline
     * private:
     *      std::string name_;
     * };
     *
     *4. 编译器决策
     *     现代编译器会根据以下因素决定是否真正内联函数：
     *      函数体的复杂程度
     *      函数的大小
     *      调用频率
     *      优化级别
     * // 简单的getter函数，编译器很可能会内联
     * inline std::string name() const { return name_; }
     * // 复杂的函数，编译器可能忽略inline建议
     *  inline void complexFunction() {
     *      // 大量复杂代码...
     *  for (int i = 0; i < 1000000; i++) {
     *      // 复杂计算...
     *  }
     * }
     *
     *
     * 5. 性能考虑
     *  对于像 name() 这样简单的getter函数，使用 inline 是合适的：
     *
     *
     * 6. 与const结合使用
     * 在您的代码中，inline std::string name() const 结合了三个特性：
     * inline: 建议内联展开
     * const: 表示该函数不会修改对象状态
     * 返回类型和函数名: 标准的getter函数
     */
    // 完整的函数声明含义：
    // inline - 建议内联
    // std::string - 返回类型
    // name() - 函数名
    // const - 常量成员函数，不修改对象状态
    inline std::string name() const { return name_; }

private:
    std::string name_; ///< 主题名称
};

/**
 * @brief 显示设备抽象基类
 *
 * Display类定义了显示设备的通用接口，包括状态显示、通知、表情、聊天消息等功能。
 * 该类使用了模板方法模式，具体的显示实现由派生类完成。
 *
 * 显示设备的宽度和高度通过width()和height()方法获取。
 * 该类还提供了线程安全的显示锁定机制，通过DisplayLockGuard实现。
 */
class Display
{
public:
    /**
     * @brief 构造函数，初始化显示设备
     */
    Display();

    /**
     * @brief 虚析构函数，确保派生类能正确析构
     */
    virtual ~Display();

    /**
     * @brief 设置设备状态信息
     *
     * 在显示屏上显示设备当前状态信息，如"正在连接WiFi"、"已连接"等。
     *
     * @param status 状态信息字符串
     */
    virtual void SetStatus(const char *status);

    /**
     * @brief 显示通知消息（C风格字符串版本）
     *
     * 在显示屏上临时显示一条通知消息，指定显示持续时间。
     *
     * @param notification 通知消息内容
     * @param duration_ms 显示持续时间（毫秒），默认3000ms
     */
    virtual void ShowNotification(const char *notification, int duration_ms = 3000);

    /**
     * @brief 显示通知消息（std::string版本）
     *
     * 在显示屏上临时显示一条通知消息，指定显示持续时间。
     *
     * @param notification 通知消息内容
     * @param duration_ms 显示持续时间（毫秒），默认3000ms
     */
    virtual void ShowNotification(const std::string &notification, int duration_ms = 3000);

    /**
     * @brief 设置显示表情
     *
     * 在显示屏上显示指定的表情图标或动画。
     *
     * @param emotion 表情标识符字符串
     */
    virtual void SetEmotion(const char *emotion);

    /**
     * @brief 设置聊天消息显示
     *
     * 在显示屏上显示聊天对话内容，区分发送者角色（用户或AI）。
     *
     * @param role 发送者角色，如"user"或"assistant"
     * @param content 消息内容
     */
    virtual void SetChatMessage(const char *role, const char *content);

    /**
     * @brief 设置显示主题
     *
     * 应用指定的主题样式到显示界面。
     *
     * @param theme 指向Theme对象的指针
     */
    virtual void SetTheme(Theme *theme);

    /**
     * @brief 获取当前主题
     *
     * @return 返回当前应用的主题对象指针
     */
    virtual Theme *GetTheme() { return current_theme_; }

    /**
     * @brief 更新状态栏显示
     *
     * 刷新显示屏顶部状态栏的信息显示。
     *
     * @param update_all 是否更新所有状态信息，false表示只更新变化部分
     */
    virtual void UpdateStatusBar(bool update_all = false);

    /**
     * @brief 设置省电模式
     *
     * 启用或禁用显示屏的省电模式，可能包括降低亮度、关闭背光等操作。
     *
     * @param on true表示启用省电模式，false表示禁用
     */
    virtual void SetPowerSaveMode(bool on);

    /**
     * @brief 获取显示设备宽度
     *
     * @return 返回显示设备的宽度（像素）
     */
    inline int width() const { return width_; }

    /**
     * @brief 获取显示设备高度
     *
     * @return 返回显示设备的高度（像素）
     */
    inline int height() const { return height_; }

protected:
    // DisplayLockGuard可以访问这些受保护成员
    int width_ = 0;                  ///< 显示设备宽度（像素）
    int height_ = 0;                 ///< 显示设备高度（像素）
    Theme *current_theme_ = nullptr; ///< 当前应用的主题

    // friend关键字用于声明友元关系，它允许一个类授予其他类或函数访问其私有成员和受保护成员的权限
    // Friend关键字的功能和作用
    // 1. 打破封装性限制
    // 正常情况下，类的私有成员和受保护成员只能被类的成员函数访问。friend关键字打破了这一限制：
    //     class MyClass {
    // private:
    //     int privateData = 42;

    //     // 友元函数可以访问私有成员
    //     friend void friendFunction(MyClass& obj);

    //     // 友元类可以访问私有成员
    //     friend class FriendClass;
    // };

    // // 友元函数可以直接访问MyClass的私有成员
    // void friendFunction(MyClass& obj) {
    //     std::cout << obj.privateData << std::endl; // 合法访问
    // }

    // class FriendClass {
    // public:
    //     void accessPrivateData(MyClass& obj) {
    //         std::cout << obj.privateData << std::endl; // 合法访问
    //     }
    // };

    // 3. 友元的类型
    // C++中可以声明三种类型的友元：
    //     class MyClass {
    // private:
    //     int data;

    // public:
    //     // 1. 友元函数
    //     friend void friendFunction(MyClass& obj);

    //     // 2. 友元类
    //     friend class FriendClass;

    //     // 3. 友元成员函数
    //     friend void AnotherClass::friendMemberFunction(MyClass& obj);
    // };

    // // 友元函数实现
    // void friendFunction(MyClass& obj) {
    //     obj.data = 100; // 可以访问私有成员
    // }

    // 4. 友元的特点
    //  单向性：如果类A是类B的友元，类B不一定是类A的友元
    //  非传递性：如果类A是类B的友元，类B是类C的友元，类A不一定是类C的友元
    //  非继承性：友元关系不能被继承
    //  class Base {
    //      friend class FriendClass; // FriendClass是Base的友元
    //  private:
    //      int baseData;
    //  };

    // class Derived : public Base {
    // private:
    //     int derivedData;
    // };

    // class FriendClass {
    // public:
    //     void accessData(Base& base) {
    //         base.baseData = 10; // 可以访问
    //     }

    //     void accessData(Derived& derived) {
    //         // derived.baseData = 10;    // 可以访问继承的基类成员
    //         // derived.derivedData = 20; // 不能访问派生类的私有成员
    //     }
    // };

    // 6. 使用友元的优缺点
    // 优点：

    // 提供必要的访问权限，实现特定功能（如RAII模式）
    // 保持接口简洁，避免暴露不必要的公共接口
    // 实现高效的访问，避免通过公共接口的开销
    // 缺点：

    // 破坏封装性，增加类之间的耦合
    // 降低代码的可维护性
    // 可能影响类的安全性
    // 7. 最佳实践
    // 谨慎使用：只在确实需要时才使用友元
    // 最小权限原则：只授予必要的访问权限
    // 文档说明：清楚地说明为什么需要友元关系

    //  声明友元类，允许DisplayLockGuard访问私有成员
    friend class DisplayLockGuard;

    // DisplayLockGuard可以访问这些私有成员函数（如果有的话）
    /**
     * @brief 锁定显示设备（纯虚函数）
     *
     * 用于多线程环境下保护显示设备访问的同步机制。
     * 派生类需要实现具体的锁定机制。
     *
     * @param timeout_ms 锁定超时时间（毫秒），0表示无限等待
     * @return 成功获取锁返回true，超时返回false
     */
    virtual bool Lock(int timeout_ms = 0) = 0;

    /**
     * @brief 解锁显示设备（纯虚函数）
     *
     * 释放显示设备的锁定，与Lock方法配对使用。
     * 派生类需要实现具体的解锁机制。
     */
    virtual void Unlock() = 0;
};

/**
 * @brief 显示设备锁定保护类
 *
 * DisplayLockGuard实现了RAII（Resource Acquisition Is Initialization）模式，
 * 用于自动管理显示设备的锁定和解锁，确保在作用域结束时自动释放锁。
 *
 * 使用示例：
 * @code
 * {
 *     DisplayLockGuard lock(display);
 *     // 在此作用域内安全地访问显示设备
 *     display->SetStatus("Processing...");
 *     // 作用域结束时自动解锁
 * }
 * @endcode
 */
class DisplayLockGuard
{
public:
    /**
     * @brief 构造函数，自动锁定显示设备
     *
     * 创建DisplayLockGuard对象时会自动尝试锁定指定的显示设备。
     * 如果锁定失败会记录错误日志。
     *
     * @param display 指向Display对象的指针
     */
    DisplayLockGuard(Display *display) : display_(display)
    {
        // 可以访问Display的受保护成员函数Lock
        if (!display_->Lock(30000))
        { // 等待30秒超时
            ESP_LOGE("Display", "Failed to lock display");
        }
    }

    /**
     * @brief 析构函数，自动解锁显示设备
     *
     * DisplayLockGuard对象销毁时会自动解锁显示设备。
     */
    ~DisplayLockGuard()
    {
        // 可以访问Display的受保护成员函数Unlock
        display_->Unlock();
    }

private:
    Display *display_; ///< 指向被保护的Display对象的指针
};

/**
 * @brief 无显示设备实现类
 *
 * NoDisplay是Display的派生类，用于表示没有物理显示设备的情况。
 * 所有显示方法都为空实现，Lock和Unlock方法直接返回成功。
 *
 * 当目标硬件平台没有显示设备时，系统会使用此类作为默认实现。
 */
class NoDisplay : public Display
{
private:
    /**
     * @brief 空实现的锁定方法
     *
     * 由于没有实际的显示设备，锁定总是成功。
     *
     * @param timeout_ms 锁定超时时间（毫秒）
     * @return 总是返回true
     */
    virtual bool Lock(int timeout_ms = 0) override
    {
        return true;
    }

    /**
     * @brief 空实现的解锁方法
     *
     * 由于没有实际的显示设备，无需执行任何解锁操作。
     */
    virtual void Unlock() override {}
};

#endif