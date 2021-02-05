#!/usr/bin/env python

# Define columns in input file.
COL_APP_CODE  = 0
COL_ACT_CODE  = 1
COL_TIME_USEC = 2

# Define activity types.
LOG_ACT_IRQ_RECV  = 0
LOG_ACT_APP_START = 1
LOG_ACT_APP_STOP  = 2

# Define application codes.
LOG_APP_SCH            = 0
LOG_APP_SIM            = 1
LOG_APP_READ_SENSOR    = 2
LOG_APP_SEND_ACTUATOR  = 3
LOG_APP_AGREEMENT      = 4
LOG_APP_SOURCE_SELECT  = 5
LOG_APP_FILTER_SENSOR  = 6  
LOG_APP_COMPUTE_0      = 7
LOG_APP_COMPUTE_1      = 8
LOG_APP_COMPUTE_2      = 9
LOG_APP_COMPUTE_3      = 10
LOG_APP_COMPUTE_4      = 11
LOG_APP_COMPUTE_ALL    = 12
LOG_APP_STATE_CONS     = 13
LOG_APP_STATE_DISPERSE = 14
LOG_APP_FILTER_0       = 15
LOG_APP_FILTER_1       = 16
LOG_APP_FILTER_2       = 17
LOG_APP_FILTER_3       = 18
LOG_APP_FILTER_4       = 19
LOG_APP_PARALLEL_START = 20

# Length of a minor frame in milliseconds.
MINOR_FRAME_MS = 2.5

# Number of minor frames in a major frame.
MINOR_FRAME_COUNT = 400

# Activities per minor frame.
ACTIVITIES_PER_MINOR = 10

# Number of minor frames to offset sim read/write.
SIM_OFFSET_SLOTS = 1

# Margin to add to time measurements (1.2 = 20%).
TIME_MARGIN = 1.1

# Set to True to print measurement histograms.
PRINT_HIST = False

# Max length of a histogram row.
HIST_MAX_WIDTH = 50

