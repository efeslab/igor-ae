cmake_minimum_required(VERSION 2.6.4)
project(CFS_LOG_LIB C)

include_directories(fsw/public_inc)
include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)
include_directories(../../${MISSION_NAME}_defs)
include_directories(../../${MISSION_NAME}_defs/tables)
include_directories(../3rd-party/pigpio)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(log_lib ${LIB_SRC_FILES})

# Use pigpio for timestamping.
target_link_libraries(log_lib pthread ${CMAKE_SOURCE_DIR}/../apps/3rd-party/pigpio/libpigpio.a)
