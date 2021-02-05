cmake_minimum_required(VERSION 2.6.4)
project(CFS_SIM C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(../../${MISSION_NAME}_defs)
include_directories(../../${MISSION_NAME}_defs/tables)
include_directories(../comp_lib/fsw/src)
include_directories(../afdx_lib/fsw/src)
include_directories(../io_lib/fsw/src)
include_directories(../vote_lib/fsw/src)
include_directories(../comp_lib/fsw/src)
include_directories(../log_lib/fsw/src)

aux_source_directory(fsw/src APP_SRC_FILES)

# Create the app module
add_cfe_app(sim ${APP_SRC_FILES})
