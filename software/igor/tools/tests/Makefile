include config.mak

COPTS += -Wall -O3 $(DEFS)
COPTS += -DREPLICA_INDEX=$(REPLICA_INDEX) -DCFE_PLATFORM_CPU_ID=$(CFE_PLATFORM_CPU_ID)
LIBS  := -lm -lpthread

# Where to look for header files
INCLUDES := -I.
INCLUDES += -I./cfs_incs
INCLUDES += -I../../cfe/fsw/cfe-core/src/inc
INCLUDES += -I../../osal/src/os/inc
INCLUDES += -I../../psp/fsw/inc
INCLUDES += -I../../psp/fsw/pc-linux/inc
INCLUDES += -I../../igor_defs
INCLUDES += -I../../igor_defs/tables
INCLUDES += -I../../apps/afdx_lib/fsw/src
INCLUDES += -I../../apps/bcast_lib/fsw/src
INCLUDES += -I../../apps/vote_lib/fsw/src
INCLUDES += -I../../apps/comp_lib/fsw/src
INCLUDES += -I../../apps/io_lib/fsw/src
INCLUDES += -I../../apps/select_lib/fsw/src
INCLUDES += -I../../apps/exchange_lib/fsw/src
INCLUDES += -I../../apps/state_lib/fsw/src

SOURCES := ../../apps/afdx_lib/fsw/src/afdx_lib.c
SOURCES += ../../apps/afdx_lib/fsw/src/afdx_api.c
SOURCES += ../../igor_defs/tables/afdx_lib_vls.c
SOURCES += ../../apps/bcast_lib/fsw/src/bcast_lib.c
SOURCES += ../../apps/vote_lib/fsw/src/vote_lib.c
SOURCES += ../../apps/comp_lib/fsw/src/comp_lib.c
SOURCES += ../../apps/io_lib/fsw/src/io_lib.c
SOURCES += ../../apps/select_lib/fsw/src/select_lib.c
SOURCES += ../../apps/exchange_lib/fsw/src/exchange_lib.c
SOURCES += ../../apps/state_lib/fsw/src/state_lib.c

all: tests.bin
all: afdx_send.bin afdx_recv.bin
all: bcast_send.bin bcast_recv.bin
all: bcast_send_parallel.bin bcast_recv_parallel.bin
all: exchange_parallel.bin
all: state_parallel.bin

clean:
	rm -f *.o *.bin *.dat
	
tests.bin: tests.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)
	
afdx_send.bin: afdx_send.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)
	
afdx_recv.bin: afdx_recv.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)
	
bcast_send.bin: bcast_send.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)
	
bcast_recv.bin: bcast_recv.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)	

bcast_send_parallel.bin: bcast_send_parallel.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)
	
bcast_recv_parallel.bin: bcast_recv_parallel.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)	

exchange_parallel.bin: exchange_parallel.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)	
	
state_parallel.bin: state_parallel.c $(SOURCES)
	gcc $(COPTS) $(INCLUDES) $^ -o $@ $(LIBS)
	
