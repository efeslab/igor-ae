cmake_minimum_required(VERSION 2.6.4)
project(CFS_AFDX_LIB C)

include_directories(fsw/public_inc)
include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)
include_directories(../../${MISSION_NAME}_defs)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(afdx_lib ${LIB_SRC_FILES} ../../igor_defs/tables/afdx_lib_vls.c)

