#!/bin/bash

# 服务器端口
PORT=2222

# 总请求数量
REQUESTS=500

# 服务时间
SERVICE_TIME=1

# 输出文件
OUTPUT_FILE="server_utilization.csv"

# 打印标题到 CSV 文件
echo "Arrival Rate,Utilization" > $OUTPUT_FILE

# 运行实验
for ARRIVAL_RATE in {1..12}
do
    echo "Running test with arrival rate: $ARRIVAL_RATE"

    # 启动服务器并放在后台运行
    /usr/bin/time -v ./server $PORT > server_output_a${ARRIVAL_RATE}.txt 2>&1 &
    SERVER_PID=$!
    echo "Server started with PID $SERVER_PID"

    # 等待服务器准备就绪
    sleep 2

    echo "Running client now..."
    # 运行客户端
    ./client -a $ARRIVAL_RATE -s $SERVICE_TIME -n $REQUESTS $PORT > /dev/null
    echo "Client finished."

    # 等待服务器进程结束
    wait $SERVER_PID
    echo "Server process $SERVER_PID ended."

    # 提取 CPU 利用率数据
    UTILIZATION=$(grep "Percent of CPU this job got:" server_output_a${ARRIVAL_RATE}.txt | awk '{print $8}')
    echo "Utilization for rate $ARRIVAL_RATE: $UTILIZATION"

    # 保存到 CSV 文件
    echo "$ARRIVAL_RATE,$UTILIZATION" >> $OUTPUT_FILE
done

echo "Tests completed."

