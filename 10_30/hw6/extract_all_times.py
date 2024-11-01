# -*- coding: utf-8 -*-
import re
import sys

if len(sys.argv) != 3:
    print("Usage: python extract_all_times.py <log_file> <output_file>")
    sys.exit(1)

log_file = sys.argv[1]
output_file = sys.argv[2]

times = []

# 定义正则表达式来匹配处理时间
# 假设日志格式与之前相同，并且处理时间可以计算为：
# 处理时间 = 完成时间戳 - 请求时间戳

pattern = re.compile(
    r'T\d+ R\d+:(?P<req_timestamp>\d+\.\d+),\w+,\d+,\d+,\d+,\d+\.\d+,\d+\.\d+,(?P<completion_timestamp>\d+\.\d+)'
)

with open(log_file, 'r') as f:
    for line in f:
        match = pattern.search(line)
        if match:
            req_timestamp = float(match.group('req_timestamp'))
            completion_timestamp = float(match.group('completion_timestamp'))
            processing_time = completion_timestamp - req_timestamp
            times.append(processing_time)

with open(output_file, 'w') as f:
    for t in times:
        f.write(f"{t}\n")

