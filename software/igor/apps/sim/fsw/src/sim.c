/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "cfe_platform_cfg.h"

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#include "msg_ids.h"

#include "comp_lib.h"
#include "io_lib.h"
#include "vote_lib.h"
#include "log_lib.h"
#include "log_codes.h"

/*
** Function Declarations
*/

void SIM_AppMain(void);
void SIM_AppInit(void);

/** 
** Calculate difference between two timestamps in milliseconds.
**
** \param[in] start Starting timestamp.
** \param[in] stop  Ending timestamp.
**
** \return Difference between timestamps.
*/
static double SIM_TimeDiffMs(struct timespec start,
                             struct timespec stop);
                        
/*
** Constants
*/

#define APP_NAME "SIM"

#define WAKEUP_PIPE_NAME  "SIM Pipe"
#define WAKEUP_PIPE_DEPTH (50)

/*
** Global Variables
*/

static CFE_SB_PipeId_t g_WakeupPipe;
static CFE_SB_MsgPtr_t g_MsgPtr;

/*
** Function Definitions
*/

/* Calculate difference between two timestamps in milliseconds. */
double SIM_TimeDiffMs(struct timespec start,
                      struct timespec stop)
{
    double time;
    time = (stop.tv_sec - start.tv_sec) * 1e9; /* time in seconds as ns */
    time = (time + (stop.tv_nsec - start.tv_nsec)) * 1e-6; /* add in ns, convert to ms */
    return time;
}

/* Entry point for the application. */
void SIM_AppMain(void)
{
    CFE_SB_MsgId_t msg_id;
    int32  status;
    uint32 RunStatus = CFE_ES_APP_RUN;
    
    uint8 sensor_data[IO_MAX_DATA_SIZE];                       /* sensor data to send */
    uint8 actuator_data[IO_MAX_DATA_SIZE * MAX_REPLICA_COUNT]; /* actuator data received */
    bool got_data[MAX_REPLICA_COUNT];                          /* true if got output from replica */  
    size_t msg_len = 0;                                        /* length of message read */ 
    int ret = 0;   
    int i = 0;

    uint8 result_check[IO_MAX_DATA_SIZE]; /* buffer to check results */
    void* pp_vals[MAX_REPLICA_COUNT];     /* pointer to values to vote */ 
    size_t num = 0;                       /* number of values to vote */
    void* p_result = NULL;                /* pointer to final result */        

    struct timespec start;  /* for timestamping */
    struct timespec stop;   /* for timestamping */    
    double avg_latency = 0; /* average end-to-end latency */
    int num_iterations = 0; /* number of iterations performed */   
                                
    assert(SENSOR_DATA_SIZE <= IO_MAX_DATA_SIZE);            
    assert(ACTUATOR_DATA_SIZE <= IO_MAX_DATA_SIZE);
                                            
    SIM_AppInit();

    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {        
        /* Wait for wakeup message - no timeout. */
        status = CFE_SB_RcvMsg(&g_MsgPtr, g_WakeupPipe, CFE_SB_PEND_FOREVER);

        if (status == CFE_SUCCESS) {
        
            msg_id = CFE_SB_GetMsgId(g_MsgPtr);       
            switch (msg_id) {
            
                case (SIM_SEND_SENSOR_MID):
                
                    /* Generate random input to send. */
                    for (i = 0; i < SENSOR_DATA_SIZE; i++) {
                        sensor_data[i] = rand() % 256;
                    }                

                    printf("%s: Sending data to replicas ...\n", APP_NAME);
                                 
                    /* Send the input from each sensor. */
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        IO_Send_Sensor2Replicas(i, sensor_data, SENSOR_DATA_SIZE);
                        if (SIM_MODE == SIM_MODE_NO_REP) {
                            break; /* if no replication, only send from sensor 0 */
                        }                                         
                    }                                  

                    clock_gettime(CLOCK_MONOTONIC, &start);

                    break;
                
                case (SIM_READ_ACTUATOR_MID):

                    /* Read output from all replicas. */
                    memset(got_data, 0x0, sizeof(got_data));
                    for (i = 0; i < REPLICA_COUNT; i++) {
                        ret = IO_Recv_Replica2Actuators(i, &(actuator_data[i * IO_MAX_DATA_SIZE]), &msg_len);
                        if ((ret == IO_SUCCESS) && (msg_len == ACTUATOR_DATA_SIZE)) {
                            //printf("%s: Got data from replica %d\n", APP_NAME, i);
                            got_data[i] = true;
                        }
                        if (SIM_MODE == SIM_MODE_NO_REP) {
                            break; /* if no replication, only read replica 0 */
                        }
                    }
                    
                    switch (SIM_MODE) {
                        p_result = NULL;
                        
                        case (SIM_MODE_NO_REP):
                            if (got_data[0]) {
                                p_result = &(actuator_data[0]);
                            }
                            break;
                          
                        default:
                        
                            num = 0;
                            for (i = 0; i < REPLICA_COUNT; i++) {
                                if (got_data[i]) {
                                    pp_vals[num] = &(actuator_data[i * IO_MAX_DATA_SIZE]);
                                    num++;
                                }
                            }
                            
                            /* Vote results from the replicas. */
                            ret = VOTE_FindMaj(pp_vals, num, ACTUATOR_DATA_SIZE, &p_result);
                            if (ret != VOTE_SUCCESS) {
                                p_result = NULL;
                            }                           
                                                    
                            break;                
                    }

                    clock_gettime(CLOCK_MONOTONIC, &stop);

                    if (p_result != NULL) {
                        COMP_Execute(0, sensor_data, SENSOR_DATA_SIZE, result_check, ACTUATOR_DATA_SIZE);
                        if (memcmp(p_result, result_check, ACTUATOR_DATA_SIZE) == 0) {
                            printf("%s: Result from replicas is correct!\n", APP_NAME);
                        } else {
                            printf("%s: Error - result from replicas is wrong\n", APP_NAME);
                            printf("%s: Terminating ...\n", APP_NAME);
                            return;          
                        }
                    } else {
                        printf("%s: Error - got no result\n", APP_NAME);
                        printf("%s: Terminating ...\n", APP_NAME);
                        return;
                    }
                    
                    avg_latency += SIM_TimeDiffMs(start, stop);
                    num_iterations++;
                    if (num_iterations == SIM_NUM_ITER) {
                        avg_latency = avg_latency/num_iterations;
                        printf("--------------------------\n");
                        printf("%s: Average latency: %lf ms (%d iterations)\n", 
                               APP_NAME, avg_latency, num_iterations);                        
                        printf("%s: Waiting for CTRL-C to terminate\n", APP_NAME);
                        printf("--------------------------\n");
                    }                    

                default:
                    /* Should never happen */
                    break;
            }
        }
    }

    CFE_ES_ExitApp(RunStatus);
    
    return;
}

/* Initialize the application. */
void SIM_AppInit(void)
{
    time_t t;
    
    /* Register app with Executive services. */
    CFE_ES_RegisterApp();

    /* Seed random number generator. */
    srand((unsigned)time(&t));    
    
    /* Create wakeup pipe. */
    /* Subscribe to wakeup messages. */
    CFE_SB_CreatePipe(&g_WakeupPipe, WAKEUP_PIPE_DEPTH, WAKEUP_PIPE_NAME);
    CFE_SB_Subscribe(SIM_SEND_SENSOR_MID, g_WakeupPipe);
    CFE_SB_Subscribe(SIM_READ_ACTUATOR_MID, g_WakeupPipe);
    
    printf("%s: App Initialized\n", APP_NAME);

    return;				
}

