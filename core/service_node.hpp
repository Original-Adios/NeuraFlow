/**
 * core/service_node.hpp
 * NeuraFlow 服务容器 - 对应原项目的 StackFlow
 * 作用：封装注册流程、隐藏通信细节
 */
#pragma once

#include "neura_ipc.hpp"
#include <iostream>
#include <thread>
#include <chrono>

class ServiceNode
{
public:
    // 构造函数：传入服务名（如 "llm", "tts"）
    ServiceNode(const std::string &service_name) : service_name_(service_name)
    {
        std::cout << "[" << service_name_ << "] Starting..." << std::endl;

        // 1. 初始化 IPC 节点
        // 这里不需要 bind RPC，因为我们还没拿到分配的地址
        // 但是通常每个 Worker 也会有一个自己的控制 RPC 地址，
        // 为了简化，我们暂时让 Worker 只通过 Broker 分配的 Stream 地址接收数据，
        // 或者我们可以让 Broker 分配两个地址。
        // **简化方案**：我们让 Worker 随机绑定一个临时 RPC 地址用于调试，
        // 但核心数据流地址必须由 Broker 分配。

        // 2. 向 Broker 注册
        register_to_broker();

        // 3. 注册标准生命周期 RPC
        // 当外界通过 RPC 调用的 "init" 时，自动触发子类的 on_init
        ipc_.register_rpc("init", [this](const std::string &config)
                          {
            std::cout << "[" << service_name_ << "] Received INIT command." << std::endl;
            bool success = this->on_init(config);
            return success ? "OK" : "FAIL"; });
    }

    virtual ~ServiceNode() {}

    // 运行服务（阻塞主线程）
    void run()
    {
        std::cout << "[" << service_name_ << "] Running. Press Ctrl+C to stop." << std::endl;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

protected:
    // ================== 供子类使用的功能 ==================

    // 发送结果数据（输出到下游）
    // 在真实场景中，这里应该查询路由表，或者发回给 Broker
    // 这里为了演示，我们默认发回给 Broker 的数据汇聚点
    void send_output(const std::string &data)
    {
        ipc_.push_stream("ipc:///tmp/neura.stream.broker", data);
    }

    // ================== 需子类实现的虚接口 ==================

    // 初始化业务 (加载模型等)
    virtual bool on_init(const std::string &config) = 0;

    // 处理流式数据 (推理等)
    virtual void on_process(const std::string &data) = 0;

private:
    // 注册逻辑
    void register_to_broker()
    {
        std::string broker_addr = "ipc:///tmp/neura.rpc.broker";

        std::cout << "[" << service_name_ << "] Connecting to Broker at " << broker_addr << "..." << std::endl;

        // 呼叫 Broker 的 "register" 接口
        // 重试机制：如果 Broker 没启动，不断重试
        while (true)
        {
            std::string resp = IPCNode::call(broker_addr, "register", service_name_);

            if (resp.find("ipc://") != std::string::npos)
            {
                my_stream_addr_ = resp;
                std::cout << "[" << service_name_ << "] Registration Successful!" << std::endl;
                std::cout << "[" << service_name_ << "] My Stream Address: " << my_stream_addr_ << std::endl;
                break;
            }
            else
            {
                std::cerr << "[" << service_name_ << "] Registration failed (Broker offline?), retrying in 2s..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }

        // 拿到地址后，立即启动 Stream 接收器
        // 绑定回调到子类的 on_process
        ipc_.start_stream_receiver(my_stream_addr_, [this](const std::string &data)
                                   { this->on_process(data); });
    }

    IPCNode ipc_;
    std::string service_name_;
    std::string my_stream_addr_;
};