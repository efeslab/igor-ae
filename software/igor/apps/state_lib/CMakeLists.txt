cmake_minimum_required(VERSION 2.6.4)
project(CFS_STATE_LIB C)

include_directories(fsw/public_inc)
include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)
include_directories(../../${MISSION_NAME}_defs)
include_directories(../../${MISSION_NAME}_defs/tables)
include_directories(../afdx_lib/fsw/src)
include_directories(../vote_lib/fsw/src)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(state_lib ${LIB_SRC_FILES})


target_link_libraries(state_lib m)
