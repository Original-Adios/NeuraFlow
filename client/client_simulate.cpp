/**
 * client_simulate.cpp
 * 模拟外部用户：初始化 -> 发送 Prompt -> 接收流式结果
 */
#include "core/neura_ipc.hpp"
#include <iostream>

int main()
{
    // 1. 定义地址
    // 真实场景下，Client 应该先问 Broker "llm_service" 在哪。
    // 这里为了简化，假设我们知道 Broker 会把它分到 ID 100。
    // (你可以先运行 worker 看看它被分到了哪个 ID，通常是 100)
    std::string broker_addr = "ipc:///tmp/neura.rpc.broker";
    // 注意：Worker 的 RPC 地址目前我们在 SDK 里没暴露独立的，
    // 但我们可以直接把 Stream 发给 Worker，或者通过 Broker 转发。
    // 为了演示 Phase 4 的完整性，我们这里直接往 Worker 的 Stream 地址发数据。

    std::string worker_stream_addr = "ipc:///tmp/neura.stream.100";

    // 2. 模拟发送 RPC Init 指令 (SDK 中我们让 Worker 随机绑定了一个临时 RPC 用于调试，但这里比较难猜)
    // *修正策略*：在 Phase 3 的代码中，ServiceNode 构造函数里注册了 "init" RPC，
    // 但 ServiceNode 目前只连了 Broker，没暴露自己的 RPC Server 端口给外部连。
    // 所以，我们在当前简化版架构中，直接测试 "on_process" (数据流)。
    // 工业级实现中，Broker 会充当 RPC 网关，转发 init 指令给 Worker。

    std::cout << "[Client] Step 1: Sending Prompt to " << worker_stream_addr << "..." << std::endl;

    // 创建一个发送节点
    IPCNode client;
    client.push_stream(worker_stream_addr, "Hello AI, who are you?");

    // 3. 模拟接收结果 (监听 Broker 的汇聚端口)
    // 因为 Worker 默认把结果 send_output 给 Broker
    // 所以我们其实看不到结果，除非我们也去监听 Broker。
    // 但 Broker 的日志会打印出来。

    std::cout << "[Client] Prompt sent! Check Worker and Broker terminals for logs." << std::endl;

    return 0;
}