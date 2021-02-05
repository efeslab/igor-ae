/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Run an error-free agreement protocol before execution.
** Construct the agreement protocol using Turpin's binary reduction.
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
#include "bcast_lib.h"
#include "vote_lib.h"
#include "select_lib.h"
#include "log_lib.h"
#include "exchange_lib.h"

/*
** Function Declarations
*/

void TEST_BFT_TURPIN_EF_AppMain(void);
void TEST_BFT_TURPIN_EF_AppInit(void);
void TEST_BFT_TURPIN_EF_Cleanup(void);

static size_t TEST_BFT_TURPIN_EF_GetIndexExchange(size_t replica_idx, size_t sensor_idx);
static size_t TEST_BFT_TURPIN_EF_GetIndexBcast(size_t replica_idx, size_t sensor_idx);

static void   TEST_BFT_TURPIN_EF_ThreadAttrComm(pthread_attr_t* p_attr);
static void   TEST_BFT_TURPIN_EF_ThreadAttrCompute(pthread_attr_t* p_attr, size_t instance);

static void   TEST_BFT_TURPIN_EF_ExchangeConfig(void);
static void   TEST_BFT_TURPIN_EF_BcastConfig(void);

static void*  TEST_BFT_TURPIN_EF_FilteringInstance(void* p_arg);
static void*  TEST_BFT_TURPIN_EF_BcastInstance(void* p_arg);
static void*  TEST_BFT_TURPIN_EF_SelectInstance(void* p_arg);
static void*  TEST_BFT_TURPIN_EF_ComputeInstance(void* p_arg);

/*
** Constants
*/

#define APP_NAME "TEST_BFT_TURPIN_EF"

#define WAKEUP_PIPE_NAME  "TBT Pipe"
#define WAKEUP_PIPE_DEPTH (50)

/*
** Global Variables
*/

static CFE_SB_PipeId_t g_WakeupPipe;
static CFE_SB_MsgPtr_t g_MsgPtr;

/* Data I received from each sensor. */
/* The data from sensor k is stored at index (k * SENSOR_DATA_SIZE). */
static uint8 g_sensor_data[SENSOR_COUNT * SENSOR_DATA_SIZE];

/*
** Used to stored messages exchanged in Turpin round 1.
** The value Replica i sent for sensor k is stored at index:
**
** = (k * REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE) + (i * EXCHANGE_MAX_DATA_SIZE)
** = ((k * REPLICA_COUNT) + i) * EXCHANGE_MAX_DATA_SIZE
**
** Each element in a buffer is EXCHANGE_MAX_DATA_SIZE bytes.
*/
static uint8 g_turpin_rd1[SENSOR_COUNT * REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE];

/* Used to store flags indicating whether I received data in Turpin round 1. */
/* Mostly unused, since we can convert missing data to pre-determined defaults. */
static bool g_turpin_got_data_rd1[SENSOR_COUNT * REPLICA_COUNT];

/* Used to store whether I am perplexed about each sensor. */
static uint8 g_am_perplexed[SENSOR_COUNT];

/* Used to store perplexed bytes exchanged in Turpin round 2. */
static uint8 g_turpin_rd2[SENSOR_COUNT * REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE];

/* Used to store flags indicating whether I received data in Turpin round 2. */
/* Mostly unused, since we can convert missing data to pre-determined defaults. */
static bool g_turpin_got_data_rd2[SENSOR_COUNT * REPLICA_COUNT];

/* Used to store alert for each sensor. */
static uint8 g_alert[SENSOR_COUNT];

/*
** Used to stored bytes broadcasted by each replica.
** The byte broadcasted by Replica i for sensor k is stored at index:
**
** = ((i * SENSOR_COUNT) + k) * 1
**
** Each element in a buffer is 1 byte.
*/
static uint8 g_bcast_results[SENSOR_COUNT * REPLICA_COUNT * 1];

/* Consistent alert bytes for each sensor. */
/* This is determined by voting the alert bytes that were broadcasted. */
static uint8 g_final_alert[SENSOR_COUNT];

