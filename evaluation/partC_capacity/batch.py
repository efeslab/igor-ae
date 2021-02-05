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
import pickle
from datetime import datetime
from copy import *

# Installed modules
import numpy as np
from matplotlib import pyplot as plt
from matplotlib import dates as mdt
from matplotlib import pylab as pyl

# Local modules

#
# Constants
#

SCRIPT_DIR = os.path.realpath(__file__)
SCRIPT_DIR = os.path.dirname(SCRIPT_DIR)
FIGURE_DIR = SCRIPT_DIR + "/figures"
INPUT_DIR = SCRIPT_DIR + "/../partB_schedulability"
SCHEDULE_PATH_DEFAULT_F1 = INPUT_DIR + "/output/schedules_default_f1.txt"
SCHEDULE_PATH_IGOR_F1 = INPUT_DIR + "/output/schedules_igor_f1.txt"
SCHEDULE_PATH_DEFAULT_F2 = INPUT_DIR + "/output/schedules_default_f2.txt"
SCHEDULE_PATH_IGOR_F2 = INPUT_DIR + "/output/schedules_igor_f2.txt"

# Copy 'config.py' file to local directory.
# Keeps this script consistent with schedulability script.
os.system('cp %s/config.py %s/.' % (INPUT_DIR, SCRIPT_DIR))
from config import *

#
# Global Variables
#

#
# Functions
#
# Return the average of the numbers in the list.
def avg(mylist):
    if (len(mylist) > 0):
        return (sum(mylist)/float(len(mylist)))
    else:
        return 0

# Given a core schedule, return the capacity remaining on that core.
def GetCapacityCore(core_schedule):

    total_slots = 0
    used_slots = 0

    for slot in core_schedule:
        total_slots += 1
        if (slot != None):
            used_slots += 1

    return ((total_slots - used_slots)/float(total_slots))

# Given a multicore schedule, return the average capacity remaining per core.
def GetAvgCapacityCore(schedule):
    capacity_core_list = []
    for i in range(0, CONFIG_NUM_CORES):
        core_schedule = schedule[i]
        capacity_core = GetCapacityCore(core_schedule)
        capacity_core_list.append(capacity_core)
    return (avg(capacity_core_list))

