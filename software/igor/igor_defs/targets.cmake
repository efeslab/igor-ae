######################################################################
# 
# Master config file for cFS target boards
#
# This file indicates the architecture and configuration of the
# target boards that will run core flight software.
#
# The following variables are defined per board, where <x> is the 
# CPU number starting with 1:
#
#  TGT<x>_NAME : the user-friendly name of the cpu.  Should be simple
#       word with no punctuation
#  TGT<x>_APPLIST : list of applications to build and install on the CPU
#  TGT<x>_DRIVERLIST : list of drivers to build and statically link into 
#       the PSP for this CPU. This requires the PSP iodriver enhancement.
#  TGT<x>_FILELIST : list of extra files to copy onto the target.  No
#       modifications of the file will be made.  In order to differentiate
#       between different versions of files with the same name, priority
#       will be given to a file named <cpuname>_<filename> to be installed
#       as simply <filename> on that cpu (prefix will be removed). 
#  TGT<x>_SYSTEM : the toolchain to use for building all code.  This
#       will map to a CMake toolchain file called "toolchain-<ZZZ>"
#       If not specified then it will default to "cpu<x>" so that
#       each CPU will have a dedicated toolchain file and no objects
#       will be shared across CPUs.  
#       Otherwise any code built using the same toolchain may be 
#       copied to multiple CPUs for more efficient builds.
#  TGT<x>_PLATFORM : configuration for the CFE core to use for this
#       cpu.  This determines the cfe_platform_cfg.h to use during the
#       build.  Multiple files/components may be concatenated together
#       allowing the config to be generated in a modular fashion.  If 
#       not specified then it will be assumed as "default <cpuname>".
# 

# The MISSION_NAME will be compiled into the target build data structure
# as well as being passed to "git describe" to filter the tags when building
# the version string.
SET(MISSION_NAME "igor")

# SPACECRAFT_ID gets compiled into the build data structure and the PSP may use it.
# should be an integer.
SET(SPACECRAFT_ID 42)

# UI_INSTALL_SUBDIR indicates where the UI data files (included in some apps) should
# be copied during the install process.
SET(UI_INSTALL_SUBDIR "host/ui")

# FT_INSTALL_SUBDIR indicates where the black box test data files (lua scripts) should
# be copied during the install process.
SET(FT_INSTALL_SUBDIR "host/functional-test")

#=============================================
# CONFIGURATION SECTION
# In general, should not need to change.

# Possible modes for the sim app.
add_definitions(-DSIM_MODE_NO_REP=0) # Send from sensor 0 only; don't vote replies.
add_definitions(-DSIM_MODE_VOTE=1)   # Send from all sensors; vote replies.

# Define number of target processors.
# I.e. The number of RPis I can run cFS on.
# This is used for sizing the AFDX IP address table and CANNOT CHANGE.
add_definitions(-DCPU_COUNT=11) 

# Define max number of replica the system supports.
# This is used for logic related to the AFDX config table and CANNOT CHANGE.
add_definitions(-DMAX_REPLICA_COUNT=10) 

# Define max number of sensors the system supports.
# This is used for definitions in some of the test cases and CANNOT CHANGE.
add_definitions(-DMAX_SENSOR_COUNT=5) 

# Time to sleep between polling in exchange library.
# Set too low and the exchange threads will starve each other.
# Set too high and you add latency unnecessarily.
add_definitions(-DEXCHANGE_RECV_POLL_US=1000) 

# Time to sleep between polling in exchange library.
# Like EXCHANGE_RECV_POLL_US, but used when batching.
add_definitions(-DEXCHANGE_RECV_POLL_MULTI_US=1000) 

# Time to sleep between polling in broadcast library.
# Set too low and the broadcast threads will starve each other.
# Set too high and you add latency unnecessarily.
add_definitions(-DBCAST_RECV_POLL_US=1000) 

# Time to sleep between polling in broadcast library.
# Like BCAST_RECV_POLL_US, but used when batching.
add_definitions(-DBCAST_RECV_POLL_MULTI_US=1000) 

# Max number of AFDX messages to send between polling
# during messages exchange in broadcast library.
add_definitions(-DBCAST_SEND_MAX_QUEUE=1) # 20

# Time to sleep between polling in state dispersal library.
add_definitions(-DSTATE_RECV_POLL_US=1000) 

# Time to sleep between polling in dispersal library.
# Like STATE_RECV_POLL_US, but used when batching.
add_definitions(-DSTATE_RECV_POLL_MULTI_US=1000) 

#=============================================
# USER CONFIGURATION SECTION

# Import user configuration file.
include(${MISSION_DEFS}/targets_config.cmake)

#=============================================
# BUILD SECTION

# Determine which targets to enable.
include(${MISSION_DEFS}/targets_enable.cmake)

SET(REPLICA_APPLIST afdx_lib bcast_lib comp_lib io_lib log_lib vote_lib select_lib exchange_lib state_lib sch_sync test_no_rep test_bft_ef test_bft_turpin_ef test_igor_ef test_igor_nofilter_ef)

SET(SIM_APPLIST     afdx_lib bcast_lib comp_lib io_lib log_lib vote_lib exchange_lib state_lib sch_sync sim)

SET(TGT1_NAME cpu1)
SET(TGT1_APPLIST ${REPLICA_APPLIST})
SET(TGT1_FILELIST cfe_es_startup.scr)

SET(TGT2_NAME cpu2)
SET(TGT2_APPLIST ${REPLICA_APPLIST})
SET(TGT2_FILELIST cfe_es_startup.scr)

SET(TGT3_NAME cpu3)
SET(TGT3_APPLIST ${REPLICA_APPLIST})
SET(TGT3_FILELIST cfe_es_startup.scr)

SET(TGT4_NAME cpu4)
SET(TGT4_APPLIST ${REPLICA_APPLIST})
SET(TGT4_FILELIST cfe_es_startup.scr)

SET(TGT5_NAME cpu5)
SET(TGT5_APPLIST ${REPLICA_APPLIST})
SET(TGT5_FILELIST cfe_es_startup.scr)

SET(TGT6_NAME cpu6)
SET(TGT6_APPLIST ${REPLICA_APPLIST})
SET(TGT6_FILELIST cfe_es_startup.scr)

SET(TGT7_NAME cpu7)
SET(TGT7_APPLIST ${REPLICA_APPLIST})
SET(TGT7_FILELIST cfe_es_startup.scr)

SET(TGT8_NAME cpu8)
SET(TGT8_APPLIST ${REPLICA_APPLIST})
SET(TGT8_FILELIST cfe_es_startup.scr)

SET(TGT9_NAME cpu9)
SET(TGT9_APPLIST ${REPLICA_APPLIST})
SET(TGT9_FILELIST cfe_es_startup.scr)

SET(TGT10_NAME cpu10)
SET(TGT10_APPLIST ${REPLICA_APPLIST})
SET(TGT10_FILELIST cfe_es_startup.scr)

SET(TGT11_NAME cpu11)
SET(TGT11_APPLIST ${SIM_APPLIST})
SET(TGT11_FILELIST cfe_es_startup.scr)