/* Consistent data from each sensor. */
/* The data from sensor k is stored at index (k * SENSOR_DATA_SIZE). */
static uint8 g_final_sensor_data[SENSOR_COUNT * SENSOR_DATA_SIZE];

/* Pointer to data selected by source selection. */
static void* p_g_selected_sensor = NULL;

/* Message to send to actuators. */
static uint8 g_msg_send[IO_MAX_DATA_SIZE] = {0};

/*
** Used for thread synchronization.
*/

/* Condition variable for starting filtering. */
static pthread_cond_t  g_filter_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_filter_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_filter_run = 0; /* true to run filter thread */
        
/* Condition variable for starting broadcast. */
static pthread_cond_t  g_bcast_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_bcast_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_bcast_run = 0; /* true to run bcast thread */

/* Condition variable for starting state selection. */
static pthread_cond_t  g_select_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_select_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_select_run = 0; /* true to run select thread */

/* Condition variable for starting computation. */
static pthread_cond_t  g_compute_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_compute_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_compute_run = 0; /* true to run compute thread */
      
/*
** Function Definitions
*/

/* Calculate an index into a global buffer thet corresponds to Replica */
/* (replica_idx) sending the data it received for sensor (sensor_idx). */
size_t TEST_BFT_TURPIN_EF_GetIndexExchange(size_t replica_idx,
                                           size_t sensor_idx)
{
    assert(replica_idx < REPLICA_COUNT);
    assert(sensor_idx < SENSOR_COUNT);
    return ((sensor_idx * REPLICA_COUNT) + replica_idx);
}

/* Calculate an index into a global buffer thet corresponds to Replica */
/* (replica_idx) broadcasting a byte corresponding to sensor (sensor_idx). */
size_t TEST_BFT_TURPIN_EF_GetIndexBcast(size_t replica_idx,
                                        size_t sensor_idx)
{
    assert(replica_idx < REPLICA_COUNT);
    assert(sensor_idx < SENSOR_COUNT);
    return ((replica_idx * SENSOR_COUNT) + sensor_idx);
}

/* Set the thread attributes for compute threads. */
/* Populate the given thread attribute struct with a RT FIFO scheduling */
/* policy, and with a priority that is 1 less than the current thread. */
/* Also make the thread detached, and pin it to a core based on the instance. */
void TEST_BFT_TURPIN_EF_ThreadAttrCompute(pthread_attr_t* p_attr, size_t instance)
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

/* Set thread attributes for communication and selection threads. */
/* Pin the thread to core 0, and make the thread detached. */
void TEST_BFT_TURPIN_EF_ThreadAttrComm(pthread_attr_t* p_attr)
{
    cpu_set_t cpus; /* used to pin thread to core */

    assert(p_attr != NULL);
    
    /* Set attributes of new thread. */
    memset(p_attr, 0x0, sizeof(pthread_attr_t));
    pthread_attr_init(p_attr);
    CPU_ZERO(&cpus);
    CPU_SET(0, &cpus);
    pthread_attr_setaffinity_np(p_attr, sizeof(cpu_set_t), &cpus);
    pthread_attr_setdetachstate(p_attr, PTHREAD_CREATE_DETACHED);   
    
    return;
}

/*==========================================*/
/*==========================================*/