# Entry point for the program.
def Main():

    # Create directories.
    if os.path.exists(FIGURE_DIR):
        shutil.rmtree(FIGURE_DIR)
    os.makedirs(FIGURE_DIR)
    print "Created figures directory: %s" % (FIGURE_DIR)

    #==========================================
    # F = 1 Case
    # 

    print "\nProcessing schedules for f = 1 ..."

    # Read feasible schedules from a file.
    f = open(SCHEDULE_PATH_DEFAULT_F1, 'rb')
    f1_all_default_schedules = pickle.load(f)
    print "\nRead default schedules from: %s" % (SCHEDULE_PATH_DEFAULT_F1)
    f.close()
    f = open(SCHEDULE_PATH_IGOR_F1, 'rb')
    f1_all_igor_schedules = pickle.load(f)
    print "Read IGOR schedules from: %s" % (SCHEDULE_PATH_IGOR_F1)
    f.close()

    # Calculate average remaining capacity per core (default)
    all_avg_capacity_remain_core_default = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        f1_default_schedules = f1_all_default_schedules[i]
        avg_capacity_remain_core_list = []
        for schedule in f1_default_schedules:
            avg_capacity_remain_core_list.append(GetAvgCapacityCore(schedule))
        all_avg_capacity_remain_core_default.append(avg(avg_capacity_remain_core_list))

    # Calculate average remaining capacity per core (IGOR)
    all_avg_capacity_remain_core_igor = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        f1_igor_schedules = f1_all_igor_schedules[i]
        avg_capacity_remain_core_list = []
        for schedule in f1_igor_schedules:
            avg_capacity_remain_core_list.append(GetAvgCapacityCore(schedule))
        all_avg_capacity_remain_core_igor.append(avg(avg_capacity_remain_core_list))

    print " "
    print "f = 1: Average remaining capacity per core"
    print "u   | OM   | OM + IGOR"
    print "-------------------"
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = CONFIG_UTIL_LIST[i]
        print "%.1f | %.2f | %.2f" % \
              (u, all_avg_capacity_remain_core_default[i], all_avg_capacity_remain_core_igor[i])

    # Calculate reduction in capacity from IGOR at each utilization.
    all_capacity_reduction = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        default_remain = all_avg_capacity_remain_core_default[i]
        igor_remain = all_avg_capacity_remain_core_igor[i]
        if (default_remain > 0):
            all_capacity_reduction.append((default_remain - igor_remain)/float(default_remain) * 100.0)
    print "\nWith IGOR, capacity is reduced by %.2f -- %.2f percent" % \
        (min(all_capacity_reduction), max(all_capacity_reduction))

    # Prepare data to plot, matching axis from paper.
    u_data = []
    default_data = []
    igor_data = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = round(CONFIG_UTIL_LIST[i], 1) # fix python float precision
        if (u in [.1,.3,.5,.7,.9]): # match axis in paper 
            u_data.append(u)
            default_data.append(all_avg_capacity_remain_core_default[i])
            igor_data.append(all_avg_capacity_remain_core_igor[i])

    # Generate the f = 1 plot.
    print "\nGenerating figure %s/capacity_f1.png ..." % (FIGURE_DIR)
    fig, ax = plt.subplots()
    barwidth = 0.25
    r1 = np.arange(len(u_data))
    r2 = [x + barwidth for x in r1]
    ax.bar(r1, default_data, color='green', width=barwidth, edgecolor='white', label='OM')
    ax.bar(r2, igor_data, color='red', width=barwidth, edgecolor='white', label='OM + IGOR')
    ax.set(xlabel='Application utilization per core', 
           ylabel='Avg. remaning capacity per core', 
           title='(a) 4 replicas (f = 1)')
    ax.set_xticks([.25, 1.25, 2.25, 3.25, 4.25])
    ax.set_xticklabels(['0.1', '0.3', '0.5', '0.7', '0.9'])
    ax.set_xlim([-0.25,4.75])  
    ax.set_ylim([0,1])    
    ax.legend(loc='upper right')
    fig.savefig(FIGURE_DIR + "/capacity_f1.png")

    #==========================================
    # F = 2 Case
    # 

    print "\nProcessing schedules for f = 2 ..."

    # Read feasible schedules from a file.
    f = open(SCHEDULE_PATH_DEFAULT_F2, 'rb')
    f2_all_default_schedules = pickle.load(f)
    print "\nRead default schedules from: %s" % (SCHEDULE_PATH_DEFAULT_F2)
    f.close()
    f = open(SCHEDULE_PATH_IGOR_F2, 'rb')
    f2_all_igor_schedules = pickle.load(f)
    print "Read IGOR schedules from: %s" % (SCHEDULE_PATH_IGOR_F2)
    f.close()

    # Calculate average remaining capacity per core (default)
    all_avg_capacity_remain_core_default = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        f2_default_schedules = f2_all_default_schedules[i]
        avg_capacity_remain_core_list = []
        for schedule in f2_default_schedules:
            avg_capacity_remain_core_list.append(GetAvgCapacityCore(schedule))
        all_avg_capacity_remain_core_default.append(avg(avg_capacity_remain_core_list))

    # Calculate average remaining capacity per core (IGOR)
    all_avg_capacity_remain_core_igor = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        f2_igor_schedules = f2_all_igor_schedules[i]
        avg_capacity_remain_core_list = []
        for schedule in f2_igor_schedules:
            avg_capacity_remain_core_list.append(GetAvgCapacityCore(schedule))
        all_avg_capacity_remain_core_igor.append(avg(avg_capacity_remain_core_list))

    print " "
    print "f = 2: Average remaining capacity per core"
    print "u   | TC   | TC + IGOR"
    print "-------------------"
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = CONFIG_UTIL_LIST[i]
        print "%.1f | %.2f | %.2f" % \
              (u, all_avg_capacity_remain_core_default[i], all_avg_capacity_remain_core_igor[i])

    # Calculate reduction in capacity from IGOR at each utilization.
    all_capacity_reduction = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        default_remain = all_avg_capacity_remain_core_default[i]
        igor_remain = all_avg_capacity_remain_core_igor[i]
        if (default_remain > 0):
            all_capacity_reduction.append((default_remain - igor_remain)/float(default_remain) * 100.0)
    print "\nWith IGOR, capacity is reduced by %.2f -- %.2f percent" % \
        (min(all_capacity_reduction), max(all_capacity_reduction))

    # Prepare data to plot, matching axis from paper.
    u_data = []
    default_data = []
    igor_data = []
    for i in range(0, len(CONFIG_UTIL_LIST)):
        u = round(CONFIG_UTIL_LIST[i], 1) # fix python float precision
        if (u in [.1,.3,.5,.7,.9]): # match axis in paper 
            u_data.append(u)
            default_data.append(all_avg_capacity_remain_core_default[i])
            igor_data.append(all_avg_capacity_remain_core_igor[i])

    # Generate the f = 1 plot.
    print "\nGenerating figure %s/capacity_f2.png ..." % (FIGURE_DIR)
    fig, ax = plt.subplots()
    barwidth = 0.25
    r1 = np.arange(len(u_data))
    r2 = [x + barwidth for x in r1]
    ax.bar(r1, default_data, color='purple', width=barwidth, edgecolor='white', label='TC')
    ax.bar(r2, igor_data, color='red', width=barwidth, edgecolor='white', label='TC + IGOR')
    ax.set(xlabel='Application utilization per core', 
           ylabel='Avg. remaning capacity per core', 
           title='(a) 7 replicas (f = 2)')
    ax.set_xticks([.25, 1.25, 2.25, 3.25, 4.25])
    ax.set_xticklabels(['0.1', '0.3', '0.5', '0.7', '0.9'])
    ax.set_xlim([-0.25,4.75])  
    ax.set_ylim([0,1])    
    ax.legend(loc='upper right')
    fig.savefig(FIGURE_DIR + "/capacity_f2.png")

    return
    
if __name__ == "__main__":
    Main()

