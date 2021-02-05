#!/bin/bash

# Copy the Igor code to RPis 1-10 (all the replicas).
# For each of RPis, build the tests for that RPi in the background.

USER=aloveless
USER_RPI=pi
HOST_RPI=( "10.0.0.21" "10.0.0.22" "10.0.0.23" "10.0.0.24" "10.0.0.25" "10.0.0.26" "10.0.0.27" "10.0.0.28" "10.0.0.29" "10.0.0.30" )
INDEX_RPI=( "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" )
ID_RPI=( "1" "2" "3" "4" "5" "6" "7" "8" "9" "10" )

for ((i=0;i<${#HOST_RPI[@]};++i)); do
    HOST="${HOST_RPI[i]}"
    ssh -l ${USER_RPI} ${HOST} "cd ~/Desktop/; rm -rf igor/; rm -rf tests/; mkdir tests"
    scp -r ../../../igor ${USER_RPI}@${HOST}:/home/${USER_RPI}/Desktop/igor
done

for ((i=0;i<${#HOST_RPI[@]};++i)); do
    HOST="${HOST_RPI[i]}"
    INDEX="${INDEX_RPI[i]}"    
    ID="${ID_RPI[i]}"    
    ssh -l ${USER_RPI} ${HOST} "cd ~/Desktop/igor/tools/tests; make clean; make REPLICA_INDEX=\"${INDEX}\" CFE_PLATFORM_CPU_ID=\"${ID}\"; cp *.bin ~/Desktop/tests/." &
done
wait
