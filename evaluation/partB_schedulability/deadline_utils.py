#!/usr/bin/python2

# deadline_utils.py
# Check whether tasks in a schedule meet deadlines.

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

# Return True if all deadlines in multicore taskset are met.
#
# This is a convenience function that checks whether all of the tasksets in the
# multicore taskset meet their deadlines. If yes, the function returns True.
# Otherwise it returns False.
#
# Use it on the list returned by CheckDeadlinesMulti().
#
# param[in] bad_tasks_list List returned by CheckDeadlinesMulti()
# returns True if all deadlines met, else False.
#
def AllDeadlinesMet(bad_tasks_list):
    for bad_tasks in bad_tasks_list:
        if (len(bad_tasks) > 0):
            return False
    return True

# Check whether the multicore taskset meets all deadlines.
#
# This is a convenience function that checks whether all tasksets meet their deadlines.
# If yes, the function returns an empty list of lists. Otherwise, it returns a list with one
# sub-list for each core. Each sub-list contains the names of the tasks on the corresponding
# core that did not meet their deadlines.
#
# param[in] taskset_list List of tasksets, one for each core.
# param[in] protos_list  List of lists specifying the protocol to use for each task.
# param[in] ft           "f1" for 1-fault tolerance, "f2" for 2-fault tolerance
# param[in] bft_dict     Dictionary of worst-case timing for each protocol
#
# returns List of lists, where each sub-list contains the names of tasks that don't meet
#         deadlines on the corresponding core.
#         List of emtpy lists if all tasks meet deadlines
#
def CheckDeadlinesMulti(taskset_list, protos_list, ft, bft_dict):
    bad_tasks_list = []
    for i in range(0, CONFIG_NUM_CORES):
        bad_tasks_list.append(CheckDeadlines(taskset_list[i], protos_list[i], ft, bft_dict))
    return bad_tasks_list

# Check whether the taskset for a single core meets all deadlines.
#
# This function checks whether the provided core taskset meets all its deadlines.
# If yes, then it returns an empty list. Otherwise it returns a list of the
# names of the tasks that missed their deadlines.
#
# The protos variable specifies which BFT protocol is used when checking
# the deadlines for each of the tasks.
#
# param[in] taskset  Taskset for a single core
# param[in] protos   List of BFT protocols, one to use for each task
# param[in] ft       "f1" for 1-fault tolerance, "f2" for 2-fault tolerance
# param[in] bft_dict Dictionary of worst-case timing for each protocol
#
# returns List of names of tasks that don't meet deadlines
#         Empty list if all tasks meet deadlines
# 
def CheckDeadlines(taskset, protos, ft, bft_dict):

    bad_tasks = []

    # Iterate through the tasks.
    for i in range(0, len(taskset)):

        task  = taskset[i]
        proto = protos[i]
        task_name, wcet_slots, period_slots, deadline_slots = task

        # Get properties for BFT protocol.
        f1_props, f2_props = bft_dict[proto]

        if (ft == "f1"): # 1-fault tolerance
            agree_slots, filter_slots, disperse_slots = f1_props
        else: # 2-fault tolerance
            agree_slots, filter_slots, disperse_slots = f2_props 

        # Determine first slot at which sensor data is available,
        # relative to the start of the execution.
        if (proto == "Igor"): # execution is parallel 
            sensor_slot = 0
        else: # execution not parallel
            sensor_slot = 0 - (filter_slots + agree_slots)

        # Calculate deadline slot at which all filtering, agreement,
        # and execution steps must have already completed.
        # Note that this is relative to when sensor data was received.
        actuator_slot = sensor_slot + deadline_slots

        # Calculate slot at which the protocol has completed all
        # agreement, filtering, and execution stages.
        if (proto == "Igor"): # execution is parallel 
            if (wcet_slots > (agree_slots + filter_slots)):
                finished_slot = 0 + wcet_slots
            else:
                finished_slot = 0 + (agree_slots + filter_slots)
        else: # execution not parallel
            finished_slot = 0 + wcet_slots 

        # Determine whether the task meets its deadline.
        if (finished_slot > actuator_slot):
            bad_tasks.append(task_name)
            continue

        # In addition, Igor must finish state dispersal
        # before the next invocation of the protocol.
        if (proto == "Igor"): # execution is parallel         
            if ((finished_slot + disperse_slots) > period_slots):
                bad_tasks.append(task_name)
                continue

    return (bad_tasks)

