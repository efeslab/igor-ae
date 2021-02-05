#!/usr/bin/env python
# -*- coding: UTF-8 -*-

# Standard modules
import os
import sys
sys.dont_write_bytecode = True
import math
import shutil
import glob
import os
import re
import gc
import cgi

# Installed modules
from Tkinter import Tk
from tkFileDialog import askopenfilename
import numpy as np

# Import user-configuration. 
from config import *

# Parse the provided log file and return a list of the times it takes for the
# given app to execute in microseconds. Each complete execution of the app 
# corresponds to a different entry in the returned list.
# Each measurement is multiplied by a configurable TIME_MARGIN.
def GetTimesUs(file_data, app_code):

    times_us = []
    start_us = 0
    stop_us  = 0
    delta_us = 0

    for row in file_data:
        if (row[COL_APP_CODE] == app_code):     
            if (row[COL_ACT_CODE] == LOG_ACT_APP_START):        
                start_us = row[COL_TIME_USEC]
            if (row[COL_ACT_CODE] == LOG_ACT_APP_STOP):        
                stop_us = row[COL_TIME_USEC]              
                delta_us = stop_us - start_us
                times_us.append(delta_us * TIME_MARGIN)

    return times_us

# This function is similar to GetTimesUs(), except allows the caller to
# specify a different application code for the start and stop log entries.
# In the case when multiple application codes are used to represent the end
# of an activity, the lastest one is used. I.e. execution time is the time
# between the start app code and the latest stop app code.
def GetTimesMultiUs(file_data, start_code, stop_codes):

    times_us = []
    start_us = -1
    stop_us  = -1
    delta_us = -1

    for row in file_data:

        if ((row[COL_APP_CODE] == start_code) and
            (row[COL_ACT_CODE] == LOG_ACT_APP_START)):

            if (start_us != -1):
                delta_us = stop_us - start_us # last stop - last start
                times_us.append(delta_us * TIME_MARGIN)
              
            start_us = row[COL_TIME_USEC]    

        elif ((row[COL_APP_CODE] in stop_codes) and
              (row[COL_ACT_CODE] == LOG_ACT_APP_STOP)):

            stop_us = row[COL_TIME_USEC]

    return times_us

# Given a list of execution times in microseconds, return a list of the number
# of slots needed for each execution to complete.
def GetSlots(times_us):

    minor_frame_us = MINOR_FRAME_MS * 1000.0
    slots = []
    for time_us in times_us:
        # Round up to next integer.
        time_int_us = math.ceil(time_us)
        # Then round that number up to next multiple of slot duration.       
        time_int_us = time_int_us + (minor_frame_us - 1)
        time_int_us = time_int_us - (time_int_us % minor_frame_us)
        slots.append((int(time_int_us/minor_frame_us)))
   
    return slots
    
# Draw a histogram of the slots needed to run a specific code segment.
# This is useful for visualizing the amount of timing variability in a given run,
# and to make sure the worst-case timing is not being determined by an outlier
# that may be caused by incorrect configuration of the system. 
def PrintHist(name, slots):
    min_slots = min(slots)
    max_slots = max(slots)
    total_count = len(slots)
    print(" ")
    print("%s" % name)
    print("-----------------------------------------")
    print "Slots | # Measurements"
    for slot in range(min_slots, max_slots + 1):
        print "%02d    | " % (slot),
        count = slots.count(slot)
        num_dots = int(math.ceil((float(count)/total_count) * HIST_MAX_WIDTH))
        for i in range(0, num_dots):
            sys.stdout.write('▒')
        print " %s" % (count)
    print(" ")
    print("Max slots: %d") % (max_slots)
    return

