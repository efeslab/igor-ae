cmake_minimum_required(VERSION 2.6.4)
project(CFS_SCH_SYNC C)

include_directories(fsw/public_inc)
include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)
include_directories(../3rd-party/pigpio)
include_directories(../../${MISSION_NAME}_defs)
include_directories(../../${MISSION_NAME}_defs/tables)
include_directories(../afdx_lib/fsw/src)
include_directories(../log_lib/fsw/src)
include_directories(../bcast_lib/fsw/src)
include_directories(../io_lib/fsw/src)
include_directories(../exchange_lib/fsw/src)
include_directories(../state_lib/fsw/src)

aux_source_directory(fsw/src APP_SRC_FILES)

# Choose schedule table based on CPU.
# CPU1 - CPU10 are replicas; CPU11 is the sim.
# MY_TARGET_NAME is defined in cfe/MakeLists.txt.
if(${MY_TARGET_NAME} STREQUAL "cpu11")
    add_cfe_app(sch_sync ${APP_SRC_FILES} ../../igor_defs/tables/${SIM_SCH_TABLE})
else()
    add_cfe_app(sch_sync ${APP_SRC_FILES} ../../igor_defs/tables/${REPLICA_SCH_TABLE})
endif()

# Used for LEDs and interrupt.
target_link_libraries(sch_sync pthread ${CMAKE_SOURCE_DIR}/../apps/3rd-party/pigpio/libpigpio.a)

