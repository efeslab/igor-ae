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
from time import sleep, mktime, strftime
import multiprocessing
from functools import partial

# Installed modules
from Tkinter import Tk
from tkFileDialog import askopenfilename
import numpy as np
# Non-interactive mode supports multiprocessing */
import matplotlib as mpl
mpl.use('agg')
from matplotlib import pyplot as plt
from matplotlib import dates as mdt
from matplotlib import pylab as pyl

# Import user-configuration. 
from config import *

# Time of first interrupt.
# Used to offset time axis so it starts at 0.
FIRST_IRQ_TIME = 0

# Keeps track of apps that continued onto the next plot.
# Used to deal with apps that span a whole plot, with no start/stop in between.
APP_RUNOVER = [False] * len(APP_DICT)

def DrawLines(ax, pos, *args, **kwargs):
    """
    Draw horizontal or vertical lines on a matplotlib plot.

    Keyword arguments:
    ax   -- 'x' for vertical lines, and 'y' for horizontal.
    pos  -- A list or array of x-axis or y-axis values.
    args -- Arguments to pass to plt.axvline() or plt.axhline().
    """
    if ax == 'x':
        for p in pos:
            plt.axvline(p, *args, **kwargs)
    else:
        for p in pos:
            plt.axhline(p, *args, **kwargs)

    return
    
def PlotApp(app_code, base_line, active_height, file_data_app):    
    """
    Plot the executions for a single app.

    Keyword arguments:
    app_code      -- Indicates application to plot for.
    base_line     -- Height of baseline for the plot.
    active_height -- Offset from baseline when app is running.
    file_data_app -- List of lists, where each sub-list contains the
                     start and stop times for the app.
    """    
    _, plot_color, plot_alpha = APP_DICT[app_code]
    for i in range(0, len(file_data_app) - 1):
        j = i + 1
        row_start = file_data_app[i]
        row_stop  = file_data_app[j] 
        if (row_start[COL_ACT_CODE] == LOG_ACT_APP_START):
            assert(row_stop[COL_ACT_CODE] == LOG_ACT_APP_STOP)
            # Get x interval to plot.
            x = [row_start[COL_TIME_USEC], row_stop[COL_TIME_USEC]]
            # Get y interval to plot.
            y1 = np.array([base_line, base_line])
            y2 = y1 + active_height
            plt.fill_between(x, y1, y2 = y2, color=plot_color, alpha=plot_alpha)   
    return
    