/* Configure all instances of exchange. */
/* One set of instances is used for Turpin round 1 messages. */
/* The next set of instances is used for Turpin round 2 messages. */
void TEST_BFT_TURPIN_EF_ExchangeConfig(void)
{
    size_t timeout_ms = 1000; /* 1 second to avoid impacting latency test */
    size_t instance = 0;      /* instance of exchange to config */
    size_t index = 0;
    int i = 0;

    /* Double check config is valid. */
    assert((SENSOR_COUNT * 2) <= EXCHANGE_MAX_INSTANCES);
    
    /* Configure exchanges for Turpin round 1. */
    /* Each replica forwards the values it got for each sensor. */
    for (i = 0; i < SENSOR_COUNT; i++) {   
        index = TEST_BFT_TURPIN_EF_GetIndexExchange(0, i); /* start of section */
        EXCHANGE_Config(instance, REPLICA_COUNT,
                        &(g_sensor_data[i * SENSOR_DATA_SIZE]),
                        &(g_turpin_rd1[index * EXCHANGE_MAX_DATA_SIZE]),
                        &(g_turpin_got_data_rd1[index]),
                        SENSOR_DATA_SIZE, timeout_ms);
        instance++;
    }
    
    /* Configure exchanges for Turpin round 2. */
    /* Each replica forwards whether it is perplexed about each sensor. */    
    for (i = 0; i < SENSOR_COUNT; i++) {
        index = TEST_BFT_TURPIN_EF_GetIndexExchange(0, i); /* start of section */    
        EXCHANGE_Config(instance, REPLICA_COUNT,
                        &(g_am_perplexed[i * 1]),
                        &(g_turpin_rd2[index * EXCHANGE_MAX_DATA_SIZE]),
                        &(g_turpin_got_data_rd2[index]),                       
                        1, timeout_ms);
        instance++;
    }

    return;
}

/* Configure all instances of Byzantine broadcast. */
void TEST_BFT_TURPIN_EF_BcastConfig(void)
{
    size_t max_faults = floor((REPLICA_COUNT - 1.0)/3.0); /* max faults to tolerate */
    size_t timeouts_ms[MAX_REPLICA_COUNT] = {0};          /* timeouts for rounds of agreement */
    size_t index = 0;
    int i = 0;

    /* Fill in timeouts for the broadcast rounds. */
    /* We make them large to avoid impacting the latency measurements. */
    for (i = 0; i < MAX_REPLICA_COUNT; i++) {
        timeouts_ms[i] = 1000; /* 1 second */
    }
   
    /* Configure the instances of Byzantine broadcast. */
    /* We use one instance to broadcast each accept vector. */
    for (i = 0; i < REPLICA_COUNT; i++) {
        BCAST_Config(i, REPLICA_COUNT, i,
                     &(g_bcast_results[i * SENSOR_COUNT]),
                     &(g_bcast_results[i * SENSOR_COUNT]),            
                     SENSOR_COUNT, true, timeouts_ms, max_faults + 1);
    }
    
    return;
}

/*==========================================*/
/*==========================================*/

