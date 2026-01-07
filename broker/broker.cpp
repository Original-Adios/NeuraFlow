/**
 * broker.cpp
 * NeuraFlow Service Broker - 系统管理中心
 */
#include "core/neura_ipc.hpp"
#include <iostream>
#include <atomic>
#include <vector>

class ServiceBroker
{
public:
    ServiceBroker()
    {
        // 初始化 ID 计数器，从 100 开始
        next_worker_id_ = 100;
    }

    void run()
    {
        // 1. 启动 RPC 监听 (这是 Broker 的固定入口)
        // 所有 Worker 启动时都会尝试连接这个地址
        std::string broker_rpc_addr = "ipc:///tmp/neura.rpc.broker";
        ipc_.start_rpc_server(broker_rpc_addr);

        // 2. 注册核心管理接口: "register"
        ipc_.register_rpc("register", [this](const std::string &service_name)
                          { return this->handle_registration(service_name); });

        // 3. (可选) 启动一个数据流接收端口，作为默认的数据汇聚点
        // 比如 Worker 算完结果，默认发回给 Broker
        std::string broker_stream_addr = "ipc:///tmp/neura.stream.broker";
        ipc_.start_stream_receiver(broker_stream_addr, [this](const std::string &data)
                                   { std::cout << "[Broker] Received Data Stream: " << data << std::endl; });

        std::cout << "[Broker] System Started." << std::endl;
        std::cout << "   - RPC Registry: " << broker_rpc_addr << std::endl;
        std::cout << "   - Data Sink   : " << broker_stream_addr << std::endl;

        // 阻塞主线程，防止退出
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

private:
    // 处理注册逻辑
    std::string handle_registration(const std::string &service_name)
    {
        // 1. 分配 ID
        int id = next_worker_id_++;

        // 2. 生成该 Worker 专属的通信地址
        // 这里的格式是约定好的，Worker 拿到这个地址后，会去 bind 它
        std::string worker_stream_addr = "ipc:///tmp/neura.stream." + std::to_string(id);

        // 3. 记录路由表 (加锁保护)
        {
            std::lock_guard<std::mutex> lock(routes_mutex_);
            routes_[service_name] = worker_stream_addr;
        }

        std::cout << "[Broker] New Service Registered: "
                  << service_name << " (ID: " << id << ")" << std::endl;
        std::cout << "         -> Assigned Address: " << worker_stream_addr << std::endl;

        // 4. 返回分配的地址给 Worker
        return worker_stream_addr;
    }

    IPCNode ipc_;
    std::atomic<int> next_worker_id_;

    std::unordered_map<std::string, std::string> routes_;
    std::mutex routes_mutex_;
};

int main()
{
    ServiceBroker broker;
    broker.run();
    return 0;
}