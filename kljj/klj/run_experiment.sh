#!/bin/bash

# 设置参数
FIFO_COMMAND="/usr/bin/time -v ./server_pol -w 2 -q 100 -p FIFO 2222 & ./client -a 10 -s 20 -n 1500 localhost  2222"
SJN_COMMAND="/usr/bin/time -v ./server_pol -w 2 -q 100 -p SJN 2222 & ./client -a 10 -s 20 -n 1500 localhost 2222"
HIGH_FIFO_COMMAND="/usr/bin/time -v ./server_pol -w 2 -q 100 -p FIFO 2222 & ./client -a 40 -s 20 -n 1500 localhost 2222"
HIGH_SJN_COMMAND="/usr/bin/time -v ./server_pol -w 2 -q 100 -p SJN 2222 & ./client -a 40 -s 20 -n 1500 localhost 2222"

# 输出文件
OUTPUT_FILE="results.txt"
echo "调度策略比较实验结果" > $OUTPUT_FILE
echo "======================" >> $OUTPUT_FILE

# 低利用率实验
echo "低利用率实验：" >> $OUTPUT_FILE
for i in {1..10}
do
    echo "运行第 $i 次 FIFO..." | tee -a $OUTPUT_FILE
    eval $FIFO_COMMAND 2>&1 | grep "Elapsed (wall clock) time" | tee -a $OUTPUT_FILE
done

for i in {1..10}
do
    echo "运行第 $i 次 SJN..." | tee -a $OUTPUT_FILE
    eval $SJN_COMMAND 2>&1 | grep "Elapsed (wall clock) time" | tee -a $OUTPUT_FILE
done

# 高利用率实验
echo "高利用率实验：" >> $OUTPUT_FILE
for i in {1..10}
do
    echo "运行第 $i 次 FIFO..." | tee -a $OUTPUT_FILE
    eval $HIGH_FIFO_COMMAND 2>&1 | grep "Elapsed (wall clock) time" | tee -a $OUTPUT_FILE
done

for i in {1..10}
do
    echo "运行第 $i 次 SJN..." | tee -a $OUTPUT_FILE
    eval $HIGH_SJN_COMMAND 2>&1 | grep "Elapsed (wall clock) time" | tee -a $OUTPUT_FILE
done

echo "实验结束，结果已保存到 $OUTPUT_FILE" | tee -a $OUTPUT_FILE

