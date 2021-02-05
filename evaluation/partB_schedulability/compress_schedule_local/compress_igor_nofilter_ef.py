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

# Installed modules
from Tkinter import Tk
from tkFileDialog import askopenfilename

# Import user-configuration. 
from config import *
from common import *

def Main(filename):

    # If no input file was provided, prompt user.
    if not filename:
        Tk().withdraw() 
        filename = askopenfilename(title="Choose log file", 
                                   initialdir=(os.path.expanduser('./input')),
                                   filetypes=(('log file', '*.dat'),))
        if not filename:
            print "No file selected ... terminating"
            exit()

    print "Selected file: %s" % (filename)

    script_dir = os.path.dirname(__file__)
    output_dir = script_dir + "/output"
    
    # Create output directory.
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)
    print "Created output directory: %s" % (output_dir)

    # Get a list of lists containing log data.
    # Each sub-list is a row from the file.
    file_data = np.loadtxt(filename, skiprows = 1)
    file_data = file_data.astype(np.int64)
    file_data = list(file_data)

    # Get a list of the times needed to run each code segment.
    # A configurable margin TIME_MARGIN is added to each measurement.
    sensor_read_us   = GetTimesUs(file_data, LOG_APP_READ_SENSOR)
    source_sel_us    = GetTimesUs(file_data, LOG_APP_SOURCE_SELECT)
    actuator_send_us = GetTimesUs(file_data, LOG_APP_SEND_ACTUATOR)
    state_con_us     = GetTimesUs(file_data, LOG_APP_STATE_CONS)
    state_disp_us    = GetTimesUs(file_data, LOG_APP_STATE_DISPERSE)
    agree_us         = GetTimesMultiUs(file_data, LOG_APP_PARALLEL_START, [LOG_APP_AGREEMENT])
    compute_us       = GetTimesMultiUs(file_data, LOG_APP_PARALLEL_START, 
                         [LOG_APP_COMPUTE_0, LOG_APP_COMPUTE_1, LOG_APP_COMPUTE_2, LOG_APP_COMPUTE_3, LOG_APP_COMPUTE_4])
   
    # Get a list of the slots needed to run each code segment.  
    sensor_read_slots   = GetSlots(sensor_read_us)
    agree_slots         = GetSlots(agree_us)        
    source_sel_slots    = GetSlots(source_sel_us)        
    compute_slots       = GetSlots(compute_us)        
    actuator_send_slots = GetSlots(actuator_send_us)    
    state_con_slots     = GetSlots(state_con_us)        
    state_disp_slots    = GetSlots(state_disp_us)        
        
    # Generate a histogram of the slots needed to run each code segment.
    if (PRINT_HIST):
        PrintHist("Sensor Read", sensor_read_slots)
        PrintHist("Agreement", agree_slots)
        PrintHist("Source Selection", source_sel_slots)
        PrintHist("Compute", compute_slots)
        PrintHist("Actuator Send", actuator_send_slots)
        PrintHist("State Consolidation", state_con_slots)
        PrintHist("State Dispersal", state_disp_slots)
        print(" ")

    # Get the maximum slots needed to run each code segment.
    max_sensor_read_slots   = max(sensor_read_slots)
    max_agree_slots         = max(agree_slots)
    max_source_sel_slots    = max(source_sel_slots)
    max_compute_slots       = max(compute_slots)
    max_actuator_send_slots = max(actuator_send_slots)      
    max_state_con_slots     = max(state_con_slots)
    max_state_disp_slots    = max(state_disp_slots)

    # Determine in which slot to schedule each activity.
    sim_send_sensor_slot       = 0
    replica_read_sensor_slot   = SIM_OFFSET_SLOTS
    replica_compute_slot       = replica_read_sensor_slot + max_sensor_read_slots
    replica_source_sel_slot    = replica_compute_slot + max_agree_slots  

    if (max_compute_slots > (max_agree_slots + max_source_sel_slots)):   
        replica_send_actuator_slot = replica_compute_slot + max_compute_slots
    else:
        replica_send_actuator_slot = replica_compute_slot + (max_agree_slots + max_source_sel_slots)     

    sim_read_actuator_slot = replica_send_actuator_slot + SIM_OFFSET_SLOTS

    total_delay_ms = (sim_read_actuator_slot - sim_send_sensor_slot) * MINOR_FRAME_MS
    print "End-to-end delay = %d ms" % (int(total_delay_ms))
    # Write max end to end latency in milliseconds.
    with open("%s/delay_ms.txt" % (output_dir), 'w') as f:
        f.write('%d' % (total_delay_ms))
    # Write max slots for Agreement, Filtering, and State Dispersal.
    with open("%s/max_slots.txt" % (output_dir), 'w') as f:
        f.write('%d, %d, %d' % (max_agree_slots, 0, max_state_disp_slots))

    print "Generating schedule tables ..."

    boilerplate = "#include \"cfe.h\"\n" + \
                  "#include \"sch_sync_tbldefs.h\"\n" + \
                  "#include \"msg_ids.h\"\n" + \
                  "\n" + \
                  "/* Schedule table for flight software. */\n" + \
                  "/* An MID of zero means the slot is not used. */\n" + \
                  "uint16 SCH_SYNC_Table[SCH_SYNC_TABLE_ENTRIES] =\n" + \
                  "{\n" + \
                  "\n"

    # Generate replica schedule table.
    f = open(output_dir + "/custom_replica_table_igor_nofilter_ef.c","w+")
    f.write(boilerplate)
    for minor in range(0, MINOR_FRAME_COUNT):    
        f.write("/* slot %d */\n" % minor)
        
        # Write first activity.
        if (minor == replica_read_sensor_slot):
            f.write("REPLICA_READ_SENSOR_MID, \n")
        elif (minor == replica_compute_slot):
            f.write("REPLICA_START_COMPUTE_MID, \n")        
        elif (minor == replica_source_sel_slot):
            f.write("REPLICA_START_SOURCE_SELECT_MID, \n") 
        elif (minor == replica_send_actuator_slot):
            f.write("REPLICA_SEND_ACTUATOR_MID, \n")                
        else:
            f.write("0, \n")
        
        # Write the other activities.
        for act in range(1, ACTIVITIES_PER_MINOR):
            f.write("0, \n")      
        f.write("\n")       
    f.write("};")                   
    f.write("\n")
    f.close()    

    # Generate sim schedule table.
    f = open(output_dir + "/custom_sim_table_igor_nofilter_ef.c","w+")
    f.write(boilerplate)
    for minor in range(0, MINOR_FRAME_COUNT):    
        f.write("/* slot %d */\n" % minor)
        
        # Write first activity.
        if (minor == sim_send_sensor_slot):
            f.write("SIM_SEND_SENSOR_MID, \n")
        elif (minor == sim_read_actuator_slot):
            f.write("SIM_READ_ACTUATOR_MID, \n")
        else:
            f.write("0, \n")
        
        # Write the other activities.
        for act in range(1, ACTIVITIES_PER_MINOR):
            f.write("0, \n")      
        f.write("\n")       
    f.write("};")                   
    f.write("\n")
    f.close()   

    return
    
if __name__ == "__main__":
    input_file = None
    if (len(sys.argv) > 1):
        input_file = sys.argv[1]
    Main(input_file)

