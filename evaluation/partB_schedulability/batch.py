#!/usr/bin/python2

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
import fractions
from datetime import datetime
from copy import *

# Installed modules
from matplotlib import pyplot as plt
from matplotlib import dates as mdt
from matplotlib import pylab as pyl

# Local modules
from config import *
from schedule_utils import *
from task_utils import *
from deadline_utils import *
from log_utils import *

#
# Constants
#

SCRIPT_DIR = os.path.realpath(__file__)
SCRIPT_DIR = os.path.dirname(SCRIPT_DIR)
FIGURE_DIR = SCRIPT_DIR + "/figures"
OUTPUT_DIR = SCRIPT_DIR + "/output"
DATA_DIR = SCRIPT_DIR + "/../../data"
TOOL_DIR = SCRIPT_DIR + "/../../scripts/compress_schedule"
LOCAL_TOOL_DIR = SCRIPT_DIR + "/compress_schedule_local"
LOCAL_TOOL_OUTPUT_DIR = LOCAL_TOOL_DIR + "/output"
LOCAL_TOOL_CONFIG_DIR = SCRIPT_DIR + "/compress_schedule_config"
SCHEDULE_PATH_DEFAULT_F1 = OUTPUT_DIR + "/schedules_default_f1.txt"
SCHEDULE_PATH_IGOR_F1 = OUTPUT_DIR + "/schedules_igor_f1.txt"
SCHEDULE_PATH_DEFAULT_F2 = OUTPUT_DIR + "/schedules_default_f2.txt"
SCHEDULE_PATH_IGOR_F2 = OUTPUT_DIR + "/schedules_igor_f2.txt"

#
# Global Variables
#

#
# Functions
#

# Given a default multicore taskset, check whether it is possible to meet all
# deadlines using only the specified default protocol. Then, build a duplicate
# multicore taskset in which every task that does not meet deadlines is replaced
# by a speculative IGOR task that runs on all cores. Return whether it is
# possible to create a valid schedule with the IGOR tasks, such that all
# deadlines are met. If so, return the new schedule.
#
# param[in] multicore_taskset A list of taskets, one for each core
# param[in] schedule          Default schedule for the multicore taskset
# param[in] default_proto     Default protocols to use for all tasks
# param[in] ft                "f1" for 1-fault tolerance, "f2" for 2-fault tolerance
# param[in] bft_dict          Dictionary of worst-case timing for each protocol
#
# returns a tuple of of the form:
#         (default_met_deadlines, igor_met_deadlines, new_schedule)
#         Where the first two elements are boolean, and the last element
#         is the new schedule containing IGOR tasks. Note that the new
#         schedule is only meaningful if igor_met_deadlines is True.
#
def ProcessTasks(multicore_taskset, schedule, default_proto, ft, bft_dict):

    default_met_deadlines = False
    igor_met_deadlines    = False

    #print "Initial Multicore Taskset:"
    #for taskset in multicore_taskset:
    #    print taskset
    #print " "

    #print "Initial Schedule:"
    #SchedulePrint(schedule)
    #print " "

    # Specify that each task uses default protocol.
    multicore_protoset = []
    for taskset in multicore_taskset:
        protoset = [default_proto] * len(taskset)
        multicore_protoset.append(protoset)

    # Get a list of tasks that miss deadlines on each core.
    multicore_badset = CheckDeadlinesMulti(multicore_taskset, multicore_protoset, ft, bft_dict)
    if (AllDeadlinesMet(multicore_badset)):
        default_met_deadlines = True

    # Create a new multicore taskset that is identical to the previous taskset,
    # except each task that does not meet deadlines is replaced with
    # CONFIG_NUM_CORES speculative IGOR tasks (one per core).
    igor_multicore_taskset  = []
    igor_multicore_protoset = []
    bad_tasks  = [] # tasks that missed deadlines
    igor_tasks = [] # tasks to replace bad tasks

    # Remove all tasks that miss deadlines.
    for p in range(0, CONFIG_NUM_CORES):
        taskset  = multicore_taskset[p] 
        protoset = multicore_protoset[p] 
        badset   = multicore_badset[p]
        igor_taskset  = []
        igor_protoset = []

        # Check which tasks missed deadlines.
        for q in range(0, len(taskset)):
            task     = taskset[q]
            protocol = protoset[q]
            task_name, _, _, _ = task
            if (task_name not in badset): # task made deadlines
                igor_taskset.append(task)
                igor_protoset.append(protocol)
            else: # task missed deadlines
                bad_tasks.append(task)                        

        igor_multicore_taskset.append(igor_taskset)
        igor_multicore_protoset.append(igor_protoset)

    if (default_met_deadlines):
        assert(len(bad_tasks) == 0)

    #print "Bad Tasks:"
    #print bad_tasks
    #print " "

    # Replace each task that missed deadlines with
    # IGOR tasks that run all on cores.
    for bad_task in bad_tasks:
        task_name, wcet_slots, period_slots, deadline_slots = bad_task
        task_name = "I_" + task_name
        igor_task = (task_name, wcet_slots, period_slots, deadline_slots)
        igor_tasks.append(igor_task)

    #print "Replacement IGOR Tasks:"
    #print igor_tasks
    #print " "

    # Check whether it is possible to produce a valid schedule
    # when the IGOR tasks are included.
    new_schedule = ScheduleTasks(igor_multicore_taskset, igor_tasks)
    if (len(new_schedule) == 0):
        # If no valid schedule, also can't meet deadlines.'
        return (default_met_deadlines, False, None)

    assert(ScheduleCheck(new_schedule, igor_multicore_taskset, igor_tasks) == True)

    #print "New Schedule:"
    #SchedulePrint(new_schedule)
    #print " "

    # Add the IGOR tasks to the individual schedules for each core.
    for igor_task in igor_tasks:
        for p in range(0, CONFIG_NUM_CORES):
            igor_multicore_taskset[p].append(igor_task) 
            igor_multicore_protoset[p].append("Igor")

    # Check whether all tasks meet deadlines.
    multicore_badset = CheckDeadlinesMulti(igor_multicore_taskset, 
                                           igor_multicore_protoset,
                                           ft, bft_dict)
    if (AllDeadlinesMet(multicore_badset)):
        igor_met_deadlines = True

    return (default_met_deadlines,
            igor_met_deadlines,
            new_schedule)

