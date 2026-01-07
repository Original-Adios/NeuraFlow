/**
 * neura_ipc.hpp
 * NeuraFlow 通信内核 - 封装 ZeroMQ
 */
#pragma once

#include <zmq.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstring>
#include <vector>

// 定义回调函数类型
using RpcCallback = std::function<std::string(const std::string &)>;
using StreamCallback = std::function<void(const std::string &)>;

class IPCNode
{
public:
    // 构造函数：初始化 ZMQ Context
    IPCNode()
    {
        context_ = zmq_ctx_new();
        running_ = false;
    }

    ~IPCNode()
    {
        running_ = false;
        if (rpc_thread_.joinable())
            rpc_thread_.join();
        if (stream_thread_.joinable())
            stream_thread_.join();

        if (rpc_socket_)
            zmq_close(rpc_socket_);
        if (stream_socket_)
            zmq_close(stream_socket_);
        zmq_ctx_destroy(context_);
    }

    // ================= RPC 服务端功能 =================

    // 启动 RPC 监听 (REP模式)
    void start_rpc_server(const std::string &address)
    {
        rpc_socket_ = zmq_socket(context_, ZMQ_REP);
        int rc = zmq_bind(rpc_socket_, address.c_str());
        if (rc != 0)
        {
            std::cerr << "[IPC] RPC Bind failed: " << address << std::endl;
            exit(1);
        }

        running_ = true;
        rpc_thread_ = std::thread(&IPCNode::rpc_loop, this);
        std::cout << "[IPC] RPC Server listening on " << address << std::endl;
    }

    // 注册 RPC 方法
    void register_rpc(const std::string &action, RpcCallback cb)
    {
        std::lock_guard<std::mutex> lock(map_mutex_);
        rpc_map_[action] = cb;
    }

    // ================= RPC 客户端功能 =================

    // 静态方法：发起一次 RPC 调用 (REQ模式)
    // 注意：每次调用都创建新连接，适合低频控制指令
    static std::string call(const std::string &address, const std::string &action, const std::string &data)
    {
        void *ctx = zmq_ctx_new();
        void *sock = zmq_socket(ctx, ZMQ_REQ);

        int rc = zmq_connect(sock, address.c_str());
        if (rc != 0)
        {
            zmq_close(sock);
            zmq_ctx_destroy(ctx);
            return "ERROR: Connect failed";
        }

        // 1. 发送 Action (SNDMORE)
        zmq_send(sock, action.c_str(), action.size(), ZMQ_SNDMORE);
        // 2. 发送 Data
        zmq_send(sock, data.c_str(), data.size(), 0);

        // 3. 接收响应
        char buf[4096] = {0};
        int len = zmq_recv(sock, buf, sizeof(buf) - 1, 0);

        zmq_close(sock);
        zmq_ctx_destroy(ctx);

        if (len < 0)
            return "ERROR: Recv failed";
        return std::string(buf, len);
    }

    // ================= Stream 接收端功能 =================

    // 启动流数据接收 (PULL模式)
    void start_stream_receiver(const std::string &address, StreamCallback cb)
    {
        stream_socket_ = zmq_socket(context_, ZMQ_PULL);
        int rc = zmq_bind(stream_socket_, address.c_str());
        if (rc != 0)
        {
            std::cerr << "[IPC] Stream Bind failed: " << address << std::endl;
            exit(1);
        }

        stream_cb_ = cb;
        // 如果之前没启动线程，现在启动
        if (!running_)
            running_ = true;
        stream_thread_ = std::thread(&IPCNode::stream_loop, this);
        std::cout << "[IPC] Stream Receiver listening on " << address << std::endl;
    }

    // ================= Stream 发送端功能 =================

    // 推送流数据 (PUSH模式)
    void push_stream(const std::string &address, const std::string &data)
    {
        // 为了演示简单，这里每次创建 Socket。生产环境应使用连接池复用 Socket。
        void *sock = zmq_socket(context_, ZMQ_PUSH);
        zmq_connect(sock, address.c_str());
        zmq_send(sock, data.c_str(), data.size(), 0);
        zmq_close(sock); // PUSH 模式下 close 会尝试把缓冲数据发完
    }

private:
    // RPC 处理循环
    void rpc_loop()
    {
        while (running_)
        {
            // 1. 读取 Action
            char action_buf[256] = {0};
            int len = zmq_recv(rpc_socket_, action_buf, sizeof(action_buf) - 1, 0);
            if (len < 0)
                break; // Context 关闭或出错

            // 2. 读取 Data (如果有)
            std::string data = "";
            int more;
            size_t more_size = sizeof(more);
            zmq_getsockopt(rpc_socket_, ZMQ_RCVMORE, &more, &more_size);

            if (more)
            {
                std::vector<char> buffer(1024 * 1024); // 1MB Buffer
                int d_len = zmq_recv(rpc_socket_, buffer.data(), buffer.size(), 0);
                if (d_len > 0)
                    data.assign(buffer.data(), d_len);
            }

            // 3. 查找并执行回调
            std::string response = "ERROR: Action not found";
            std::string action(action_buf, len);

            {
                std::lock_guard<std::mutex> lock(map_mutex_);
                if (rpc_map_.count(action))
                {
                    // 执行业务逻辑
                    response = rpc_map_[action](data);
                }
            }

            // 4. 发送响应
            zmq_send(rpc_socket_, response.c_str(), response.size(), 0);
        }
    }

    // 流数据处理循环
    void stream_loop()
    {
        while (running_)
        {
            std::vector<char> buffer(1024 * 1024); // 1MB Buffer
            int len = zmq_recv(stream_socket_, buffer.data(), buffer.size(), 0);
            if (len < 0)
                break;

            if (stream_cb_)
            {
                std::string data(buffer.data(), len);
                stream_cb_(data);
            }
        }
    }

    void *context_ = nullptr;
    void *rpc_socket_ = nullptr;
    void *stream_socket_ = nullptr;

    bool running_;
    std::thread rpc_thread_;
    std::thread stream_thread_;

    std::unordered_map<std::string, RpcCallback> rpc_map_;
    std::mutex map_mutex_;
    StreamCallback stream_cb_;
};