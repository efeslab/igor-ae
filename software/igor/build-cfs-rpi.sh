#!/bin/bash

# Copy the Igor code to all target RPis.
# For each RPi, update the targets_enable.cmake file.
# For each RPi, build cFS for that RPi in the background.
# Then copy the resulting binaries to the desktop.

USER=aloveless
USER_RPI=pi

#HOST_RPI=( "10.0.0.21" "10.0.0.31" )
#ID_RPI=( "1" "11" )

HOST_RPI=( "10.0.0.21" "10.0.0.22" "10.0.0.23" "10.0.0.24" "10.0.0.31" )
ID_RPI=( "1" "2" "3" "4" "11" )

#HOST_RPI=( "10.0.0.21" "10.0.0.22" "10.0.0.23" "10.0.0.24" "10.0.0.25" "10.0.0.26" "10.0.0.27" "10.0.0.31" )
#ID_RPI=( "1" "2" "3" "4" "5" "6" "7" "11" )

for ((i=0;i<${#HOST_RPI[@]};++i)); do
    HOST="${HOST_RPI[i]}"
    ID="${ID_RPI[i]}"    
    ssh -4 -l ${USER_RPI} ${HOST} "cd ~/Desktop/; rm -rf igor/; rm -rf cfs/"
    scp -4 -r ../igor ${USER_RPI}@${HOST}:/home/${USER_RPI}/Desktop/igor
    ssh -4 -l ${USER_RPI} ${HOST} "cd ~/Desktop/igor/igor_defs; rm -f targets_enable.cmake; touch targets_enable.cmake; echo 'SET(cpu${ID}_ENABLE 1)' > targets_enable.cmake"
done

for ((i=0;i<${#HOST_RPI[@]};++i)); do
    HOST="${HOST_RPI[i]}"
    ssh -4 -l ${USER_RPI} ${HOST} "cd ~/Desktop/igor; rm -rf build/; make prep; make; make install" &
done
wait

for ((i=0;i<${#HOST_RPI[@]};++i)); do
    HOST="${HOST_RPI[i]}"
    ID="${ID_RPI[i]}"    
    ssh -4 -l ${USER_RPI} ${HOST} "cd ~/Desktop/igor/build/exe; cp -rp cpu${ID} ~/Desktop/cfs"
done