# Entry point for the program.
def Main():

    # Create directories.
    if os.path.exists(FIGURE_DIR):
        shutil.rmtree(FIGURE_DIR)
    if os.path.exists(OUTPUT_DIR):
        shutil.rmtree(OUTPUT_DIR)
    if os.path.exists(LOCAL_TOOL_DIR):
        shutil.rmtree(LOCAL_TOOL_DIR)
    os.makedirs(FIGURE_DIR)
    os.makedirs(OUTPUT_DIR)
    os.makedirs(LOCAL_TOOL_DIR)
    print "Created figures directory: %s" % (FIGURE_DIR)
    print "Created output directory: %s" % (OUTPUT_DIR)
    print "Created local tools directory: %s" % (LOCAL_TOOL_DIR)

    # Copy 'compress_schedule' script to local directory.
    os.system('cp -rp %s/* %s/.' % (TOOL_DIR, LOCAL_TOOL_DIR))
    os.system('cp %s/* %s/.' % (LOCAL_TOOL_CONFIG_DIR, LOCAL_TOOL_DIR))

    # Parse logs for timing data.
    bft_dict = GetDataDict()

    # Seed random number generator.
    random.seed(datetime.now())

    # For each utilization, generate CONFIG_NUM_TASKSETS multicore tasksets,
    # each with a valid schedule that may not meet deadlines.
    print "Randomly generating tasksets ..."
    all_multicore_tasksets = []
    all_multicore_schedules = []
    for u in CONFIG_UTIL_LIST:
        print "utilization = %.1f" % (u)
        num_tasksets = CONFIG_NUM_TASKSETS
        multicore_tasksets = []
        multicore_schedules = []
        while (num_tasksets > 0):
            multicore_taskset  = GenTasksetMulti(u)
            multicore_schedule = ScheduleTasks(multicore_taskset, [])
            if (len(multicore_schedule) > 0):
                if (ScheduleCheck(multicore_schedule, multicore_taskset, []) == False):
                    print "Error: Generated schedule doesn't match taskset"
                    exit()
                multicore_tasksets.append(multicore_taskset)
                multicore_schedules.append(multicore_schedule)
                num_tasksets -= 1 
        all_multicore_tasksets.append(multicore_tasksets)
        all_multicore_schedules.append(multicore_schedules)
    print " "

    #==========================================
    # F = 1 Case
    # 

    print "Processing tasksets for f = 1 ..."

    f1_default_met_deadlines   = [0] * len(CONFIG_UTIL_LIST)
    f1_igor_met_deadlines      = [0] * len(CONFIG_UTIL_LIST)
    f1_default_successes_ratio = [0] * len(CONFIG_UTIL_LIST)
    f1_igor_successes_ratio    = [0] * len(CONFIG_UTIL_LIST)

    # Save all schedulable tasksets for future processing.
    f1_all_default_schedules = []
    f1_all_igor_schedules = []
    total_default_schedulable = 0
    total_igor_schedulable = 0

    # For each utilization, process each taskset.
    for i in range(0, len(CONFIG_UTIL_LIST)):
        multicore_tasksets  = all_multicore_tasksets[i]
        multicore_schedules = all_multicore_schedules[i]
        f1_default_schedules = []
        f1_igor_schedules = []
        for k in range(0, CONFIG_NUM_TASKSETS):
            multicore_taskset  = multicore_tasksets[k]
            multicore_schedule = multicore_schedules[k] 
            ret = ProcessTasks(multicore_taskset, multicore_schedule, "Lamport", "f1", bft_dict)
            default_met_deadlines, igor_met_deadlines, new_schedule = ret
            f1_default_met_deadlines[i] += 1 if default_met_deadlines else 0
            f1_igor_met_deadlines[i]    += 1 if igor_met_deadlines else 0
            if (default_met_deadlines):
                f1_default_schedules.append(multicore_schedule)
                total_default_schedulable += 1
            if (igor_met_deadlines):
                f1_igor_schedules.append(new_schedule)
                total_igor_schedulable += 1           

        f1_default_successes_ratio[i] = float(f1_default_met_deadlines[i])/CONFIG_NUM_TASKSETS 
        f1_igor_successes_ratio[i]    = float(f1_igor_met_deadlines[i])/CONFIG_NUM_TASKSETS 
        f1_all_default_schedules.append(f1_default_schedules)
        f1_all_igor_schedules.append(f1_igor_schedules)

    print " "
    print "Lamport alone (f = 1): Fraction of schedulable tasksets at each utilization"
    print "u   | ratio"
    print "-------------------"
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = CONFIG_UTIL_LIST[i]
        ratio = f1_default_successes_ratio[i]
        print"%.1f | %.2f" % (u, ratio)

    print " "
    print "Lamport with IGOR (f = 1): Fraction of schedulable tasksets at each utilization"
    print "u   | ratio"
    print "-------------------"
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = CONFIG_UTIL_LIST[i]
        ratio = f1_igor_successes_ratio[i]
        print"%.1f | %.2f" % (u, ratio)

    print "\nWith IGOR, %.2fx more tasksets were schedulable" % \
          (total_igor_schedulable/float(total_default_schedulable))

    # Generate the f = 1 plot.
    fig, ax = plt.subplots()
    ax.plot(CONFIG_UTIL_LIST, f1_default_successes_ratio, label='OM', linewidth=3.0, marker='o')
    ax.plot(CONFIG_UTIL_LIST, f1_igor_successes_ratio, label='OM + IGOR', linewidth=3.0, marker='o')
    ax.ticklabel_format(axis='y', style='plain')
    ax.set(xlabel='Application utilization per core', 
           ylabel='Fraction of schedulable tasksets', 
           title='(a) 4 replicas (f = 1)')
    ax.set_xlim([CONFIG_UTIL_LIST[0], CONFIG_UTIL_LIST[-1]])
    ax.set_ylim([0,1])    
    ax.legend(loc='upper right')
    ax.grid()
    fig.savefig(FIGURE_DIR + "/schedulability_f1.png")

    # Write feasible schedules to a file.
    f = open(SCHEDULE_PATH_DEFAULT_F1, 'wb')
    pickle.dump(f1_all_default_schedules, f)
    print "\nWrote default schedules to: %s" % (SCHEDULE_PATH_DEFAULT_F1)
    f.close()
    f = open(SCHEDULE_PATH_IGOR_F1, 'wb')
    pickle.dump(f1_all_igor_schedules, f)
    print "Wrote IGOR schedules to: %s" % (SCHEDULE_PATH_IGOR_F1)
    f.close()

    #==========================================
    # F = 2 Case
    # 

    print "\nProcessing tasksets for f = 2 ..."

    f2_default_met_deadlines   = [0] * len(CONFIG_UTIL_LIST)
    f2_igor_met_deadlines      = [0] * len(CONFIG_UTIL_LIST)
    f2_default_successes_ratio = [0] * len(CONFIG_UTIL_LIST)
    f2_igor_successes_ratio    = [0] * len(CONFIG_UTIL_LIST)

    # Save all schedulable tasksets for future processing.
    f2_all_default_schedules = []
    f2_all_igor_schedules = []
    total_default_schedulable = 0
    total_igor_schedulable = 0

    # For each utilization, process each taskset.
    for i in range(0, len(CONFIG_UTIL_LIST)):
        multicore_tasksets  = all_multicore_tasksets[i]
        multicore_schedules = all_multicore_schedules[i]
        f2_default_schedules = []
        f2_igor_schedules = []
        for k in range(0, CONFIG_NUM_TASKSETS):
            multicore_taskset  = multicore_tasksets[k]
            multicore_schedule = multicore_schedules[k]
            ret = ProcessTasks(multicore_taskset, multicore_schedule, "Turpin", "f2", bft_dict)
            default_met_deadlines, igor_met_deadlines, new_schedule = ret
            f2_default_met_deadlines[i] += 1 if default_met_deadlines else 0
            f2_igor_met_deadlines[i]    += 1 if igor_met_deadlines else 0
            if (default_met_deadlines):
                f2_default_schedules.append(multicore_schedule)
                total_default_schedulable += 1
            if (igor_met_deadlines):
                f2_igor_schedules.append(new_schedule)
                total_igor_schedulable += 1

        f2_default_successes_ratio[i] = float(f2_default_met_deadlines[i])/CONFIG_NUM_TASKSETS 
        f2_igor_successes_ratio[i]    = float(f2_igor_met_deadlines[i])/CONFIG_NUM_TASKSETS 
        f2_all_default_schedules.append(f2_default_schedules)
        f2_all_igor_schedules.append(f2_igor_schedules)

    print " "
    print "Turpin alone (f = 2): Fraction of schedulable tasksets at each utilization"
    print "u   | ratio"
    print "-------------------"
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = CONFIG_UTIL_LIST[i]
        ratio = f2_default_successes_ratio[i]
        print"%.1f | %.2f" % (u, ratio)

    print " "
    print "Turpin with IGOR (f = 2): Fraction of schedulable tasksets at each utilization"
    print "u   | ratio"
    print "-------------------"
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = CONFIG_UTIL_LIST[i]
        ratio = f2_igor_successes_ratio[i]
        print"%.1f | %.2f" % (u, ratio)

    print "\nWith IGOR, %.2fx more tasksets were schedulable" % \
          (total_igor_schedulable/float(total_default_schedulable))

    # Generate the f = 1 plot.
    fig, ax = plt.subplots()
    ax.plot(CONFIG_UTIL_LIST, f2_default_successes_ratio, label='TC', linewidth=3.0, marker='o')
    ax.plot(CONFIG_UTIL_LIST, f2_igor_successes_ratio, label='TC + IGOR', linewidth=3.0, marker='o')
    ax.ticklabel_format(axis='y', style='plain')
    ax.set(xlabel='Application utilization per core', 
           ylabel='Fraction of schedulable tasksets', 
           title='(b) 7 replicas (f = 2)')
    ax.set_xlim([CONFIG_UTIL_LIST[0], CONFIG_UTIL_LIST[-1]])
    ax.set_ylim([0,1])    
    ax.legend(loc='upper right')
    ax.grid()
    fig.savefig(FIGURE_DIR + "/schedulability_f2.png")

    # Write feasible schedules to a file.
    f = open(SCHEDULE_PATH_DEFAULT_F2, 'wb')
    pickle.dump(f2_all_default_schedules, f)
    print "\nWrote default schedules to: %s" % (SCHEDULE_PATH_DEFAULT_F2)
    f.close()
    f = open(SCHEDULE_PATH_IGOR_F2, 'wb')
    pickle.dump(f2_all_igor_schedules, f)
    print "Wrote IGOR schedules to: %s" % (SCHEDULE_PATH_IGOR_F2)
    f.close()

    return
    
if __name__ == "__main__":
    Main()

