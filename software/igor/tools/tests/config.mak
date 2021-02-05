#
# User configuration
#

# Enable or disable assertions.
#DEFS := -DNDEBUG
DEFS :=

# Bandwidth allocation GAP for AFDX library.
# Minimum seperation between frames sent on same VL.
# Valid values per A664 are 1 ms, 2 ms, 4 ms, etc.
DEFS += -DAFDX_BAG_MS=1

#=============================================
# Meant to not change in targets.cmake

# Define number of target processors.
# I.e. The number of RPis I can run cFS on.
# This is used for sizing the AFDX IP address table and CANNOT CHANGE.
DEFS += -DCPU_COUNT=11 

# Define max number of replica the system supports.
# This is used for logic related to the AFDX config table and CANNOT CHANGE.
DEFS += -DMAX_REPLICA_COUNT=10 

# Time to sleep between polling in exchange library.
# Set too low and the exchange threads will starve each other.
# Set too high and you add latency unnecessarily.
DEFS += -DEXCHANGE_RECV_POLL_US=500

# Time to sleep between polling in broadcast library.
# Set too low and the broadcast threads will starve each other.
# Set too high and you add latency unnecessarily.
DEFS += -DBCAST_RECV_POLL_US=1000

# Max number of AFDX messages to send between polling
# during messages exchange in broadcast library.
DEFS += -DBCAST_SEND_MAX_QUEUE=20

# Time to sleep between polling in state dispersal library.
DEFS += -DSTATE_RECV_POLL_US=1000

