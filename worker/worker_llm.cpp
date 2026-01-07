/**
 * worker_llm.cpp
 * 模拟一个 LLM 推理服务
 * 特点：纯业务逻辑，无 ZMQ 代码
 */
#include "core/service_node.hpp"
#include <vector>
#include <sstream>

class LLMWorker : public ServiceNode
{
public:
    // 给服务起个名字叫 "llm_service"
    LLMWorker() : ServiceNode("llm_service") {}

protected:
    // 1. 初始化逻辑 (对应 RPC "init")
    bool on_init(const std::string &config) override
    {
        std::cout << ">>> [LLM] Loading model... Config: " << config << std::endl;

        // 模拟加载大模型的耗时 (2秒)
        for (int i = 0; i <= 100; i += 20)
        {
            std::cout << ">>> [LLM] Loading: " << i << "%" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }

        std::cout << ">>> [LLM] Model Loaded Successfully!" << std::endl;
        return true; // 返回 true 表示初始化成功
    }

    // 2. 处理逻辑 (对应 Stream Data)
    void on_process(const std::string &prompt) override
    {
        std::cout << ">>> [LLM] Received Prompt: " << prompt << std::endl;

        // 模拟 LLM 生成的回复内容
        std::string full_response = "DeepSeek is a powerful AI model running on Edge.";
        std::stringstream ss(full_response);
        std::string word;

        // 模拟流式输出 (Token Streaming)
        while (ss >> word)
        {
            // 模拟推理耗时 (每生成一个词花 200ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // 构造输出数据 (加个空格还原句子)
            std::string token = word + " ";

            std::cout << ">>> [LLM] Generated Token: " << token << std::endl;

            // 发送给下游 (通过 SDK)
            send_output(token);
        }

        // 发送结束标记 (可选协议)
        send_output("<EOS>");
        std::cout << ">>> [LLM] Inference Finished." << std::endl;
    }
};

int main()
{
    LLMWorker worker;
    worker.run(); // 进入 SDK 管理的事件循环
    return 0;
}