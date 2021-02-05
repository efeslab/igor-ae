#!/usr/bin/python2

# schedule_utils.py
# Create simple cyclic schedules for multicore systems.

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
import random
import pickle
from copy import *

# Installed modules

# Local modules
from config import *

#
# Constants
#

#
# Global Variables
#

#
# Functions
#

# Check that provided task is correctly scheduled on given core.
#
# param[in] core_schedule  The schedule for a specific core 
# param[in] task           The task to check
#
# returns True if schedule is correct, else False.
#
def ScheduleCheckTask(core_schedule, task):
    task_name, wcet_slots, period_slots, _ = task

    # Find first occurrence of task in schedule.    
    first_slot = -1
    for i in range(0, CONFIG_SCHEDULE_SLOTS):
        if (core_schedule[i] == task_name):
            first_slot = i
            break
    if (first_slot == -1):
        print "Task '%s' not in schedule" % (task_name)
        return False

    # Check each time task is scheduled.        
    jump_slot = first_slot
    check_periods = True
    while (check_periods):
        for slot in range(jump_slot, jump_slot + wcet_slots):
            if (slot >= CONFIG_SCHEDULE_SLOTS):
                check_periods = False
                break # done checking task periods 
            if (core_schedule[slot] != task_name):
                print "Task '%s' schedule is not correct" % (task_name)
                return False
        jump_slot += period_slots

    return True

# Check that the schedule correctly represents the provided tasksets.
#
# This function checks that each core's taskset is correctly scheduled on its
# corresponding core. Also, it checks that the global taskset is correctly
# scheduled in the same slots on all cores.
#
# param[in] schedule       List of schedules, with one schedule per core. 
# param[in] tasksets       List of tasksets to schedule (one taskset per core)
# param[in] global_taskset Additional tasks to schedule on all cores
#
# returns True if schedule is correct, else False.
#
def ScheduleCheck(schedule, tasksets, global_taskset):

    # Check each core taskset.
    for i in range(0, CONFIG_NUM_CORES):
        core_schedule = schedule[i]   
        taskset = tasksets[i]
        for task in taskset:  
            if (ScheduleCheckTask(core_schedule, task) == False):
                return False

    # Check global taskset.
    for i in range(0, CONFIG_NUM_CORES):
        core_schedule = schedule[i]   
        for task in global_taskset:  
            if (ScheduleCheckTask(core_schedule, task) == False):
                return False

    # Verify that global tasks always run in 
    # the same slots on all cores.
    for i in range(0, len(global_taskset)):
        task_name, wcet_slots, period_slots, _ = global_taskset[i]
        for core in range(0, CONFIG_NUM_CORES):
            core_schedule = schedule[core]
            for slot in range(0, len(core_schedule)):
                if (core_schedule[slot] == task_name):
                    for dst_core in range(0, CONFIG_NUM_CORES):
                        dst_core_schedule = schedule[dst_core]
                        if (dst_core_schedule[slot] != task_name):
                            print "Global task '%s' runs in different slots" % (task_name)
                            return False
    return True

# Print the provided schedule.
def SchedulePrint(schedule):

    print "=========================================================="
    sys.stdout.write("%-15s" % ("Slot"))
    for i in range(0, CONFIG_NUM_CORES):
        sys.stdout.write("Core %-10d" % (i))
    sys.stdout.write("\n")        
    print "----------------------------------------------------------"
    for i in range(0, CONFIG_SCHEDULE_SLOTS):
        sys.stdout.write("%-15d" % (i))            
        for j in range(0, CONFIG_NUM_CORES):
            core_schedule = schedule[j]
            sys.stdout.write("%-15s" % (core_schedule[i] if (core_schedule[i] != None) else "--"))
        sys.stdout.write("\n")    

    return

