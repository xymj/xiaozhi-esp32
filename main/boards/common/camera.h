#ifndef CAMERA_H
#define CAMERA_H

#include <string>

/**
 * @brief 摄像头设备抽象基类
 *
 * Camera类定义了摄像头设备的通用接口，包括图像捕获、镜像翻转、图像解释等功能。
 * 该类使用了模板方法模式，具体的摄像头实现由派生类完成。
 *
 * 派生类需要实现所有纯虚函数，以提供具体的摄像头功能实现。
 */
class Camera
{
public:
    /**
     * @brief 设置图像解释服务的URL和访问令牌
     *
     * 配置用于图像解释（如AI视觉识别）的远程服务地址和认证信息。
     * 当摄像头捕获图像后，可以通过该服务对图像内容进行分析和解释。
     *
     * @param url 图像解释服务的URL地址
     * @param token 访问服务所需的认证令牌
     */
    virtual void SetExplainUrl(const std::string &url, const std::string &token) = 0;

    /**
     * @brief 捕获图像
     *
     * 触发摄像头进行图像捕获操作，将捕获的图像数据存储在内部缓冲区中。
     *
     * @return 成功捕获图像返回true，失败返回false
     */
    virtual bool Capture() = 0;

    /**
     * @brief 设置水平镜像
     *
     * 控制摄像头图像的水平镜像功能，开启后图像会左右翻转显示。
     *
     * @param enabled true表示启用水平镜像，false表示禁用
     * @return 设置成功返回true，失败返回false
     */
    virtual bool SetHMirror(bool enabled) = 0;

    /**
     * @brief 设置垂直翻转
     *
     * 控制摄像头图像的垂直翻转功能，开启后图像会上下翻转显示。
     *
     * @param enabled true表示启用垂直翻转，false表示禁用
     * @return 设置成功返回true，失败返回false
     */
    virtual bool SetVFlip(bool enabled) = 0;

    /**
     * @brief 图像解释（AI视觉识别）
     *
     * 将捕获的图像发送到配置的解释服务，对图像内容进行分析并根据问题返回解释结果。
     * 例如：识别图像中的物体、场景或回答与图像相关的问题。
     *
     * @param question 关于图像内容的问题
     * @return 返回图像解释的结果字符串
     */
    virtual std::string Explain(const std::string &question) = 0;
};

#endif // CAMERA_H