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
from matplotlib import pyplot as plt
from matplotlib import dates as mdt
from matplotlib import pylab as pyl

# Add the provided suffix to each table in the source directory, then
# move the tables to the destination directory.
def MoveTables(suffix, src_dir, dst_dir):
    for filename in os.listdir(src_dir):
        a,b = filename.split(".")
        if (b == "c"):
            filename_new = "%s_%s.%s" % (a, suffix, b) 
            os.system("mv %s/%s %s/%s" % (src_dir, filename, dst_dir, filename_new))
    return

def Main():

    script_dir = os.path.realpath(__file__)
    script_dir = os.path.dirname(script_dir)
    output_dir = script_dir + "/output"
    tmp_dir = script_dir + "/tmp"
    figure_dir = script_dir + "/figures"
    data_dir = script_dir + "/../../data"
    tool_dir = script_dir + "/../../scripts/compress_schedule"
    tool_output_dir = tool_dir + "/output"

    # Create directories.
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    if os.path.exists(tmp_dir):
        shutil.rmtree(tmp_dir)
    if os.path.exists(figure_dir):
        shutil.rmtree(figure_dir)
    os.makedirs(output_dir)
    os.makedirs(tmp_dir)
    os.makedirs(figure_dir)
    print "Created output directory: %s" % (output_dir)
    print "Created tmp directory: %s" % (tmp_dir)
    print "Created figures directory: %s" % (figure_dir)

    os.system('mkdir %s/execution_time_f1' % (tmp_dir))
    exec_times_f1 = [5, 10, 15, 20]
    delay_norep = []
    delay_lamport = []
    delay_turpin = []
    delay_igor = []

    os.system('%s/compress_no_rep.py %s/execution_time_f1/log_no_rep_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_no_rep.py %s/execution_time_f1/log_no_rep_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_no_rep.py %s/execution_time_f1/log_no_rep_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_no_rep.py %s/execution_time_f1/log_no_rep_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f1")

    os.system('%s/compress_igor_nofilter_ef.py %s/execution_time_f1/log_igor_nofilter_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_igor_nofilter_ef.py %s/execution_time_f1/log_igor_nofilter_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_igor_nofilter_ef.py %s/execution_time_f1/log_igor_nofilter_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_igor_nofilter_ef.py %s/execution_time_f1/log_igor_nofilter_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f1")

    os.system('%s/compress_bft_ef.py %s/execution_time_f1/log_lamport_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_bft_ef.py %s/execution_time_f1/log_lamport_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_bft_ef.py %s/execution_time_f1/log_lamport_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_bft_ef.py %s/execution_time_f1/log_lamport_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f1")

    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f1/log_turpin_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f1/log_turpin_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f1/log_turpin_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f1")
    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f1/log_turpin_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f1")

    # Generate the plot.
    fig, ax = plt.subplots()
    ax.plot(exec_times_f1, delay_norep, label='NoRep', linewidth=3.0, marker='o')
    ax.plot(exec_times_f1, delay_igor, label='IGOR', linewidth=3.0, marker='o')
    ax.plot(exec_times_f1, delay_lamport, label='OM', linewidth=3.0, marker='o')
    ax.plot(exec_times_f1, delay_turpin, label='TC', linewidth=3.0, marker='o')
    ax.set(xlabel='Execution time (ms)', ylabel='Latency (ms)', title='(a) 4 replicas (f = 1)')
    ax.ticklabel_format(axis='y', style='plain')
    ax.set_xlim([exec_times_f1[0], exec_times_f1[-1]])
    ax.set_ylim([0,40])    
    ax.legend(loc='upper left')
    ax.grid()
    fig.savefig(figure_dir + "/execute_time_f1.png")

    os.system('mkdir %s/execution_time_f2' % (tmp_dir))
    exec_times_f2 = [5, 10, 15, 20]
    delay_norep = []
    delay_lamport = []
    delay_turpin = []
    delay_igor = []

    os.system('%s/compress_no_rep.py %s/execution_time_f2/log_no_rep_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_no_rep.py %s/execution_time_f2/log_no_rep_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_no_rep.py %s/execution_time_f2/log_no_rep_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_no_rep.py %s/execution_time_f2/log_no_rep_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f2")

    os.system('%s/compress_igor_ef.py %s/execution_time_f2/log_igor_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_igor_ef.py %s/execution_time_f2/log_igor_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_igor_ef.py %s/execution_time_f2/log_igor_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_igor_ef.py %s/execution_time_f2/log_igor_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f2")

    os.system('%s/compress_bft_ef.py %s/execution_time_f2/log_lamport_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_bft_ef.py %s/execution_time_f2/log_lamport_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_bft_ef.py %s/execution_time_f2/log_lamport_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_bft_ef.py %s/execution_time_f2/log_lamport_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f2")

    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f2/log_turpin_5ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("5ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f2/log_turpin_10ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("10ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f2/log_turpin_15ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("15ms", tool_output_dir, tmp_dir + "/execution_time_f2")
    os.system('%s/compress_bft_turpin_ef.py %s/execution_time_f2/log_turpin_20ms.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("20ms", tool_output_dir, tmp_dir + "/execution_time_f2")

    # Generate the plot.
    fig, ax = plt.subplots()
    ax.plot(exec_times_f2, delay_norep, label='NoRep', linewidth=3.0, marker='o')
    ax.plot(exec_times_f2, delay_igor, label='IGOR', linewidth=3.0, marker='o')
    ax.plot(exec_times_f2, delay_lamport, label='OM', linewidth=3.0, marker='o')
    ax.plot(exec_times_f2, delay_turpin, label='TC', linewidth=3.0, marker='o')
    ax.set(xlabel='Execution time (ms)', ylabel='Latency (ms)', title='(b) 7 replicas (f = 2)')
    ax.ticklabel_format(axis='y', style='plain')
    ax.set_xlim([exec_times_f2[0], exec_times_f2[-1]])
    ax.set_ylim([0,95])    
    ax.legend(loc='upper left')
    ax.grid()
    fig.savefig(figure_dir + "/execute_time_f2.png")

    os.system('mkdir %s/sensor_data_size_f1' % (tmp_dir))
    sensor_sizes_f1 = [250, 750, 1250]
    delay_norep = []
    delay_lamport = []
    delay_turpin = []
    delay_igor = []

    os.system('%s/compress_no_rep.py %s/sensor_data_size_f1/log_no_rep_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_no_rep.py %s/sensor_data_size_f1/log_no_rep_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_no_rep.py %s/sensor_data_size_f1/log_no_rep_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")

    os.system('%s/compress_igor_nofilter_ef.py %s/sensor_data_size_f1/log_igor_nofilter_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_igor_nofilter_ef.py %s/sensor_data_size_f1/log_igor_nofilter_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_igor_nofilter_ef.py %s/sensor_data_size_f1/log_igor_nofilter_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")

    os.system('%s/compress_bft_ef.py %s/sensor_data_size_f1/log_lamport_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_bft_ef.py %s/sensor_data_size_f1/log_lamport_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_bft_ef.py %s/sensor_data_size_f1/log_lamport_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")

    os.system('%s/compress_bft_turpin_ef.py %s/sensor_data_size_f1/log_turpin_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_bft_turpin_ef.py %s/sensor_data_size_f1/log_turpin_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")
    os.system('%s/compress_bft_turpin_ef.py %s/sensor_data_size_f1/log_turpin_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f1")

    # Generate the plot.
    fig, ax = plt.subplots()
    ax.plot(sensor_sizes_f1, delay_norep, label='NoRep', linewidth=3.0, marker='o')
    ax.plot(sensor_sizes_f1, delay_igor, label='IGOR', linewidth=3.0, marker='o')
    ax.plot(sensor_sizes_f1, delay_lamport, label='OM', linewidth=3.0, marker='o')
    ax.plot(sensor_sizes_f1, delay_turpin, label='TC', linewidth=3.0, marker='o')
    ax.set(xlabel='Sensor data size (bytes)', ylabel='Latency (ms)', title='(c) 4 replicas (f = 1)')
    ax.ticklabel_format(axis='y', style='plain')
    ax.set_xlim([sensor_sizes_f1[0], sensor_sizes_f1[-1]])
    ax.set_ylim([0,40])    
    ax.legend(loc='upper left')
    ax.grid()
    fig.savefig(figure_dir + "/sensor_data_size_f1.png")

    os.system('mkdir %s/sensor_data_size_f2' % (tmp_dir))
    sensor_sizes_f2 = [250, 750, 1250]
    delay_norep = []
    delay_lamport = []
    delay_turpin = []
    delay_igor = []

    os.system('%s/compress_no_rep.py %s/sensor_data_size_f2/log_no_rep_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_no_rep.py %s/sensor_data_size_f2/log_no_rep_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_no_rep.py %s/sensor_data_size_f2/log_no_rep_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_norep.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")

    os.system('%s/compress_igor_ef.py %s/sensor_data_size_f2/log_igor_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_igor_ef.py %s/sensor_data_size_f2/log_igor_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_igor_ef.py %s/sensor_data_size_f2/log_igor_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_igor.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")

    os.system('%s/compress_bft_ef.py %s/sensor_data_size_f2/log_lamport_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_bft_ef.py %s/sensor_data_size_f2/log_lamport_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_bft_ef.py %s/sensor_data_size_f2/log_lamport_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_lamport.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")

    os.system('%s/compress_bft_turpin_ef.py %s/sensor_data_size_f2/log_turpin_250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_bft_turpin_ef.py %s/sensor_data_size_f2/log_turpin_750bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("750bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")
    os.system('%s/compress_bft_turpin_ef.py %s/sensor_data_size_f2/log_turpin_1250bytes.dat' % (tool_dir, data_dir))
    with open('%s/delay_ms.txt' % (tool_output_dir), 'r') as f:
        delay_turpin.append(int(f.read().replace('\n', '')))
    MoveTables("1250bytes", tool_output_dir, tmp_dir + "/sensor_data_size_f2")

    # Generate the plot.
    fig, ax = plt.subplots()
    ax.plot(sensor_sizes_f2, delay_norep, label='NoRep', linewidth=3.0, marker='o')
    ax.plot(sensor_sizes_f2, delay_igor, label='IGOR', linewidth=3.0, marker='o')
    ax.plot(sensor_sizes_f2, delay_lamport, label='OM', linewidth=3.0, marker='o')
    ax.plot(sensor_sizes_f2, delay_turpin, label='TC', linewidth=3.0, marker='o')
    ax.set(xlabel='Sensor data size (bytes)', ylabel='Latency (ms)', title='(d) 7 replicas (f = 2)')
    ax.ticklabel_format(axis='y', style='plain')
    ax.set_xlim([sensor_sizes_f2[0], sensor_sizes_f2[-1]])
    ax.set_ylim([0,130])    
    ax.legend(loc='upper left')
    ax.grid()
    fig.savefig(figure_dir + "/sensor_data_size_f2.png")

    os.system('cp -rp %s/* %s/.' % (tmp_dir, output_dir))
    os.system('rm -rf %s/' % (tmp_dir))

    return
    
if __name__ == "__main__":
    Main()

