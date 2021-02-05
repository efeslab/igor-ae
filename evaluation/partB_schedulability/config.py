from numpy import arange

# Length of each slot (milliseconds).
CONFIG_SLOT_SIZE_MS = 2.5

# List of possible task worst-case execution times (milliseconds).
# Each must be a multiple of CONFIG_SLOT_SIZE_MS.
CONFIG_TASK_WCETS_MS = [5, 10, 15, 20]

# List of possible task rates (Hz).
# The set of possible rates must be harmonic.
CONFIG_TASK_RATES_HZ = [5, 10, 20, 40]

# Maximum utilization error for generating a taskset.
CONFIG_TASKSET_UTIL_ERROR = 3 # percent

# Maximum iterations to take when generating a taskset.
CONFIG_TASKSET_MAX_ITERS = 1000

# Number of tasksets to generate for each utilization.
CONFIG_NUM_TASKSETS = 1000

# Number of cores used for computations.
CONFIG_NUM_CORES = 3

# Do not change below this line --------------------------------------

# List of utilizations to consider.
# Note that this utilization is *per core*.
CONFIG_UTIL_LIST = list(arange(0.1, 1 + 0.1, 0.1))

# List of possible task worst-case execution times (slots).
CONFIG_TASK_WCETS_SLOTS = \
    [int(x/CONFIG_SLOT_SIZE_MS) for x in CONFIG_TASK_WCETS_MS]

# List of possible task periods (milliseconds).
CONFIG_TASK_PERIODS_MS = \
    [(1000/x) for x in CONFIG_TASK_RATES_HZ]

# List of possible task periods (slots).
CONFIG_TASK_PERIODS_SLOTS = \
    [int(x/CONFIG_SLOT_SIZE_MS) for x in CONFIG_TASK_PERIODS_MS]

# Number of slots in schedule to generate.
CONFIG_SCHEDULE_SLOTS = max(CONFIG_TASK_PERIODS_SLOTS)

# Verify that period is never shorter than WCET.
for wcet in CONFIG_TASK_WCETS_SLOTS:
    for period in CONFIG_TASK_PERIODS_SLOTS:
        if (period < wcet):
            print ("INVALID CONFIG: Script assumes period is never < wcet")
            exit()
