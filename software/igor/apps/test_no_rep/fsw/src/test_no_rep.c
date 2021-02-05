/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#define _GNU_SOURCE

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
#include <pthread.h>
#include <sched.h>

#include "msg_ids.h"

#include "comp_lib.h"
#include "io_lib.h"
#include "log_lib.h"

/*
** Function Declarations
*/

void  TEST_NO_REP_AppMain(void);
void  TEST_NO_REP_AppInit(void);

static void  TEST_NO_REP_ThreadAttrCompute(pthread_attr_t* p_attr, size_t instance);
static void* TEST_NO_REP_ComputeInstance(void* p_arg);

/*
** Constants
*/

#define APP_NAME "TEST_NO_REP"

#define WAKEUP_PIPE_NAME  "TNR Pipe"
#define WAKEUP_PIPE_DEPTH (50)

/*
** Global Variables
*/

static CFE_SB_PipeId_t g_WakeupPipe;
static CFE_SB_MsgPtr_t g_MsgPtr;

/* Message to from/to sensors and actuators. */
static uint8 g_msg_recv[IO_MAX_DATA_SIZE] = {0};
static uint8 g_msg_send[IO_MAX_DATA_SIZE] = {0};

/* Condition variable for starting computation. */
static pthread_cond_t  g_compute_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_compute_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_compute_run = 0; /* true to run compute thread */

/*
** Function Definitions
*/

/* Set the thread attributes for compute threads. */
/* Populate the given thread attribute struct with a RT FIFO scheduling */
/* policy, and with a priority that is 1 less than the current thread. */
/* Also make the thread detached, and pin it to a core based on the instance. */
void TEST_NO_REP_ThreadAttrCompute(pthread_attr_t* p_attr, size_t instance)
{
    struct sched_param param; /* parameters of current thread */
    int policy;               /* policy of the current thread */
    int priority;             /* priority of current thread */
    cpu_set_t cpus;           /* used to pin thread to core */
    size_t core_index;        /* core to pin to (1 - 3)*/

    struct sched_param priority_holder; /* thread priority to set */

    assert(p_attr != NULL);
    
    /* Pin to core 1, 2, or 3 based on instance. */
    /* Keeps the computation off core 0. */
    core_index = (instance % 3) + 1;
    
    /* Get priority of current thread. */
    pthread_getschedparam(pthread_self(), &policy, &param);
    priority = param.sched_priority; 

    /* Set attributes of new thread. */
    memset(p_attr, 0x0, sizeof(pthread_attr_t));
    memset(&priority_holder, 0x0, sizeof(priority_holder));
    pthread_attr_init(p_attr);
    CPU_ZERO(&cpus);    
    
    pthread_attr_setinheritsched(p_attr, PTHREAD_EXPLICIT_SCHED);    
    pthread_attr_setschedpolicy(p_attr, SCHED_FIFO);
    priority_holder.sched_priority = priority - 1;
    CPU_SET(core_index, &cpus);

    pthread_attr_setschedparam(p_attr, &priority_holder);   
    pthread_attr_setdetachstate(p_attr, PTHREAD_CREATE_DETACHED);   
    pthread_attr_setaffinity_np(p_attr, sizeof(cpu_set_t), &cpus);
    
    return;
}

/* Execute the computation stage. */
void* TEST_NO_REP_ComputeInstance(void* p_arg)
{
    while (1){
        pthread_mutex_lock(&g_compute_mtx);
        do {
            pthread_cond_wait(&g_compute_var, &g_compute_mtx);
        } while (g_compute_run == 0);
        
        g_compute_run = 0;       
        pthread_mutex_unlock(&g_compute_mtx);

        //====================================================
        // Logging for start happens in main loop.

        COMP_Execute(COMP_TIME_MS, g_msg_recv, SENSOR_DATA_SIZE, g_msg_send, ACTUATOR_DATA_SIZE);

        LOG_LogWrite(LOG_APP_COMPUTE_0, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}   

/* Entry point for the application. */
void TEST_NO_REP_AppMain(void)
{
    CFE_SB_MsgId_t msg_id;
    int32  status;
    uint32 RunStatus = CFE_ES_APP_RUN;
    
    pthread_attr_t custom_attr_compute; /* thread attributes computation */
    size_t msg_len = 0;                 /* length of message read */
    int ret = 0;

    assert(SENSOR_DATA_SIZE <= IO_MAX_DATA_SIZE);            
    assert(ACTUATOR_DATA_SIZE <= IO_MAX_DATA_SIZE);

    /* Used to customize thread attributes. */
    TEST_NO_REP_ThreadAttrCompute(&custom_attr_compute, 0);

    /* Thread IDs for all the worker threads. */ 
    pthread_t compute_thread;

    TEST_NO_REP_AppInit();   

    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {
        /* Wait for wakeup message - no timeout. */
        status = CFE_SB_RcvMsg(&g_MsgPtr, g_WakeupPipe, CFE_SB_PEND_FOREVER);
        if (status == CFE_SUCCESS) {
        
            msg_id = CFE_SB_GetMsgId(g_MsgPtr);       
            switch (msg_id) {

                case (SCH_INIT_DONE_MID):

                    /* Spawn the compute thread. */
                    ret = pthread_create(&(compute_thread), &custom_attr_compute, &TEST_NO_REP_ComputeInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Compute (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }

                    break;

                case (REPLICA_READ_SENSOR_MID):

                    LOG_LogWrite(LOG_APP_READ_SENSOR, LOG_ACT_APP_START);
                
                    IO_Recv_Sensor2Replicas(0, g_msg_recv, &msg_len);

                    LOG_LogWrite(LOG_APP_READ_SENSOR, LOG_ACT_APP_STOP);
                    
                    break;

                case (REPLICA_START_COMPUTE_MID):
                
                    LOG_LogWrite(LOG_APP_COMPUTE_0, LOG_ACT_APP_START);
                
                    /* Execute the computation. */
                    pthread_mutex_lock(&g_compute_mtx);                    
                    g_compute_run = 1;
                    pthread_cond_broadcast(&(g_compute_var));
                    pthread_mutex_unlock(&g_compute_mtx);
                                        
                    break;
                
                case (REPLICA_SEND_ACTUATOR_MID):
                
                    LOG_LogWrite(LOG_APP_SEND_ACTUATOR, LOG_ACT_APP_START);
                
                    IO_Send_Replica2Actuators(REPLICA_INDEX, g_msg_send, ACTUATOR_DATA_SIZE);
                    
                    LOG_LogWrite(LOG_APP_SEND_ACTUATOR, LOG_ACT_APP_STOP);                    
                    
                    break;       

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
void TEST_NO_REP_AppInit(void)
{
    /* Register app with Executive services. */
    CFE_ES_RegisterApp();

    /* Create wakeup pipe. */
    /* Subscribe to wakeup messages. */
    CFE_SB_CreatePipe(&g_WakeupPipe, WAKEUP_PIPE_DEPTH, WAKEUP_PIPE_NAME);
    CFE_SB_Subscribe(SCH_INIT_DONE_MID, g_WakeupPipe);    
    CFE_SB_Subscribe(REPLICA_READ_SENSOR_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_START_COMPUTE_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_SEND_ACTUATOR_MID, g_WakeupPipe);
    
    printf("%s: App Initialized\n", APP_NAME);

    return;				
}