/* Execute the filtering stage for all sensor values. */
void* TEST_BFT_TURPIN_EF_FilteringInstance(void* p_arg)
{
    size_t max_faults = floor((REPLICA_COUNT - 1.0)/3.0); /* max faults to tolerate */ 
    size_t count = 0;                                     /* used for comparing values */
    size_t index = 0;                                     /* index into global buffer */
    size_t instance = 0;                                  /* for filling buffers */
    int i = 0;
    int k = 0;
 
    bool filter_run_these_rd1[EXCHANGE_MAX_INSTANCES] = {0}; /* instances to run round 1 on */
    bool filter_run_these_rd2[EXCHANGE_MAX_INSTANCES] = {0}; /* instances to run round 2 on */
    bool filter_have_data_rd1[EXCHANGE_MAX_INSTANCES] = {0}; /* says whether I have data to share */
    bool filter_have_data_rd2[EXCHANGE_MAX_INSTANCES] = {0}; /* says whether I have data to share */

    for (i = 0; i < SENSOR_COUNT; i++) {
        filter_run_these_rd1[instance] = true;
        filter_have_data_rd1[instance] = true;
        instance++;
    }    

    for (i = 0; i < SENSOR_COUNT; i++) {
        filter_run_these_rd2[instance] = true;
        filter_have_data_rd2[instance] = true;
        instance++;
    }

    while (1){
        pthread_mutex_lock(&g_filter_mtx);
        do {
            pthread_cond_wait(&g_filter_var, &g_filter_mtx);
        } while (g_filter_run == 0);
        
        g_filter_run = 0;       
        pthread_mutex_unlock(&g_filter_mtx);
        
        //====================================================
        // Logging for start happens in main loop.

        /* Perform filtering exchange round 1. */
        EXCHANGE_ExecuteMulti(SENSOR_COUNT, filter_run_these_rd1, filter_have_data_rd1);

        /* Set whether I am perplexed about each sensor. */
        /* I am perplex about a sensor if >= (1/2)(N - F) of the values */
        /* I receive for that sensor do not match my own. */
        memset(g_am_perplexed, 0x0, sizeof(g_am_perplexed));
        for (i = 0; i < SENSOR_COUNT; i++) {
            count = 0; /* count that doesn't match me */
            for (k = 0; k < REPLICA_COUNT; k++) {
                if (k != REPLICA_INDEX) { /* don't check against self */
                    index = TEST_BFT_TURPIN_EF_GetIndexExchange(k, i);
                    if (memcmp(&(g_sensor_data[i * SENSOR_DATA_SIZE]),
                               &(g_turpin_rd1[index * EXCHANGE_MAX_DATA_SIZE]),
                               SENSOR_DATA_SIZE) != 0)
                    {
                        count++;
                    }
                }
            }                        
            if (count >= (0.5 * (REPLICA_COUNT - max_faults))) {
                g_am_perplexed[i] = 1; /* perplexed about sensor */
            }
        }

        /* Perform filtering exchange round 2. */
        EXCHANGE_ExecuteMulti(SENSOR_COUNT, filter_run_these_rd2, filter_have_data_rd2);

        /* Set alert for each sensor. */
        /* I set alert for a sensor if >= (N - 2F) replicas are */
        /* perplexed about that sensor. */
        memset(g_alert, 0x0, sizeof(g_alert));
        for (i = 0; i < SENSOR_COUNT; i++) {
            count = 0; /* count that are perplexed */
            for (k = 0; k < REPLICA_COUNT; k++) {
                index = TEST_BFT_TURPIN_EF_GetIndexExchange(k, i);
                if (g_turpin_rd2[index * EXCHANGE_MAX_DATA_SIZE] == 1) {
                    count++;
                }
            }
            if (count >= (REPLICA_COUNT - (2 * max_faults))) {
                g_alert[i] = 1; /* set alert for sensor */
            }
        }

        /* Copy alert data to broadcast into broadcast instances. */
        memset(g_bcast_results, BCAST_DEF_VALUE, sizeof(g_bcast_results));                    
        for (i = 0; i < SENSOR_COUNT; i++) {
            index = TEST_BFT_TURPIN_EF_GetIndexBcast(REPLICA_INDEX, i);
            g_bcast_results[index] = g_alert[i];
        }

        LOG_LogWrite(LOG_APP_FILTER_SENSOR, LOG_ACT_APP_STOP);          
        //==================================================== 
    }

    return NULL;
}    

