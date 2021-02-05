#!/usr/bin/python2

# log_utils.py
# Functions for parsing raw timing logs.

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

SCRIPT_DIR = os.path.realpath(__file__)
SCRIPT_DIR = os.path.dirname(SCRIPT_DIR)
DATA_DIR = SCRIPT_DIR + "/../../data"
LOCAL_TOOL_DIR = SCRIPT_DIR + "/compress_schedule_local"
LOCAL_TOOL_OUTPUT_DIR = LOCAL_TOOL_DIR + "/output"

#
# Global Variables
#

#
# Functions
#

# Return worst-case number of slots needed for each communication phases of a
# given BFT protocol. This is done by calling the provided compression script,
# then reading the contents of the generated 'max_slots.txt' file.
#
# param[in] script     Path to compression script
# param[in] input_file Path to log file with raw timing data to parse
# param[in] output_dir Path to directory where 'max_slots.txt' is generated
# 
# returns Max slots needed for (agreement, filtering, state Dispersal)
#
def GetMaxSlots(script, input_file, output_dir):
    os.system('%s %s' % (script, input_file))
    with open('%s/max_slots.txt' % (output_dir), 'r') as f:
        a, b, c = f.read().replace('\n', '').split(", ")
        my_tuple = (int(a), int(b), int(c))
    return deepcopy(my_tuple)

# Return a dictionary with the worst-case number of slots needed for each
# communication phase of each BFT protocol.
#
# Determined at runtime by processing raw timing data. Each entry contains two
# tuples, one for f = 1, and one for f = 2, with each tuple of the form: 
# (max agree slots, max filtering slots, max dispersal slots).
#
# returns Dictionary of worst-case timing data
#
def GetDataDict():

    bft_dict = dict()

    print " "
    tuple_f1 = GetMaxSlots("%s/compress_bft_ef.py" % LOCAL_TOOL_DIR,
                           "%s/execution_time_f1/log_lamport_5ms.dat" % DATA_DIR,
                           LOCAL_TOOL_OUTPUT_DIR)
    tuple_f2 = GetMaxSlots("%s/compress_bft_ef.py" % LOCAL_TOOL_DIR,
                           "%s/execution_time_f2/log_lamport_5ms.dat" % DATA_DIR,
                           LOCAL_TOOL_OUTPUT_DIR)
    bft_dict["Lamport"] = [tuple_f1, tuple_f2]

    tuple_f1 = GetMaxSlots("%s/compress_bft_turpin_ef.py" % LOCAL_TOOL_DIR,
                           "%s/execution_time_f1/log_turpin_5ms.dat" % DATA_DIR,
                           LOCAL_TOOL_OUTPUT_DIR)
    tuple_f2 = GetMaxSlots("%s/compress_bft_turpin_ef.py" % LOCAL_TOOL_DIR,
                           "%s/execution_time_f2/log_turpin_5ms.dat" % DATA_DIR,
                           LOCAL_TOOL_OUTPUT_DIR)
    bft_dict["Turpin"] = [tuple_f1, tuple_f2]

    tuple_f1 = GetMaxSlots("%s/compress_igor_nofilter_ef.py" % LOCAL_TOOL_DIR,
                           "%s/execution_time_f1/log_igor_nofilter_5ms.dat" % DATA_DIR,
                           LOCAL_TOOL_OUTPUT_DIR)
    tuple_f2 = GetMaxSlots("%s/compress_igor_ef.py" % LOCAL_TOOL_DIR,
                           "%s/execution_time_f2/log_igor_5ms.dat" % DATA_DIR,
                           LOCAL_TOOL_OUTPUT_DIR)
    bft_dict["Igor"] = [tuple_f1, tuple_f2]

    print " "
    print "Protocol  Worst-Case Slots (f = 1)         Worst-Case Slots (f = 2)"
    print "          Agreement, Filtering, Dispersal  Agreement, Filtering, Dispersal"
    print "--------------------------------------------------------------------------"
    for key in bft_dict:
        tuple_f1 = bft_dict[key][0]
        tuple_f2 = bft_dict[key][1]
        print "%-09s %-32s %s" % (key, str(tuple_f1), str(tuple_f2))
    print " "

    return bft_dict

