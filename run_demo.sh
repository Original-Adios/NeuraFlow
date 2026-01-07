#!/bin/bash

# 定义清理函数：脚本退出时杀掉所有后台进程
cleanup() {
    echo ""
    echo ">>> [System] Shutting down..."
    kill $BROKER_PID $WORKER_PID 2>/dev/null
    wait $BROKER_PID $WORKER_PID 2>/dev/null
    echo ">>> [System] All services stopped."
}
trap cleanup EXIT

echo ">>> [System] 1. Starting Broker..."
./build/broker &
BROKER_PID=$!
sleep 1  # 等待 Broker 启动

echo ">>> [System] 2. Starting LLM Worker..."
./build/worker_llm &
WORKER_PID=$!
sleep 2  # 等待 Worker 注册完成

echo ">>> [System] 3. Starting Client Simulation..."
echo "---------------------------------------------------"
./build/client_simulate
echo "---------------------------------------------------"

# 等待用户按键退出，或者让 Client 跑完就退出
echo ">>> [System] Demo finished. Press Ctrl+C to exit."
wait $CLIENT_PID
sleep 5 # 让最后的 log 打印完