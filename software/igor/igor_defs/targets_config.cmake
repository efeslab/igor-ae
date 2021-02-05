# Define size of the data sent to the actuators in bytes.
add_definitions(-DACTUATOR_DATA_SIZE=500)

# Define size of the state maintained by the replicas.
# Must divide evenly by the number of state dispersal instances.
add_definitions(-DSTATE_SIZE=1500) # must be >= ACTUATOR_DATA_SIZE

# Specify number of instances to use for state dispersal.
# Note that this does not impact end-to-end latency measurements.
add_definitions(-DSTATE_INSTANCES=1)

# Set execution time for source selection in ms.
add_definitions(-DSOURCE_SELECT_TIME_MS=1)

#=================================

# Define number of processors that are replicas.
# I.e. The ones the form the replicated state machine.
add_definitions(-DREPLICA_COUNT=4)

# Define size of the sensor data in bytes.
add_definitions(-DSENSOR_DATA_SIZE=750)

# Define number of sensors the system uses.
add_definitions(-DSENSOR_COUNT=3)

# Bandwidth allocation GAP for AFDX library.
# Minimum seperation between frames sent on same VL.
# Valid values per A664 are 1 ms, 2 ms, 4 ms, etc.
add_definitions(-DAFDX_BAG_MS=1) 

# Set execution time for computation in ms.
add_definitions(-DCOMP_TIME_MS=10)

#=================================

# Enable (1) or disable (0) logging.
add_definitions(-DLOG_ENABLE=1)

# Specify the mode for the sim app.
# SIM_MODE_NO_REP = Send from sensor 0 only; don't vote replies.
# SIM_MODE_VOTE   = Send from all sensors; vote replies. 
#add_definitions(-DSIM_MODE=SIM_MODE_NO_REP)
add_definitions(-DSIM_MODE=SIM_MODE_VOTE)

# Specify number of iterations for sim to perform.
add_definitions(-DSIM_NUM_ITER=100)

# Specify the replica schedule table.
#SET(REPLICA_SCH_TABLE "default/default_replica_table_igor_nofilter_ef.c")
#SET(REPLICA_SCH_TABLE "default/default_replica_table_igor_ef.c")
#SET(REPLICA_SCH_TABLE "default/default_replica_table_bft_turpin_ef.c")
SET(REPLICA_SCH_TABLE "default/default_replica_table_bft_ef.c")
#SET(REPLICA_SCH_TABLE "default/default_replica_table_no_rep.c")

#SET(REPLICA_SCH_TABLE "custom/custom_replica_table_igor_ef.c")

# Specify the sim schedule table.
SET(SIM_SCH_TABLE "default/default_sim_table.c")

#SET(SIM_SCH_TABLE "custom/custom_sim_table_igor_ef.c")
