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
import re
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
DATA_DIR = SCRIPT_DIR + "/data"

#
# Global Variables
#

# Return the total number of bytes from the log.
def GetBytes(log_path):
    f = open(log_path, 'r')
    for line in f:
        pass
    bytes = line # last line is total
    f.close()   
    # Strip space and newlines.
    return (int(re.sub(r"[\n\s]*", "", bytes)))


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

    print "\nProcessing data for f = 1 ..."

    # Get total bytes communicated for each protocol.
    norep_bytes   = GetBytes("%s/log_norep.txt" % (DATA_DIR))
    lamport_bytes = GetBytes("%s/log_lamport_f1.txt" % (DATA_DIR))
    turpin_bytes  = GetBytes("%s/log_turpin_f1.txt" % (DATA_DIR))
    igor_bytes    = GetBytes("%s/log_igor_f1.txt" % (DATA_DIR))

    print " "
    print "f = 1: Total bytes transmitted"
    print "NoRep: %d" % (norep_bytes)
    print "OM:    %d" % (lamport_bytes)
    print "TC:    %d" % (turpin_bytes)
    print "IGOR:  %d" % (igor_bytes)

    # Generate the f = 1 plot.
    print "\nGenerating figure %s/communication_f1.png ..." % (FIGURE_DIR)
    fig, ax = plt.subplots()
    barwidth = 1
    ax.bar(0, norep_bytes, color='black', width=barwidth, edgecolor='white', label='NoRep')
    ax.bar(1, lamport_bytes, color='green', width=barwidth, edgecolor='white', label='OM')
    ax.bar(2, turpin_bytes, color='purple', width=barwidth, edgecolor='white', label='TC')
    ax.bar(3, igor_bytes, color='red', width=barwidth, edgecolor='white', label='IGOR')
    ax.set(ylabel='Bytes transmitted', 
           title='(a) 4 replicas (f = 1)')
    ax.set_xticks([.5, 1.5, 2.5, 3.5])
    ax.set_xticklabels(['NoRep', 'OM', 'TC', 'IGOR'])
    ax.set_xlim([0, 4])  
    ax.set_ylim([0,5000000])    
    fig.savefig(FIGURE_DIR + "/communication_f1.png")

    #==========================================
    # F = 1 Case
    # 

    print "\nProcessing data for f = 2 ..."

    # Get total bytes communicated for each protocol.
    norep_bytes   = GetBytes("%s/log_norep.txt" % (DATA_DIR))
    lamport_bytes = GetBytes("%s/log_lamport_f2.txt" % (DATA_DIR))
    turpin_bytes  = GetBytes("%s/log_turpin_f2.txt" % (DATA_DIR))
    igor_bytes    = GetBytes("%s/log_igor_f2.txt" % (DATA_DIR))

    print " "
    print "f = 2: Total bytes transmitted"
    print "NoRep: %d" % (norep_bytes)
    print "OM:    %d" % (lamport_bytes)
    print "TC:    %d" % (turpin_bytes)
    print "IGOR:  %d" % (igor_bytes)

    # Generate the f = 1 plot.
    print "\nGenerating figure %s/communication_f2.png ..." % (FIGURE_DIR)
    fig, ax = plt.subplots()
    barwidth = 1
    ax.bar(0, norep_bytes, color='black', width=barwidth, edgecolor='white', label='NoRep')
    ax.bar(1, lamport_bytes, color='green', width=barwidth, edgecolor='white', label='OM')
    ax.bar(2, turpin_bytes, color='purple', width=barwidth, edgecolor='white', label='TC')
    ax.bar(3, igor_bytes, color='red', width=barwidth, edgecolor='white', label='IGOR')
    ax.set(ylabel='Bytes transmitted', 
           title='(a) 7 replicas (f = 2)')
    ax.set_xticks([.5, 1.5, 2.5, 3.5])
    ax.set_xticklabels(['NoRep', 'OM', 'TC', 'IGOR'])
    ax.set_xlim([0, 4])  
    ax.set_ylim([0,65000000])    
    fig.savefig(FIGURE_DIR + "/communication_f2.png")

    return
    
if __name__ == "__main__":
    Main()