# Plot a chunk of the log data on a single plot.
def PlotData(output_path, file_data_chunk):

    # Point tick marks out instead of in.
    pyl.rcParams['xtick.direction'] = 'out'
    pyl.rcParams['ytick.direction'] = 'out'

    fig = plt.figure()
    ax1 = fig.add_subplot(111)
    ax1.set_autoscale_on(False)

    # Specify size of the plot.
    plot_width_inches = 7.5
    plot_dpi = 300
    plot_height_inches = (plot_width_inches * 3)/5
    plot_size_ratio = float(plot_height_inches)/plot_width_inches

    # Create dictionaries for font sizes.
    scale_factor = 25.0
    title_font = {'size':str((38/scale_factor) * plot_width_inches), 
                  'y':str(1 + ((0.08/scale_factor) * plot_width_inches)),
                  'color':'blue', 
                  'weight':'normal',
                  'style':'normal',
                  'verticalalignment':'bottom'}
    axis_font =  {'size':str((25/scale_factor) * plot_width_inches), 
                  'color':'black', 
                  'weight':'normal',
                  'style':'normal',
                  'verticalalignment':'center',
                  'horizontalalignment':'center'}
    label_font = {'size':str((18/scale_factor) * plot_width_inches), 
                  'color':'black', 
                  'weight':'normal',
                  'style':'normal',
                  'verticalalignment':'center',
                  'horizontalalignment':'left'}
    
    # Define constants that depend on the plot size.
    xaxis_label_offset = (30/scale_factor) * plot_width_inches
    xaxis_tick_label_offset = (0/scale_factor) * plot_width_inches
    xaxis_tick_label_size = (13/scale_factor) * plot_width_inches
    yaxis_label_offset = (105/scale_factor) * plot_width_inches
    yaxis_tick_label_offset = (18/scale_factor) * plot_width_inches
    yaxis_tick_label_size = (13/scale_factor) * plot_width_inches
    line_thickness = (3/scale_factor) * plot_width_inches
    line_thickness_bg = (2/scale_factor) * plot_width_inches
    line_thickness_major = (1.7/scale_factor) * plot_width_inches
    line_thickness_minor = (1.0/scale_factor) * plot_width_inches
    marker_size = (5.0/scale_factor) * plot_width_inches
    
    # Define constants that do not depend on plot size.
    # The height of the plot here is 1.
    num_apps = len(APP_DICT)             # number of apps to plot
    section_height = 1.0/num_apps        # height of each plot
    active_height = section_height * 0.8 # height of plot when "active"
    base_line = []                       # offset for x axis of each plot
    for i in range(0, num_apps):
        base_line.append(i * section_height)

    # Normalize all the chunk data so that the
    # timestamps start at time 0.
    file_data_chunk_new = []
    for row in file_data_chunk:
        file_data_chunk_new.append([row[COL_APP_CODE], row[COL_ACT_CODE], row[COL_TIME_USEC] - FIRST_IRQ_TIME])
    file_data_chunk = file_data_chunk_new

    # Get a list of scheduler interrupt times.
    # Normalize the times to start at 0.
    sch_irq_times = []
    for row in file_data_chunk:
        if (row[COL_APP_CODE] == LOG_APP_SCH):
            if (row[COL_ACT_CODE] == LOG_ACT_IRQ_RECV):
                sch_irq_times.append(row[COL_TIME_USEC])

    axis_offset  = section_height/2   # height of section midpoint
    lower_bound  = sch_irq_times[0]   # lower time data point
    upper_bound  = sch_irq_times[-1]  # highest time data point
    plot_width   = upper_bound - lower_bound
    label_offset = 0.08 * plot_width # offset of labels from left of plot

    plt.title("cFS Activity Chart", **title_font)

    plt.hold(True)    

    # Draw a vertical line at every scheduler interrupt.
    DrawLines('x', sch_irq_times, color='0.6', linewidth=line_thickness_bg)            
    
    # Loop through each app (treat scheduler separately)
    for app_code in APP_DICT:
        if (app_code != LOG_APP_SCH):

            # Get a list of start and stop times for app.
            file_data_app = []
            for row in file_data_chunk:
                if (row[COL_APP_CODE] == app_code):
                    if ((row[COL_ACT_CODE] == LOG_ACT_APP_START) or
                        (row[COL_ACT_CODE] == LOG_ACT_APP_STOP)):
                        file_data_app.append(row)

            global APP_RUNOVER

            # If app has no start or stop times, but overran the previous
            # plot, it must span this whole plot.
            if ((len(file_data_app) == 0) and (APP_RUNOVER[app_code] == True)):
                to_add        = [app_code, LOG_ACT_APP_START, lower_bound]
                file_data_app = [to_add] + file_data_app
                to_add        = [app_code, LOG_ACT_APP_STOP, upper_bound]
                file_data_app = file_data_app + [to_add]    

            # If app starts with stop, or ends with start,
            # add a fake start/stop at plot border.         
            # Mark whether the app runs over to the next plot.              
            if (len(file_data_app) != 0):  
                first_entry = file_data_app[0]            
                last_entry  = file_data_app[-1]                        
                if (first_entry[COL_ACT_CODE] == LOG_ACT_APP_STOP):
                    to_add = [app_code, LOG_ACT_APP_START, lower_bound]
                    file_data_app = [to_add] + file_data_app
                    APP_RUNOVER[app_code] = False  
                if (last_entry[COL_ACT_CODE] == LOG_ACT_APP_START):
                    to_add = [app_code, LOG_ACT_APP_STOP, upper_bound]
                    file_data_app = file_data_app + [to_add]
                    APP_RUNOVER[app_code] = True  
                                        
                # Plot the app executions.
                PlotApp(app_code, base_line[app_code], active_height, file_data_app)  

    # Draw a horizontal line at every baseline.
    DrawLines('y', base_line, color='0', linewidth=line_thickness_bg)

    ax1.set_xlim([lower_bound, upper_bound])
    ax1.set_ylim([0,1])
    ax1.set_xlabel('System Time (usec)', **axis_font)
    ax1.xaxis.labelpad = xaxis_label_offset
    ax1.set_ylabel('Applications', **axis_font)
    ax1.yaxis.labelpad = yaxis_label_offset

    # Place an x label for every scheduler interrupt.
    sch_irq_times_str = [str(i) for i in sch_irq_times] 
    plt.xticks(sch_irq_times, sch_irq_times_str)

    # Offset each X axis tick label by a constant amount.
    for tick in ax1.get_xaxis().get_major_ticks():
        tick.set_pad(xaxis_tick_label_offset)
        tick.label1 = tick._get_text1()

    # Manually set the size of each X axis tick mark label.
    for t in ax1.xaxis.get_ticklabels():
        t.set_size(xaxis_tick_label_size)

    # Use blank Y tick marks.
    y_ticks = [''] * len(APP_DICT)
    plt.yticks(base_line, y_ticks)
    
    # Offset each Y axis tick label by a constant amount.
    for tick in ax1.get_yaxis().get_major_ticks():
        tick.set_pad(yaxis_tick_label_offset)
        tick.label1 = tick._get_text1()

    # Manually set the size of each Y axis tick mark label.
    for t in ax1.yaxis.get_ticklabels():
        t.set_size(yaxis_tick_label_size)

    # Create a label for each section of the plot.
    for i in range(0, len(APP_DICT)):
        plt.text(lower_bound - label_offset, 
                 base_line[i] + axis_offset, 
                 APP_DICT[i][0], 
                 **label_font)

    # Save the figure.
    fig.set_size_inches(plot_width_inches, plot_height_inches)
    fig.savefig(output_path, dpi = plot_dpi)

    # Clear the figure and release the memory.
    plt.clf()
    plt.close()

    return
   
