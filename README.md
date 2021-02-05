# IGOR (RTAS '21)

This repository contains artifacts for our RTAS '21 paper on IGOR, an approach
for accelerating BFT SMR by eagerly executing on sensor data on multiple cores.

The repo is open source and available at `https://github.com/efeslab/igor-ae`.

The repo contains the following directories.

- `data/` - raw timing data logged from our prototype
- `evaluation/` - scripts for reproducing results in our paper evaluation
- `paper/` - a copy of the IGOR paper
- `scripts/` - scripts used for timing analysis and visualization
- `setup/` - a schematic of the circuit we used for time synchronization in our prototype
- `software/` - a Core Flight System project with our implementation source code

We split this README file into two main sections.

The [first section](#running-igor) describes how to set up the RPi cluster used
in our evaluation, as well as how to build and run the IGOR software. This
section is intended for researchers seeking to test and extend IGOR in their
own environment.

**For AE Committee:** The [second section](#repeating-results) describes how to
repeat the key results from the IGOR paper. This section is intended primarily
for the artifact evaluation committee.


## Deploying IGOR

This section is intended for researchers setting up their own RPi cluster in
order to test or extend IGOR.


### Setup

Our implementation of IGOR is designed to run on a cluster of RPi 3B+ single
board computers. One RPi emulates sensors and actuators. The other RPis act as 
replicas.  We tested IGOR with RPis running Raspbian 9.4 with kernel version
4.14.34 and the PREEMPT RT patch.

#### Network setup

The RPis are connected by an Ethernet switch. The IP addresses of the RPis are
defined in the `AFDX_LIB_IpTable` array defined in `igor-ae/software/igor/igor_defs/tables/afdx_lib_vls.c`.

```c
AFDX_IpEntry_t AFDX_LIB_IpTable[CPU_COUNT] =
{
    {1,  "10.0.0.21"}, /* replica 0 */
    {2,  "10.0.0.22"}, /* replica 1 */
    ...
    {11, "10.0.0.31"}, /* sim interface */     
};
```

Before running IGOR, add the following at the end of `/etc/sysctl.conf` to
increase socket buffer sizes. Then reboot the RPis.

```script
net.core.rmem_default=512000
net.core.wmem_default=512000
net.core.rmem_max=512000
net.core.wmem_max=512000
```
#### Timing setup

The RPis must be synchronized via an external timimg circuit. The flight software
performs one processing step each time the voltage level on GPIO pin 4 is changed.
We provide a diagram of the timimg circuit in `igor-ae/setup/`.

Adjust the resistor and capacitor values to change the schedule frequency. The
cFS scheduler is currently configured to run at a 500 Hz rate (see `software/igor/igor_defs/sch_sync_platform_cfg.h`).
That means the circuit needs a frequency of 250 Hz.
To get approximately this rate, you can use Ra = 1 kΩ, Rb = 3 MΩ, and C = 1 nF. 

#### Additional Setup

Before running IGOR, it is necessary to increase the number of threads that one
task can spawn. Do this by adding the following to the bottom of 
`/etc/systemd/logind.conf`.  Then reboot the RPis.

```script
UserTasksMax=infinity
```

**Note:** Always safely shutdown the RPis before powering them off. Otherwise, it is
easy to corrupt the SD cards. Safely shutdown with the following.

```script
$ sudo shutdown -h now
```


### Steps to minimize timing variability

To minimize timing variability, it is necessary to perform additional configuration
on the RPis. Specifically, it is necessary to:

#### Isolate a core for inter-replica communication

This implementation assumes core 0 is isolated for I/O threads. To prevent the
Linux scheduler from running tasks on core 0, add the following ot `/boot/cmdline.txt`
on each RPi, then reboot the RPis.

```script
isolcpus=0
```
#### Slow down background tasks

Certain background tasks will cause unnecessary timing variability. One such culprit
is `x2gocleansessions`, which will periodically consume 1% of the CPU if using X2Go.
To slow it down, run `$ sudo nano /usr/sbin/x2gocleansessions` and change `while (sleep 2)`
to while `(sleep 1000)`.

#### Built-in steps

In addition to the above, IGOR performs additional configuration changes automatically
in order to reduce timing variability. These include.

- Setting real-time task priorities. 
- Pinning processes and threads to specific cores.
- Disabling the RPi from varying clock speed automatically. 
- Preventing tasks from being forced to yield the CPU to the OS scheduler. 

**Note:** Despite these steps, other factors can also impact timing. These include the IRQ affinity settings (set via `smp_affinity` in `/proc/irq/<irq_num>/`), the runlevel for your platform, and whether the inter-replica traffic is sharing network resources with other traffic (e.g. for SSH).


### Building IGOR

IGOR is implemented in NASA's Core Flight System (cFS). The top-level cFS
directory is `software/igor`. We developed 11 supporting libraries and
applications for IGOR, which can be found in `software/igor/apps`.

- **afdx_lib** - emulates an AFDX network over standard Ethernet
- **bcast_lib** - implements reliable/Byzantine broadcast primitive
- **comp_lib** - executes mock speculative computations
- **exchange_lib** - primitives for exchanging information between replicas
- **io_lib** - primitives for communicating between sensors and replicas
- **log_lib** - used for recording activities to a log file
- **select_lib** - executes mock source selection algorithms
- **state_lib** - used for distributing state between replicas
- **vote_lib** - implements various voting functions
- **sch_sync** - application for synchronizing the execution of cFS tasks
- **sim** - emulates sensors and actuators

We also implemented the following applications for testing each BFT protocol.

- **test_bft_ef** - an agree-execute system using Lamport, Shostak, and Pease’s Oral Messages protocol (OM)
- **test_bft_turpin_ef** - an agree-execute system using Turpin and Coan's reduction protocol (TC)
- **test_igor_ef** - multi-fault version of IGOR that does use Filtering (IGOR for f > 1)
- **test_igor_nofilter_ef** - single-fault version of IGOR that does not use Filtering (IGOR for f = 1)
- **test_no_rep** - a simple non-replicated system (NoRep)


#### Configuring IGOR

Each build of IGOR is configured with two configuration files found at
`software/igor/igor_defs/cpuN_cfe_es_startup.scr` and
`software/igor/igor_defs/targets_config.cmake`.

The `cpuN_cfe_es_startup.scr` controls which applications run when cFS is started,
where N indicated the RPi that uses the startup script. Each startup file must
be edited to start the BFT protocol you want to run. For example, to run a
particular BFT protocol, edit the `cpuN_cfe_es_startup.scr` file for each replica
(e.g. `cpu1_` for the first replica, `cpu4_` for the fourth replica) and ensure
it ends in one of the following lines:

```script
CFE_APP, /cf/test_igor_nofilter_ef.so, TEST_IGOR_NOFILTER_EF_AppMain, TEST_IGOR_EF_NOFILTER, 90, 64000,  0x0, 0, 0;
CFE_APP, /cf/test_igor_ef.so,          TEST_IGOR_EF_AppMain,          TEST_IGOR_EF,          90, 64000,  0x0, 0, 0;
CFE_APP, /cf/test_bft_turpin_ef.so,    TEST_BFT_TURPIN_EF_AppMain,    TEST_BFT_TURPIN_EF,    90, 64000,  0x0, 0, 0;
CFE_APP, /cf/test_bft_ef.so,           TEST_BFT_EF_AppMain,           TEST_BFT_EF,           90, 64000,  0x0, 0, 0;
CFE_APP, /cf/test_no_rep.so,           TEST_NO_REP_AppMain,           TEST_NO_REP,           90, 64000,  0x0, 0, 0;
```
All other applications in the startup file should not be altered.
Note that all contents of the file after the first `!` character are ignored.

Next, we must edit the `targets_config.cmake` file, which contains many configuration
parameters that control the behavior of the software. The comments in the file
describe each of the configuration parameters. Below, we focus on the most
important parameters.

First, edit `SIM_MODE` according to the BFT protocol you are using. It should
be set to `SIM_MODE=SIM_MODE_NO_REP` when using the **test_no_rep** protocol,
and `SIM_MODE=SIM_MODE_VOTE` in all other cases.

Next, we must choose the cFS schedule tables that control in which slots each
task is executed. The `REPLICA_SCH_TABLE` parameter controls the task schedule
on the replica RPis. The `SIM_SCH_TABLE` parameter controls the task schedule on
the sensor/actuator RPi.

By default, these are set to "default" schedule tables located in 
`software/igor/igor_defs/tables/default`. These default tables spread each
execution of the protocol over a one second major frame, which is useful when
measuring the worst-case timing of each constituent code segment.

```script
# Specify the replica schedule table.
SET(REPLICA_SCH_TABLE "default/default_replica_table_bft_ef.c")

# Specify the sim schedule table.
SET(SIM_SCH_TABLE "default/default_sim_table.c")
```

They can easily be set to any custom tables stored in `software/igor/igor_defs/tables`.


#### Running IGOR

We build IGOR by first copying the software to each of the RPis, then building
IGOR on the RPis in parallel. We do this by running the `build-cfs-rpi.sh` script,
which can be found in `software/igor`.

```script
$ cd software/igor
$ ./build-cfs-rpi.sh
```

The `USER` and `USER_RPI` variables at the top of the script need to be set to 
to your username on your host computer (where you are copying IGOR from) and
your RPis respectively. After IGOR builds on each RPi, it will be copied to
a `cfs/` directory on the user's desktop on the RPi.

To run IGOR, ssh into each RPi (i.e. each of the replicas, and the sensor/actuator
RPi) and execute the following.

```script
$ cd ~/Desktop/cfs
$ sudo ./core-cpuN, where N is the RPi's ID.
```

Each RPi will initialize the scheduler application and start waiting for 
interrupts from the timing circuit (described in [this section](#timing-setup).
Press the push-button in the circuit to start the software on the RPis. Stop
the RPis by pressing the push-button again, then typing `CTRL-C` on each RPi.


### Processing timing data

After running IGOR, a log `log_cpu=N.dat`is produced on each RPi. The log 
contains timestamps of when each major code segment in the protocol started and
finished execution.

We provide a variety of scripts for analyzing the timing data. The scripts are
tested on Ubuntu 20.04 LTS. We provide a list of dependencies to install 
in `scripts/dependencies.txt`.

#### Visualizing the execution

To visualize the contents of the log, we developed a script `view_timing` found
in the `scripts/` directory. To run it, use the following. Note that this script
can take considerable time to run on large logs. In general, we recommend it
only be used on logs that are tens of seconds or shorter.

```script
$ cd scripts/view_timing/
$ ./main.py (then choose `log_cpu=N.dat`)
```

If you want to stop the script early, press `CTRL-C`. The `output/` directory
will be populated with a visualization of the log. Solid blocks represent the
times during which each activity (specified on the left) is performed.

#### Generating compressed schedules

Other useful scripts are the `compress_schedule` scripts, which can be used to
generate the schedule tables for a specific configuration by analyzing the
timing data produced when using the spread out "default" schedules (described
in [this section](#configuring-igor)).

Essentially, the scripts measures the worst-case execution time for each major
code segment in the protocol, then generates a compressed schedule in which 
each activity is allocated as little time as possible (with a configurable
margin).

The scripts are configured by editing the `scripts/compress_schedule/config.py`
configuration file. Other parameters that can be adjusted besides margin include
the number of slots in each one second major frame (called `MINOR_FRAME_COUNT`)
and the length of each slot (called `MINOR_FRAME_MS`). 

To run the scripts, execute the following. Note that there is a different version
of the script for each BFT protocol.

```script
$ cd scripts/compress_schedule/
$ ./compress_bft_ef.py (then choose `log_cpu=N.dat`)
```
The script produces two schedule tables, one for the replicas, and one for the
sensor/actuator RPi. To use these schedules, copy them to
`software/igor/igor_defs/tables` and specify that they should be used in
`software/igor/igor_defs/target_config.cmake`. This process is described in
[this section](#configuring-igor)).

In addition, it is necessary to set the timeouts used for each stage of the BFT
protocol to match the time allocated to each stage within the schedule. This is
necessary in cases where messages can be dropped by the network, or faulty
devices may fail to send messages. To set timeouts, edit the 
`software/igor/apps/test_X/fsw/src/test_X.c` corresponding to your protocol
to use the correct timeouts. Unfortunately, this process must currently be done
manually. In the future it should be automated.

**Note:** Because `compress_schedule` works by measuring the worst-case execution
time of each code segment over a given run, it is heavily impacted by factors that
increase timing variability (described in [this section](#steps-to-minimize-timing-variability)).

To determine whether your worst-case timing measurements are being inflated by
uncontrolled factors, set the `PRINT_HIST` parameter in `config.py` to `True`.
This will cause a histogram to be printed for each major code segment that shows
the distribution of measurements taken from the log. If a small number of 
measurements are significantly higher than the others, it is likely they are 
being inflated by [other factors](#steps-to-minimize-timing-variability)).

**Note:** Certain protocols naturally exhibit different timing distributions.
For example, **test_bft_ef** typically has wide timing distributions during the
agreement phase when tolerating multiple faults, since it needs to break
messages into multiple fragments, each of which can be delayed variably due to a
variety of factors. In contrast, **test_bft_turpin_ef** and **test_igor_ef**
typically have narrow time distributions in the agreement phase, since they each
only need to send a small number of minimally-sized messages.


## Repeating Results

This section is intended for researchers intending to reproduce results for the
IGOR paper. To minimize effort on the part of reviewers, we provide a Ubuntu
20.04 LTS virtual machine that already contains the required scripts and
dependencies outlined [above](#processing-timing-data).

The virtual machine image can be found **here**.
The username, user password, and root password are all `rtas21`.

The `igor-ae` repository has already been cloned to `/home/rtas21`.
The file structure is described at the top of this document.

### Latency

As described in Section 6A of the paper, we generated latency plots by first
measuring the worst-case latencies of each major code segment for each BFT
protocol, then generating schedules that ran each code segment in sequence with
10% margin. The end-to-end latency for each BFT protocol was determined as the time
difference in the resulting schedules between when the sensors were scheduled to
send inputs to the replicas, and the actuators were scheduled to read outputs
from the replicas.  

To get the initial timing of each code segment, we ran each protocol in each
configuration using its "default" schedule (described [here](#configuring-igor))
over 100 iterations. The raw logs produced from this activity are provided in
`~/igor-ae/data`. The first two columns of the logs contain application and
activity codes. The third column contains timestamps in microseconds. 

To generate the new schedules, we ran the `compress_schedule` script (described 
[here](#generating-compressed-schedules)) on each log. The resulting schedules
were then recompiled and tested in the IGOR software.

To repeat this compression step on the raw data and generate new schedules for
all test cases, do the following:

```script
$ cd ~/igor-ae/evaluation/partA_latency
$ ./batch.py 
```

The script will run for around 30 seconds. When it finishes, the following
directory will be populated with all the schedule tables that were generated.

```script
~/igor-ae/evaluation/partA_latency/output
```

In addition, the following directory will be populated with the latency plots
determined by measuring the generated schedules. These plots should match the
plots in **Fig. 4** of the paper.

```script
~/igor-ae/evaluation/partA_latency/figures
```


### Schedulability

As described in Section 6B of the paper, we generated latency plots by varying core
utilization from 0.1 to 1, and randomly generating 1000 tasksets for each utilization.
We scheduled tasksets in periodic rate groups from smallest period to largest. For
each taskset, we determined if the taskset was schedulable with the default BFT protocol
(OM for f = 1, TC for f = 2). If not, we replaced any tasks that did not meet deadlines
with speculative IGOR tasks, then determined whether the resulting taskset was schedulable.

The worst-case number of time slots needed to execute each stage of each protocol was
determined by parsing the raw timing data in `~/igor-ae/data`.

To re-process these steps, including re-processing the raw timing data,
execute the following:

```script
$ cd ~/igor-ae/evaluation/partB_schedulability
$ ./batch.py 
```

The script will randomly generate the tasksets and report the fraction of
schedulable tasksets at each utilization.

In addition, the following directory will be populated with the resulting
schedulability plots. These plots should be similar to **Fig. 5** of the paper.
Note that there will be some differences, since the tasksets are randomly
generated for each execution.

```script
~/igor-ae/evaluation/partB_schedulability/figures

```


### Computation Overhead

As described in Section 6C of the paper, we evaluated IGOR's 'computation
overhead by taking those schedules generated in Section 6B (using the default
BFT protocol alone, as well as using IGOR for unschedulable tasks), and for each
utilization, calculating the average CPU capacity remaining per core. Note that
unschedulable tasksets were excluded from the calculation.

To repeat these steps, execute the following:

```script
$ cd ~/igor-ae/evaluation/partC_capacity
$ ./batch.py 
```

The scripts will parse the schedules generated in `partB_schedulability/` and
report the average remaining capacity per core.

In addition, the following directory will be populated with the capacity plots.
These plots should be similar to **Fig. 6** of the paper. Note that there will
be some differences, since the tasksets are randomly generated for each
execution.

```script
~/igor-ae/evaluation/partC_capacity/figures
```

A missing histogram bar means that no tasksets were schedulable at the given
utilization for the given protocol, and thus the average remaning capacity
could not be calculated.


### Communication Cost

As described in Section 6D of the paper, we evaluated IGOR's communication
overhead by running each BFT protocol for 100 iterations, sniffing the traffic
over the network, and counting the total number of bytes communicated. The
program we used for sniffing the network is provided in `igor-ae/scripts/count_bytes`.
The program generates logs of the total number of bytes communicated. We provide
these raw logs in `igor-ae/evaluation/partD_communication/data`.

To parse the logs and compare the communication overheads of the protocols,
execute the following.

```script
$ cd ~/igor-ae/evaluation/partD_communication
$ ./batch.py 
```

The following directory will be populated with the communication overhead plots.
These plots should match **Fig. 7** in the paper.

```script
~/igor-ae/evaluation/partD_communication/figures
```

### Case Study

The flight software and simulation used in our case study are NASA proprietary
and cannot be shared. Moreover, the raw data is not approved for distribution
beyond what is included in the paper. However, our results in Section 6E are
consistent with our latency measurements from Section 6A. 

