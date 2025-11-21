#ifndef _LED_H_
#define _LED_H_

/**
 * @brief LED设备抽象基类
 *
 * Led类定义了LED设备的通用接口，用于根据设备状态控制LED的显示效果。
 * 该类使用了模板方法模式，具体的LED控制实现由派生类完成。
 *
 * 派生类需要实现OnStateChanged纯虚函数，以提供具体的LED控制逻辑。
 */
class Led
{
public:
    /**
     * @brief 虚析构函数，确保派生类能正确析构
     *
     * 使用=default显式声明默认析构函数，确保通过基类指针删除派生类对象时能正确调用派生类的析构函数。
     */
    virtual ~Led() = default;

    /**
     * @brief 当设备状态改变时调用此函数更新LED显示
     *
     * 根据设备的当前状态（如网络连接状态、电池电量、工作模式等）来设置LED的显示效果。
     * 具体的LED控制逻辑由派生类实现，可以包括LED颜色、闪烁模式、亮度等的控制。
     */
    // Set the led state based on the device state
    virtual void OnStateChanged() = 0;
};

/**
 * @brief 无LED设备实现类
 *
 * NoLed是Led的派生类，用于表示没有物理LED设备的情况。
 * OnStateChanged方法为空实现，不执行任何操作。
 *
 * 当目标硬件平台没有LED设备时，系统会使用此类作为默认实现。
 */
class NoLed : public Led
{
public:
    /**
     * @brief 空实现的设备状态改变处理函数
     *
     * 由于没有实际的LED设备，该函数不执行任何操作。
     * 这样可以避免在没有LED设备的平台上执行不必要的LED控制代码。
     */
    virtual void OnStateChanged() override {}
};

#endif // _LED_H_