def Main():

    # Prompt user for input file.
    Tk().withdraw() 
    filename = askopenfilename(title="Choose log file", 
                               initialdir=(os.path.expanduser('./input')),
                               filetypes=(('log file', '*.dat'),))
                               
    filepath = os.path.dirname(filename)
    filename_nopath = os.path.basename(filename)
    filename_nopath_noext, _ = os.path.splitext(filename_nopath)
    script_dir = os.path.dirname(__file__)
    output_dir = script_dir + "/output"

    # Exit the program if no file is selected.
    if not filename:
        print "No file selected ... terminating"
        exit()
    print "Selected file: %s" % (filename)
    
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

    # Get a list of lists containing just interrupt
    # data from the scheduler.
    file_data_irq = []
    for row in file_data:
        if (row[COL_APP_CODE] == LOG_APP_SCH):
            if (row[COL_ACT_CODE] == LOG_ACT_IRQ_RECV):
                file_data_irq.append(row)
                
    # Get time of first interrupt.
    global FIRST_IRQ_TIME
    FIRST_IRQ_TIME = file_data_irq[0][COL_TIME_USEC]

    # Get a list of lists where each sub-list
    # is the start and stop indices in file_data
    # for a single plot to generate.
    file_data_indices = []
    irq_start = 0
    while True:
        irq_stop =  irq_start + MINOR_PER_PLOT     
        if (irq_stop >= len(file_data_irq)):
            break
        start = -1
        stop = -1
        for i in range(0, len(file_data)):       
            if (set(file_data[i]) == set(file_data_irq[irq_start])):
                start = i
            if (set(file_data[i]) == set(file_data_irq[irq_stop])):
                stop = i
        file_data_indices.append([start, stop])        
        irq_start = irq_stop

    # Generate a plot for each indices pair.
    i = 0
    for row in file_data_indices:
        print "Generating plot %d/%d ..." % (i, len(file_data_indices))
        plot_name = output_dir + "/plot-" + str(i) + ".png"
        i += 1 
        start = row[0]
        stop  = row[1]
        file_data_chunk = []
        for j in range(start, stop + 1):
            file_data_chunk.append(file_data[j])
        PlotData(plot_name, file_data_chunk)

    return
    
if __name__ == "__main__":
    Main()