/* Execute the agreement stage for all sensor values. */
void* TEST_BFT_TURPIN_EF_BcastInstance(void* p_arg)
{
    size_t index = 0; /* index into a global buffer */
    int ret = 0;    
    int i = 0;
    int k = 0;

    /* Used to determine the "final" alert byte for each sensor. */
    void* pp_vals[REPLICA_COUNT + SENSOR_COUNT]; /* alert bytes to vote */
    void* p_result = NULL;                       /* result of voting alert bytes */
    size_t num_vals = 0;
    
    bool bcast_run_these[BCAST_MAX_INSTANCES] = {0}; /* instances to run bcast on */

    for (i = 0; i < REPLICA_COUNT; i++) {
        bcast_run_these[i] = true;
    }
    
    while (1) {
        pthread_mutex_lock(&g_bcast_mtx);
        do {
            pthread_cond_wait(&g_bcast_var, &g_bcast_mtx);
        } while (g_bcast_run == 0);
        
        g_bcast_run = 0;       
        pthread_mutex_unlock(&g_bcast_mtx);
        
        //====================================================
        // Logging for start happens in main loop.

        /* Perform Byzantine broadcasts for all sensor data. */
        BCAST_ExecuteMulti(REPLICA_COUNT, bcast_run_these);
        
        /* Perform vote to get a consistent vector of alert bytes. */
        memset(g_final_alert, 0x0, sizeof(g_final_alert));                    
        for (i = 0; i < SENSOR_COUNT; i++) {

            /* Get every alert byte broadcasted for sensor i. */
            for (k = 0; k < REPLICA_COUNT; k++) {
                index = TEST_BFT_TURPIN_EF_GetIndexBcast(k, i);                     
                pp_vals[k] = &(g_bcast_results[index]);                        
            }

            /* Vote the broadcasted alert bytes. */
            ret = VOTE_FindMaj(pp_vals, REPLICA_COUNT, 1, &p_result);
            if (ret == VOTE_SUCCESS) {
                g_final_alert[i] = *((uint8*)p_result);
            }
        }        
        
        /* Set final sensor values based on consistent alert vector. */ 
        memset(g_final_sensor_data, BCAST_DEF_VALUE, sizeof(g_final_sensor_data));                            
        for (i = 0; i < SENSOR_COUNT; i++) {
            switch (g_final_alert[i]) {

                case (1):
                    /* If alert is true, decide on default value. */
                    /* Nothing to do since data was initialized to default. */
                    break;
                    
                case (0):
                    if (g_am_perplexed[i]) { /* I am perplexed */
                        /* Determine result by voting the values I received in Turpin round 1, */
                        /* for which the sender said they were not perplexed in round 2. */
                        num_vals = 0;
                        for (k = 0; k < REPLICA_COUNT; k++) {
                            index = TEST_BFT_TURPIN_EF_GetIndexBcast(k, i);
                            if (g_turpin_rd2[index] != 1) {                                                           
                                pp_vals[num_vals] = &(g_turpin_rd1[index * SENSOR_DATA_SIZE]);
                                num_vals++;                       
                            }
                        }

                        /* Vote the broadcasted alert bytes. */
                        ret = VOTE_FindMaj(pp_vals, num_vals, SENSOR_DATA_SIZE, &p_result);
                        if (ret == VOTE_SUCCESS) {
                            memcpy(&(g_final_sensor_data[i * SENSOR_DATA_SIZE]), p_result, SENSOR_DATA_SIZE);
                        }                                    

                    } else { /* I am not perplexed */
                        /* The value I started with is correct. */
                        memcpy(&(g_final_sensor_data[i * SENSOR_DATA_SIZE]), 
                               &(g_sensor_data[i * SENSOR_DATA_SIZE]),
                               SENSOR_DATA_SIZE);
                    }
                
                    break;
                
                default:
                    break;
            }
        }

        LOG_LogWrite(LOG_APP_AGREEMENT, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}

/* Execute the state selection stage. */
void* TEST_BFT_TURPIN_EF_SelectInstance(void* p_arg)
{
    void* pp_vals[REPLICA_COUNT + SENSOR_COUNT]; /* candidate sensor values */
    int i = 0;

    while (1){
        pthread_mutex_lock(&g_select_mtx);
        do {
            pthread_cond_wait(&g_select_var, &g_select_mtx);
        } while (g_select_run == 0);
        
        g_select_run = 0;       
        pthread_mutex_unlock(&g_select_mtx);

        //====================================================
        // Logging for start happens in main loop.

        /* Get pointers to final sensor data. */
        for (i = 0; i < SENSOR_COUNT; i++) {
            pp_vals[i] = &(g_final_sensor_data[i * SENSOR_DATA_SIZE]);
        }

        /* Determine which sensor value to use as input. */
        SELECT_Execute(SOURCE_SELECT_TIME_MS, pp_vals, SENSOR_COUNT, &p_g_selected_sensor);                    

        LOG_LogWrite(LOG_APP_SOURCE_SELECT, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}   

/* Execute the computation stage. */
void* TEST_BFT_TURPIN_EF_ComputeInstance(void* p_arg)
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

        COMP_Execute(COMP_TIME_MS, (uint8*)p_g_selected_sensor, SENSOR_DATA_SIZE, g_msg_send, ACTUATOR_DATA_SIZE);        

        LOG_LogWrite(LOG_APP_COMPUTE_0, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}   

/*==========================================*/
/*==========================================*/
           
/* Entry point for the application. */
void TEST_BFT_TURPIN_EF_AppMain(void)
{
    CFE_SB_MsgId_t msg_id;
    int32  status;
    uint32 RunStatus = CFE_ES_APP_RUN;
    
    uint8 msg_recv[IO_MAX_DATA_SIZE]; /* message from sensor */       
    size_t msg_len = 0;               /* length of message read */
    size_t index = 0;                 /* index into a global buffer */
    int ret = 0;    
    int i = 0;
    int k = 0;

    pthread_attr_t custom_attr_comm;    /* thread attributes communication */
    pthread_attr_t custom_attr_compute; /* thread attributes computation */

    assert(SENSOR_DATA_SIZE <= IO_MAX_DATA_SIZE);            
    assert(ACTUATOR_DATA_SIZE <= IO_MAX_DATA_SIZE);

    /* Used to customize thread attributes. */
    TEST_BFT_TURPIN_EF_ThreadAttrComm(&custom_attr_comm);
    TEST_BFT_TURPIN_EF_ThreadAttrCompute(&custom_attr_compute, 0);

    /* Thread IDs for all the worker threads. */
    pthread_t filter_thread;    
    pthread_t bcast_thread;        
    pthread_t select_thread;
    pthread_t compute_thread;
    
    TEST_BFT_TURPIN_EF_AppInit();

    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {        
        /* Wait for wakeup message - no timeout. */
        status = CFE_SB_RcvMsg(&g_MsgPtr, g_WakeupPipe, CFE_SB_PEND_FOREVER);

        if (status == CFE_SUCCESS) {
        
            msg_id = CFE_SB_GetMsgId(g_MsgPtr);       
            switch (msg_id) {

                case (SCH_INIT_DONE_MID):

                    /* Configure each instance of exchange. */
                    TEST_BFT_TURPIN_EF_ExchangeConfig();                    

                    /* Configure each instance of broadcast. */
                    TEST_BFT_TURPIN_EF_BcastConfig();                    

                    /* Spawn the filtering thread. */       
                    ret = pthread_create(&(filter_thread), &custom_attr_comm, &TEST_BFT_TURPIN_EF_FilteringInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Filtering (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }

                    /* Spawn the broadcast thread. */
                    ret = pthread_create(&(bcast_thread), &custom_attr_comm, &TEST_BFT_TURPIN_EF_BcastInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Broadcast (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }

                    /* Spawn the state selection thread. */
                    ret = pthread_create(&(select_thread), &custom_attr_comm, &TEST_BFT_TURPIN_EF_SelectInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Selection (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }

                    /* Spawn the compute thread. */
                    ret = pthread_create(&(compute_thread), &custom_attr_compute, &TEST_BFT_TURPIN_EF_ComputeInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Compute (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }
                   
                    break;

                case (REPLICA_READ_SENSOR_MID):

                    LOG_LogWrite(LOG_APP_READ_SENSOR, LOG_ACT_APP_START);

                    memset(g_sensor_data, BCAST_DEF_VALUE, sizeof(g_sensor_data));
                    memset(g_turpin_rd1, BCAST_DEF_VALUE, sizeof(g_turpin_rd1));
                    
                    /* Read data from each sensor. */
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        ret = IO_Recv_Sensor2Replicas(i, msg_recv, &msg_len);
                        if ((ret == IO_SUCCESS) && (msg_len == SENSOR_DATA_SIZE)) {
                            memcpy(&(g_sensor_data[i * SENSOR_DATA_SIZE]), msg_recv, SENSOR_DATA_SIZE);
                        }
                    }

                    LOG_LogWrite(LOG_APP_READ_SENSOR, LOG_ACT_APP_STOP);

                    break;
                   
                case (REPLICA_FILTER_SENSOR_MID):                    
                    
                    LOG_LogWrite(LOG_APP_FILTER_SENSOR, LOG_ACT_APP_START);
                                        
                    /* Execute the filter stage. */
                    pthread_mutex_lock(&g_filter_mtx);                    
                    g_filter_run = 1;
                    pthread_cond_broadcast(&(g_filter_var));
                    pthread_mutex_unlock(&g_filter_mtx);
                
                    break;
                    
                case (REPLICA_START_AGREEMENT_MID):
                
                    LOG_LogWrite(LOG_APP_AGREEMENT, LOG_ACT_APP_START);
                
                    /* Execute the agreement stage. */
                    pthread_mutex_lock(&g_bcast_mtx);                    
                    g_bcast_run = 1;
                    pthread_cond_broadcast(&(g_bcast_var));
                    pthread_mutex_unlock(&g_bcast_mtx);
                    
                    break;
  
                case (REPLICA_START_SOURCE_SELECT_MID):

                    LOG_LogWrite(LOG_APP_SOURCE_SELECT, LOG_ACT_APP_START);
                    
                    /* Execute the selection stage. */
                    pthread_mutex_lock(&g_select_mtx);                    
                    g_select_run = 1;
                    pthread_cond_broadcast(&(g_select_var));
                    pthread_mutex_unlock(&g_select_mtx);
   
                    break;

                case (REPLICA_START_COMPUTE_MID):

                    LOG_LogWrite(LOG_APP_COMPUTE_0, LOG_ACT_APP_START);
                                    
                    /* Execute the selection stage. */
                    pthread_mutex_lock(&g_compute_mtx);                    
                    g_compute_run = 1;
                    pthread_cond_broadcast(&(g_compute_var));
                    pthread_mutex_unlock(&g_compute_mtx);

                    break;

                case (REPLICA_SEND_ACTUATOR_MID):

                    LOG_LogWrite(LOG_APP_SEND_ACTUATOR, LOG_ACT_APP_START);
                    
                    /* Send results to the actuators. */
                    IO_Send_Replica2Actuators(REPLICA_INDEX, g_msg_send, ACTUATOR_DATA_SIZE);

                    LOG_LogWrite(LOG_APP_SEND_ACTUATOR, LOG_ACT_APP_STOP);
                    
                    /* Clear the exchange VLs. */
                    for (i = 0; i < (SENSOR_COUNT * 2); i++) {
                        EXCHANGE_ClearVLs(i);
                    }

                    /* Clear the broadcast VLs. */
                    for (i = 0; i < REPLICA_COUNT; i++) {
                        if (i != REPLICA_INDEX) {
                            for (k = 0; k < SENSOR_COUNT; k++) {
                                index = TEST_BFT_TURPIN_EF_GetIndexBcast(i, k);  
                                BCAST_ClearVLs(index);
                            }
                        }
                    }
                                        
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
void TEST_BFT_TURPIN_EF_AppInit(void)
{
    /* Register app with Executive services. */
    CFE_ES_RegisterApp();

    /* Create wakeup pipe. */
    /* Subscribe to wakeup messages. */
    CFE_SB_CreatePipe(&g_WakeupPipe, WAKEUP_PIPE_DEPTH, WAKEUP_PIPE_NAME);
    CFE_SB_Subscribe(SCH_INIT_DONE_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_READ_SENSOR_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_FILTER_SENSOR_MID, g_WakeupPipe);    
    CFE_SB_Subscribe(REPLICA_START_AGREEMENT_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_START_SOURCE_SELECT_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_START_COMPUTE_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_SEND_ACTUATOR_MID, g_WakeupPipe);
    
    printf("%s: App Initialized\n", APP_NAME);

    return;				
}

