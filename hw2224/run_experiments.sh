#!/bin/bash


RESULT_FILE="experiment_results.csv"
if [ -f "$RESULT_FILE" ]; then
    mv "$RESULT_FILE" "${RESULT_FILE}_backup_$(date +%Y%m%d%H%M%S).csv"
fi

echo "a_parameter,Utilization,Average_Response_Time,Average_Queue_Length" > $RESULT_FILE

for a in {1..15}
do
    echo "Running experiment with a=$a..."

    ./server_q 2222 > server_log.txt 2>&1 &
    SERVER_PID=$!
    echo "Server started with PID $SERVER_PID"

    sleep 2

    ./client -a $a -s 15 -n 1000 2222 > client_log.txt 2>&1

    sleep 2

    AVERAGE_RESPONSE_TIME=$(grep "Average Response Time" client_log.txt | awk '{print $4}')

    UTILIZATION=$(grep "Utilization:" server_log.txt | awk '{print $2}')
    AVERAGE_QUEUE_LENGTH=$(grep "Time-Weighted Average Queue Length:" server_log.txt | awk '{print $5}')

    if [[ -z "$UTILIZATION" || -z "$AVERAGE_RESPONSE_TIME" || -z "$AVERAGE_QUEUE_LENGTH" ]]; then
        echo "Failed to extract metrics for a=$a. Check server_log.txt and client_log.txt"
    else
        echo "$a,$UTILIZATION,$AVERAGE_RESPONSE_TIME,$AVERAGE_QUEUE_LENGTH" >> $RESULT_FILE
        echo "Experiment with a=$a completed: Utilization=$UTILIZATION, Average Response Time=$AVERAGE_RESPONSE_TIME, Average Queue Length=$AVERAGE_QUEUE_LENGTH"
    fi

    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    rm server_log.txt client_log.txt

    sleep 1
done

echo "All experiments completed. Results are in $RESULT_FILE"

