# Toolchain file for a Raspberry Pi 3B+ running Linux.
# The board uses an ARM Cortex A53 processor.
# The toolchain file is designed for compiling cFS directly
# on the RPi board.

# Basic cross system configuration
SET(CMAKE_SYSTEM_NAME	   Linux)
SET(CMAKE_SYSTEM_VERSION   1)
SET(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler executables.
SET(CMAKE_C_COMPILER   "/usr/bin/gcc")
SET(CMAKE_CXX_COMPILER "/usr/bin/g++")

# Configure the find commands
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

# Specify which cFE and OSAL to use.
SET(CFE_SYSTEM_PSPNAME  "pc-linux")
SET(OSAL_SYSTEM_BSPNAME "pc-linux")
SET(OSAL_SYSTEM_OSTYPE  "posix")