# Generate a schedule for the provided tasksets.
#
# Tasks are scheduled in order of decreasing rate, with global tasks scheduled
# at the start of their rate groups. In addition, global tasks must be scheduled
# in the same slot on all cores. If a valid schedule cannot be created, then
# return an empty list. Otherwise, return the generated schedule.
#
# param[in] tasksets       List of tasksets to schedule (one taskset per core)
# param[in] global_taskset Additional tasks to schedule on all cores
#
# returns List of schedules, with one schedule per core.
#         Each schedule is a list of slots, each containing a task name or None.
#         Empty list if no schedule is generated.
# 
def ScheduleTasks(tasksets, global_taskset):

    # Create empty schedule to populate.
    schedule = []
    for i in range(0, CONFIG_NUM_CORES):    
        core_schedule = [None] * CONFIG_SCHEDULE_SLOTS
        schedule.append(core_schedule)

    # Loop through periods from smallest to largest.
    for period in reversed(CONFIG_TASK_PERIODS_SLOTS):

        # Schedule the global tasks first. This is done by finding the
        # first continuous set of slots that is free on all cores.
        for i in range(0, len(global_taskset)):
            task_name, wcet_slots, period_slots, _ = global_taskset[i]
            # Check if global task is in rate group.
            if (period_slots == period):
                # Find first continuous set of wcet_slots that is open between
                # slot [0, period_slots - 1] inclusive on all cores.
                task_scheduled = False
                for curr_slot in range(0, period_slots - wcet_slots + 1):
                    is_opening = True # true if continuous gap
                    for check_slot in range(curr_slot, curr_slot + wcet_slots):
                        # Check the slot on all cores.
                        for check_core in range(0, CONFIG_NUM_CORES):
                            core_schedule = schedule[check_core]
                            if (core_schedule[check_slot] != None): # slot is taken
                                is_opening = False
                                break    

                    # If continuous slots are free, claim them.        
                    if (is_opening):
                        jump_slot = curr_slot # location of first opening in first period
                        fill_periods = True
                        while (fill_periods):
                            for j in range(jump_slot, jump_slot + wcet_slots):
                                if (j >= CONFIG_SCHEDULE_SLOTS):
                                    fill_periods = False
                                    break # filled in all my periods 
                                for check_core in range(0, CONFIG_NUM_CORES):
                                    core_schedule = schedule[check_core]                                      
                                    core_schedule[j] = task_name
                            jump_slot += period_slots

                        task_scheduled = True
                        break
                        
                if (task_scheduled == False):
                    # Could not schedule global task.
                    return ([])     

        # Now schedule the rest of the tasks.
        # There is a different taskset for each core.
        for i in range(0, CONFIG_NUM_CORES):
            core_taskset  = tasksets[i]
            core_schedule = schedule[i]
            
            # Loop through tasks in taskset.
            if (None not in core_taskset):
                for task in core_taskset:
                    task_name, wcet_slots, period_slots, _ = task
                    # Check if task is in rate group.
                    if (period_slots == period):                   
                        # Find first continuous set of wcet_slots that is open between
                        # slot [0, period_slots - 1] inclusive on this core.                    
                        task_scheduled = False
                        for curr_slot in range(0, period_slots - wcet_slots + 1):
                            is_opening = True # true if continuous gap
                            for check_slot in range(curr_slot, curr_slot + wcet_slots):
                                if (core_schedule[check_slot] != None): # slot is taken
                                    is_opening = False
                                    break     

                            # If continuous slots are free, claim them.      
                            if (is_opening):
                                jump_slot = curr_slot # location of first opening in first period
                                fill_periods = True
                                while (fill_periods):
                                    for j in range(jump_slot, jump_slot + wcet_slots):
                                        if (j >= CONFIG_SCHEDULE_SLOTS):
                                            fill_periods = False
                                            break # filled in all my periods 
                                        core_schedule[j] = task_name
                                    jump_slot += period_slots

                                task_scheduled = True
                                break
                                
                        if (task_scheduled == False):
                            # Could not schedule core task.
                            return ([])
    return schedule

