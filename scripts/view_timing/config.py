#!/usr/bin/env python

# Number of minor frames per plot.
MINOR_PER_PLOT = 20

# Define columns in input file.
COL_APP_CODE  = 0
COL_ACT_CODE  = 1
COL_TIME_USEC = 2

# Define activity types.
LOG_ACT_IRQ_RECV  = 0
LOG_ACT_APP_START = 1
LOG_ACT_APP_STOP  = 2

# Define applications to plot.
# Each app has an associated color and alpha channel.
# See color list: http://stackoverflow.com/questions/22408237
LOG_APP_SCH = 0 # says which app is the scheduler
APP_DICT = {0:  ("SCH_SYNC",    "red", 1.0),  
            1:  ("SIM",         "darkgreen", 1.0),
            2:  ("SENSOR",      "darkgreen", 1.0),
            3:  ("ACTUATOR",    "darkgreen", 1.0), 
            4:  ("AGREEMENT",   "darkgreen", 1.0),                                     
            5:  ("SOURCE SEL",  "darkgreen", 1.0),
            6:  ("FILTER",      "darkgreen", 1.0),
            7:  ("COMPUTE_0",   "darkgreen", 1.0),
            8:  ("COMPUTE_1",   "darkgreen", 1.0),
            9:  ("COMPUTE_2",   "darkgreen", 1.0),
            10: ("COMPUTE_3",   "darkgreen", 1.0),
            11: ("COMPUTE_4",   "darkgreen", 1.0),
            12: ("COMPUTE_ALL", "darkgreen", 1.0),            
            13: ("STATE_CON",   "darkgreen", 1.0),
            14: ("STATE_DIS",   "darkgreen", 1.0),
            
            15: ("FILTER_0",    "darkgreen", 1.0),    
            16: ("FILTER_1",    "darkgreen", 1.0),    
            17: ("FILTER_2",    "darkgreen", 1.0),                        
            }

