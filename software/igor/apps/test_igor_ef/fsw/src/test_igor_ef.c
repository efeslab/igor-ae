/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Run the error-free Igor protocol.
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
#include "exchange_lib.h"
#include "state_lib.h"
#include "log_lib.h"
#include "log_codes.h"
       
/*
** Function Declarations
*/

void TEST_IGOR_EF_AppMain(void);
void TEST_IGOR_EF_AppInit(void);
void TEST_IGOR_EF_Cleanup(void);

static size_t TEST_IGOR_EF_GetIndexFilter(size_t replica_idx, size_t sensor_idx);
static size_t TEST_IGOR_EF_GetIndexBcast(size_t replica_idx, size_t sensor_idx);

static void  TEST_IGOR_EF_ThreadAttrComm(pthread_attr_t* p_attr);
static void  TEST_IGOR_EF_ThreadAttrCompute(pthread_attr_t* p_attr, size_t instance);

static void  TEST_IGOR_EF_ExchangeConfig(void);
static void  TEST_IGOR_EF_BcastConfig(void);
static void  TEST_IGOR_EF_StateConfig(void);

static void* TEST_IGOR_EF_FilteringInstance(void* p_arg);
static void* TEST_IGOR_EF_BcastInstance(void* p_arg);
static void* TEST_IGOR_EF_SelectInstance(void* p_arg);
static void* TEST_IGOR_EF_ComputeInstance(void* p_arg);

static void* TEST_IGOR_EF_StateInstance(void* p_arg);
static void  TEST_IGOR_EF_StateDispersal(void);

/*
** Constants
*/

#define APP_NAME "TEST_IGOR_EF"

#define WAKEUP_PIPE_NAME  "TIE Pipe"
#define WAKEUP_PIPE_DEPTH (50)

/*
** Global Variables
*/

static CFE_SB_PipeId_t g_WakeupPipe;
static CFE_SB_MsgPtr_t g_MsgPtr;

/* Data I received from each sensor. */
/* The data from sensor k is stored at index (k * SENSOR_DATA_SIZE). */
static uint8 g_sensor_data[SENSOR_COUNT * SENSOR_DATA_SIZE];

/* Indicates whether I received a value from each sensor. */
static bool g_sensor_got_data[SENSOR_COUNT];

/*
** Used to stored messages exchanged during Filtering stage.
** The value Replica i sent for sensor k is stored at index:
**
** = (k * REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE) + (i * EXCHANGE_MAX_DATA_SIZE)
** = ((k * REPLICA_COUNT) + i) * EXCHANGE_MAX_DATA_SIZE
**
** Each element in a buffer is EXCHANGE_MAX_DATA_SIZE bytes.
*/
static uint8 g_filter_data[SENSOR_COUNT * REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE];

/* Indicates whether I received data from each replica in the Filtering stage. */
/* The flag indicating if Replica i sent me a message about sensor k is at index ((k * REPLICA_COUNT) + i). */
static bool g_filter_got_data[SENSOR_COUNT * REPLICA_COUNT];

/* Indicates whether I accept the value I received from each sensor. */
static bool g_accept_data[SENSOR_COUNT];

/*
** Used to stores accept bytes broadcasted by each replica.
** The byte Replica i broadcasted for sensor k is stored at index:
**
** = ((i * SENSOR_COUNT) + k) * 1
**
** Each element in a buffer is 1 byte.
*/
static uint8 g_bcast_results[SENSOR_COUNT * REPLICA_COUNT * 1];

/* Indicates whether each sensor is a candidate. */
static bool g_candidate[SENSOR_COUNT];

/* Speculative results of computing on each sensor's data. */
/* The result from computing on sensor k's data is stored at index (k * ACTUATOR_DATA_SIZE). */
static uint8 g_results[SENSOR_COUNT * ACTUATOR_DATA_SIZE];

/* Temporary states that result from computing on each sensor. */
/* For simplicity, just have the states be the result of the latest computation. */
/* The state that results from computing on sensor k's data is stored at index (k * ACTUATOR_DATA_SIZE). */
static uint8 g_temp_states[SENSOR_COUNT * STATE_SIZE];

/* The final state maintained by the replica. */
static uint8 g_final_state[STATE_SIZE];

/* Index of the sensor selected by source selection. */
static size_t g_selected_source = 0;

/* Used to identify different computations. */
static size_t g_compute_instances[MAX_SENSOR_COUNT] = {
0, 1, 2, 3, 4
};

/* Used to identify different dispersal instances. */
static size_t g_state_instances[STATE_MAX_INSTANCES] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

/*
** Used for thread synchronization.
*/

/* Condition variable for starting computations and filtering. */
static pthread_cond_t  g_compute_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_compute_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_compute_run[SENSOR_COUNT] = {0}; /* true to run compute thread */
static uint8 g_filter_run = 0;                  /* true to run filtering thread */
        
/* Condition variable for starting broadcast. */
static pthread_cond_t  g_bcast_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_bcast_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_bcast_run = 0; /* true to run bcast thread */

/* Condition variable for starting state selection. */
static pthread_cond_t  g_select_var = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_select_mtx = PTHREAD_MUTEX_INITIALIZER;
static uint8 g_select_run = 0; /* true to run select thread */
        
/*
** Function Definitions
*/

/* Calculate an index into a global buffer thet corresponds to Replica */
/* (replica_idx) sending the data it received for sensor (sensor_idx). */
size_t TEST_IGOR_EF_GetIndexFilter(size_t replica_idx,
                                   size_t sensor_idx)
{
    assert(replica_idx < REPLICA_COUNT);
    assert(sensor_idx < SENSOR_COUNT);
    return ((sensor_idx * REPLICA_COUNT) + replica_idx);
}

/* Calculate an index into a global buffer thet corresponds to Replica */
/* (replica_idx) broadcasting its accept byte for sensor (sensor_idx). */
size_t TEST_IGOR_EF_GetIndexBcast(size_t replica_idx,
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
void TEST_IGOR_EF_ThreadAttrCompute(pthread_attr_t* p_attr, size_t instance)
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
void TEST_IGOR_EF_ThreadAttrComm(pthread_attr_t* p_attr)
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

/* Configure all instances of exchange (used for Filtering). */
void TEST_IGOR_EF_ExchangeConfig(void)
{
    size_t timeout_ms = 1000; /* 1 second to avoid impacting latency test */
    size_t index = 0;
    int i = 0;

    /* Double check config is valid. */
    assert(SENSOR_COUNT <= EXCHANGE_MAX_INSTANCES);

    /* Configure instances of exchange. */    
    for (i = 0; i < SENSOR_COUNT; i++) {   
        index = TEST_IGOR_EF_GetIndexFilter(0, i); /* start of section */
        EXCHANGE_Config(i, REPLICA_COUNT,
                        &(g_sensor_data[i * SENSOR_DATA_SIZE]),
                        &(g_filter_data[index * EXCHANGE_MAX_DATA_SIZE]),
                        &(g_filter_got_data[index]),
                        SENSOR_DATA_SIZE, timeout_ms);
    }

    return;
}

/* Configure all instances of Byzantine broadcast. */
void TEST_IGOR_EF_BcastConfig(void)
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

/* Configure all instances of state dispersal. */
void TEST_IGOR_EF_StateConfig(void)
{
    size_t timeout_ms = 1000; /* 1 second to avoid impacting latency test */
    int i = 0;

    size_t frag_size = STATE_SIZE/STATE_INSTANCES; /* side of state fragment */

    /* Double check config is valid. */
    assert(STATE_INSTANCES <= STATE_MAX_INSTANCES);
    

    /* Configure instances of state dispersal. */    
    for (i = 0; i < STATE_INSTANCES; i++) {   
        STATE_Config(i, REPLICA_COUNT,
                     &(g_final_state[i * frag_size]),
                     &(g_final_state[i * frag_size]),
                     frag_size, timeout_ms);
    }

    return;
}

/*==========================================*/
/*==========================================*/

/* Execute the filtering stage for all sensor values. */
void* TEST_IGOR_EF_FilteringInstance(void* p_arg)
{
    size_t count = 0; /* count of values that match mine */
    size_t index = 0;
    int i = 0;
    int j = 0;
    
    size_t max_faults = floor((REPLICA_COUNT - 1.0)/3.0); /* max faults to tolerate */ 
    bool filter_run_these[EXCHANGE_MAX_INSTANCES] = {0};  /* instances to run filtering on */
    bool filter_have_data[EXCHANGE_MAX_INSTANCES] = {0};  /* says whether I have data to share */

    while (1){
        pthread_mutex_lock(&g_compute_mtx);
        do {
            pthread_cond_wait(&g_compute_var, &g_compute_mtx);
        } while (g_filter_run == 0);
        
        g_filter_run = 0;       
        pthread_mutex_unlock(&g_compute_mtx);
        
        //====================================================
        LOG_LogWrite(LOG_APP_FILTER_0, LOG_ACT_APP_START);       
     
        for (i = 0; i < SENSOR_COUNT; i++) {
            filter_run_these[i] = true;
            filter_have_data[i] = g_sensor_got_data[i];
        }    
        
        /* Perform filtering exchange on all sensor data. */
        EXCHANGE_ExecuteMulti(SENSOR_COUNT, filter_run_these, filter_have_data);
        
        /* Loop through the sensors. */
        for (j = 0; j < SENSOR_COUNT; j++) {

            /* If I didn't get a value from sensor (j), I have no value to accept. */
            /* However, I now have everyone else's value for that sensor, which */
            /* I may need to use later in the Agreement stage. */
            if (g_sensor_got_data[j] == false) {
                continue; /* no value to accept */
            }

            /* If I got a value from sensor (j), and I receive >= (n - f) */
            /* values from replicas that match my value (including from */
            /* myself), then accept the value. */
            count = 1; 
            for (i = 0; i < REPLICA_COUNT; i++) {
                if (i != REPLICA_INDEX) { /* not me */
                    index = TEST_IGOR_EF_GetIndexFilter(i, j);
                    if (g_filter_got_data[index]) { /* got value from that replica */
                        if (memcmp(&(g_sensor_data[j * SENSOR_DATA_SIZE]),
                                   &(g_filter_data[index * EXCHANGE_MAX_DATA_SIZE]),
                                   SENSOR_DATA_SIZE) == 0)
                        {
                            count++; /* their value matches mine */
                        }
                    }
                }
            }

            if (count >= (REPLICA_COUNT - max_faults)) {
                g_accept_data[j] = true; /* accept the value */
            }
        }

        LOG_LogWrite(LOG_APP_FILTER_0, LOG_ACT_APP_STOP);          
        //==================================================== 
    }

    return NULL;
}    
    
/* Execute the agreement stage for all sensor values. */
void* TEST_IGOR_EF_BcastInstance(void* p_arg)
{
    size_t index = 0; /* index into a global buffer */
    size_t count = 0; /* count of accept messages */ 
    int i = 0;
    int k = 0;   

    void* pp_vals[REPLICA_COUNT]; /* array of values to find most common */
    void* p_result = NULL;        /* most common value in array */
    size_t num_vals = 0;          /* number of values in array */
    size_t common_count = 0;      /* count of most common value */
    
    size_t max_faults = floor((REPLICA_COUNT - 1.0)/3.0); /* max faults to tolerate */   
    bool bcast_run_these[BCAST_MAX_INSTANCES] = {0};      /* instances to run bcast on */

    for (i = 0; i < REPLICA_COUNT; i++) {
        bcast_run_these[i] = true;
    }
    
    while (1){
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

        /* Mark each sensor as a candidate if you received >= (n - f) */
        /* accept bits for that sensor (including from self). */
        /* Remember that for Byzantine broadcast, there is not such thing as */
        /* not receiving a message from a given source. */
        memset(g_candidate, 0x0, sizeof(g_candidate));
        for (i = 0; i < SENSOR_COUNT; i++) {
            count = 0;
            for (k = 0; k < REPLICA_COUNT; k++) {
                index = TEST_IGOR_EF_GetIndexBcast(k, i);
                if (g_bcast_results[index]) { /* replica had accept */
                    count++;
                }
            }
            if (count >= (REPLICA_COUNT - max_faults)) {
                g_candidate[i] = true; /* that sensor is candidate */
            }
        }
              
        /* If I did not accept a value for a given sensor, replace that sensor's */
        /* value with the most common value I received from replicas that */
        /* accepted that sensor's value. */
        for (i = 0; i < SENSOR_COUNT; i++) {
            if (g_accept_data[i] == false) { /* I did not accept it */
                num_vals = 0;
                for (k = 0; k < REPLICA_COUNT; k++) {
                    index = TEST_IGOR_EF_GetIndexBcast(k, i);
                    if (g_bcast_results[index]) { /* they accepted it */
                        pp_vals[num_vals] = &(g_filter_data[index * EXCHANGE_MAX_DATA_SIZE]);
                        num_vals++;
                    }
                }          
                    
                /* Find the most common value given by replicas who accepted it. */
                VOTE_FindCom(pp_vals, num_vals, SENSOR_DATA_SIZE, &p_result, &common_count);   

                /* Overwrite my sensor data with the result value. */
                memcpy(&(g_sensor_data[i * SENSOR_DATA_SIZE]), p_result, SENSOR_DATA_SIZE);
            }
        }    

        LOG_LogWrite(LOG_APP_AGREEMENT, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}

/* Perform the computation on one of the sensor values. */
void* TEST_IGOR_EF_ComputeInstance(void* p_arg)
{
    size_t sensor_index = *((size_t*)p_arg); /* sensor to compute on */
    size_t app_code = 0;                     /* used for logging */

    assert(sensor_index < SENSOR_COUNT);

    /* Choose app code for logging. */
    switch (sensor_index) {
        case (0): app_code = LOG_APP_COMPUTE_0; break;
        case (1): app_code = LOG_APP_COMPUTE_1; break;    
        case (2): app_code = LOG_APP_COMPUTE_2; break;
        case (3): app_code = LOG_APP_COMPUTE_3; break;        
        case (4): app_code = LOG_APP_COMPUTE_4; break;
        default:
            break;
    }

    while (1){
        pthread_mutex_lock(&g_compute_mtx);
        do {
            pthread_cond_wait(&g_compute_var, &g_compute_mtx);
        } while (g_compute_run[sensor_index] == 0);
        
        g_compute_run[sensor_index] = 0;       
        pthread_mutex_unlock(&g_compute_mtx);
        
        //====================================================
        LOG_LogWrite(app_code, LOG_ACT_APP_START);
            
        if (g_sensor_got_data[sensor_index] == false) { /* no data to compute on */
            LOG_LogWrite(app_code, LOG_ACT_APP_STOP);
            continue;
        }
                
        /* Perform the computation on the sensor data. */
        COMP_Execute(COMP_TIME_MS, 
                     &(g_sensor_data[sensor_index * SENSOR_DATA_SIZE]),
                     SENSOR_DATA_SIZE, 
                     &(g_results[sensor_index * ACTUATOR_DATA_SIZE]),
                     ACTUATOR_DATA_SIZE);
                     
        /* Update the temporary state that results from the */
        /* computation on that sensor. */ 
        /* Note: This assume the STATE_SIZE is >= ACTUATOR_DATA_SIZE. */
        assert(STATE_SIZE >= ACTUATOR_DATA_SIZE);
        memcpy(&(g_temp_states[sensor_index * STATE_SIZE]),
               &(g_results[sensor_index * ACTUATOR_DATA_SIZE]),
               ACTUATOR_DATA_SIZE);                
                         
        LOG_LogWrite(app_code, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}

/* Execute the state selection stage. */
void* TEST_IGOR_EF_SelectInstance(void* p_arg)
{
    void* pp_vals[REPLICA_COUNT + SENSOR_COUNT]; /* candidate sensor values */
    void* p_result = NULL;                       /* result of source selection */
    size_t num_vals = 0;
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

        /* Perform a source selection of the candidate sensor values. */
        /* Note that I may have replaced some of these values in the Agreement stage. */                    
        num_vals = 0;
        for (i = 0; i < SENSOR_COUNT; i++) {
            if (g_candidate[i]) {
                pp_vals[num_vals] = &(g_sensor_data[i * SENSOR_DATA_SIZE]);    
                num_vals++;                    
            }
        }

        /* Perform the source selection. */
        g_selected_source = SELECT_Execute(SOURCE_SELECT_TIME_MS, pp_vals, num_vals, &p_result);                    

        LOG_LogWrite(LOG_APP_SOURCE_SELECT, LOG_ACT_APP_STOP);
        //==================================================== 
    }

    return NULL;
}    

/*==========================================*/
/*==========================================*/

/* Execute a single instance of state dispersal. */
void* TEST_IGOR_EF_StateInstance(void* p_arg)
{
    size_t instance = *((size_t*)p_arg);

    assert(instance < STATE_MAX_INSTANCES);

    STATE_Disperse(instance, g_accept_data[g_selected_source]);  

    return NULL;
}

/* Disperse the state to replicas that need it. */
void TEST_IGOR_EF_StateDispersal(void)
{
    pthread_t threads[STATE_MAX_INSTANCES]; /* thread IDs */ 
    int ret = 0;      
    int i = 0;
    
    /* Run the dispersal protocol on chunks of state. */
    for (i = 0; i < STATE_INSTANCES; i++) {
        ret = pthread_create(&(threads[i]), NULL, &TEST_IGOR_EF_StateInstance, &(g_state_instances[i]));
        if (ret != 0) {
            printf("%s: Error spawning worker for dispersal %d (ret = %d '%s')\n", 
                   APP_NAME, i, ret, strerror(ret));   
        }        
    }

    /* Wait for threads to terminate. */
    for (i = 0; i < STATE_INSTANCES; i++) {
        pthread_join(threads[i], NULL);
    }
  
    return;
}

/*==========================================*/
/*==========================================*/

/* Entry point for the application. */
void TEST_IGOR_EF_AppMain(void)
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

    memset(g_temp_states, 0x0, sizeof(g_temp_states));
    memset(g_final_state, 0x0, sizeof(g_final_state));

    /* Used to customize thread attributes. */
    /* Thread attributes for computation is filled in later. */
    TEST_IGOR_EF_ThreadAttrComm(&custom_attr_comm);
    
    /* Thread IDs for all the worker threads. */
    pthread_t compute_threads[SENSOR_COUNT];
    pthread_t filter_thread;    
    pthread_t bcast_thread;        
    pthread_t select_thread;

    TEST_IGOR_EF_AppInit();

    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {        
        /* Wait for wakeup message - no timeout. */
        status = CFE_SB_RcvMsg(&g_MsgPtr, g_WakeupPipe, CFE_SB_PEND_FOREVER);

        if (status == CFE_SUCCESS) {
        
            msg_id = CFE_SB_GetMsgId(g_MsgPtr);       
            switch (msg_id) {

                case (SCH_INIT_DONE_MID):

                    /* Configure each instance of exchange. */
                    TEST_IGOR_EF_ExchangeConfig();    
                    
                    /* Configure each instance of broadcast. */
                    TEST_IGOR_EF_BcastConfig();
                    
                    /* Configure each instance of dispersal. */
                    TEST_IGOR_EF_StateConfig();

                    /* Spawn all the compute threads. */
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        TEST_IGOR_EF_ThreadAttrCompute(&custom_attr_compute, i);
                        ret = pthread_create(&(compute_threads[i]), &custom_attr_compute, &TEST_IGOR_EF_ComputeInstance, &(g_compute_instances[i]));
                        if (ret != 0) {
                            printf("%s: Error spawning worker for compute %d (ret = %d '%s')\n", 
                                   APP_NAME, i, ret, strerror(ret));   
                        }
                    }

                    memset(g_filter_data, 0x0, sizeof(g_filter_data));
                    memset(g_filter_got_data, 0x0, sizeof(g_filter_got_data));
                    memset(g_accept_data, 0x0, sizeof(g_accept_data));
       
                    /* Spawn the filtering thread. */       
                    ret = pthread_create(&(filter_thread), &custom_attr_comm, &TEST_IGOR_EF_FilteringInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Filtering (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }

                    /* Spawn the broadcast thread. */
                    ret = pthread_create(&(bcast_thread), &custom_attr_comm, &TEST_IGOR_EF_BcastInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Broadcast (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }

                    /* Spawn the state selection thread. */
                    ret = pthread_create(&(select_thread), &custom_attr_comm, &TEST_IGOR_EF_SelectInstance, NULL);
                    if (ret != 0) {
                        printf("%s: Error spawning worker for Selection (ret = %d '%s')\n", 
                               APP_NAME, ret, strerror(ret));   
                    }
                                            
                    break;

                case (REPLICA_READ_SENSOR_MID):

                    LOG_LogWrite(LOG_APP_READ_SENSOR, LOG_ACT_APP_START);

                    /* Read data from each sensor. */
                    memset(g_sensor_data, BCAST_DEF_VALUE, sizeof(g_sensor_data));
                    memset(g_sensor_got_data, 0x0, sizeof(g_sensor_got_data));
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        ret = IO_Recv_Sensor2Replicas(i, msg_recv, &msg_len);
                        if ((ret == IO_SUCCESS) && (msg_len == SENSOR_DATA_SIZE)) {
                            memcpy(&(g_sensor_data[i * SENSOR_DATA_SIZE]), msg_recv, SENSOR_DATA_SIZE);
                            g_sensor_got_data[i] = true;
                        }
                    }
                                        
                    LOG_LogWrite(LOG_APP_READ_SENSOR, LOG_ACT_APP_STOP);
                    
                    break;

                case (REPLICA_START_COMPUTE_MID):

                    /* Log start of compibed Filtering and Speculative Execution. */
                    LOG_LogWrite(LOG_APP_PARALLEL_START, LOG_ACT_APP_START);

                    memset(g_results, 0x0, sizeof(g_results));    

                    /* Signal compute and filtering threads to execute. */
                    pthread_mutex_lock(&g_compute_mtx);                    
                    g_filter_run = 1;
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        g_compute_run[i] = 1;
                    }                   
                    pthread_cond_broadcast(&(g_compute_var));
                    pthread_mutex_unlock(&g_compute_mtx);
                    
                    break;

                case (REPLICA_START_AGREEMENT_MID):
                    /* Note that this can happen in parallel with computation. */

                    LOG_LogWrite(LOG_APP_AGREEMENT, LOG_ACT_APP_START);
    
                    /* Copy the accept results into data to broadcast. */
                    memset(g_bcast_results, 0x0, sizeof(g_bcast_results));                    
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        index = TEST_IGOR_EF_GetIndexBcast(REPLICA_INDEX, i);
                        g_bcast_results[index] = g_accept_data[i];
                    }  
 
                    /* Execute the agreement stage. */
                    pthread_mutex_lock(&g_bcast_mtx);                    
                    g_bcast_run = 1;
                    pthread_cond_broadcast(&(g_bcast_var));
                    pthread_mutex_unlock(&g_bcast_mtx);
                    
                    break;
 
                case (REPLICA_START_SOURCE_SELECT_MID):
                    /* Note that this can happen in parallel with computation. */
                                    
                    LOG_LogWrite(LOG_APP_SOURCE_SELECT, LOG_ACT_APP_START);

                    /* Execute the state selection stage. */
                    pthread_mutex_lock(&g_select_mtx);
                    g_select_run = 1;
                    pthread_cond_broadcast(&(g_select_var));
                    pthread_mutex_unlock(&g_select_mtx);
                                                            
                    break;

                case (REPLICA_SEND_ACTUATOR_MID):
                                    
                    LOG_LogWrite(LOG_APP_STATE_CONS, LOG_ACT_APP_START);                    
                    
                    /* If I accepted the value from the selected sensor, then replace my */
                    /* state with the state that resulted from computing on that sensor's value. */
                    if (g_accept_data[g_selected_source]) {
                        memcpy(g_final_state, &(g_temp_states[g_selected_source * STATE_SIZE]), STATE_SIZE);
                    }
                    
                    LOG_LogWrite(LOG_APP_STATE_CONS, LOG_ACT_APP_STOP);                

                    LOG_LogWrite(LOG_APP_SEND_ACTUATOR, LOG_ACT_APP_START);
                    
                    /* If I accepted the value from the selected sensor, then I have updated */
                    /* my state, so now can reference it to send a result to the actuators. */
                    if (g_accept_data[g_selected_source]) {
                        IO_Send_Replica2Actuators(REPLICA_INDEX, g_final_state, ACTUATOR_DATA_SIZE);                    
                    }

                    LOG_LogWrite(LOG_APP_SEND_ACTUATOR, LOG_ACT_APP_STOP);
                    
                    LOG_LogWrite(LOG_APP_STATE_DISPERSE, LOG_ACT_APP_START);                    
                    
                    /* Disperse state to replicas that need it. */
                    TEST_IGOR_EF_StateDispersal();
                    
                    LOG_LogWrite(LOG_APP_STATE_DISPERSE, LOG_ACT_APP_STOP);   
                                                            
                    /* Clear the exchange VLs. */
                    for (i = 0; i < SENSOR_COUNT; i++) {
                        EXCHANGE_ClearVLs(i);
                    }

                    /* Clear the broadcast VLs. */
                    for (i = 0; i < REPLICA_COUNT; i++) {
                        if (i != REPLICA_INDEX) {
                            for (k = 0; k < SENSOR_COUNT; k++) {
                                index = TEST_IGOR_EF_GetIndexBcast(i, k);  
                                BCAST_ClearVLs(index);
                            }
                        }
                    }

                    /* Clear the dispersal VLs. */
                    for (i = 0; i < STATE_INSTANCES; i++) {
                        STATE_ClearVLs(i);
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
void TEST_IGOR_EF_AppInit(void)
{
    /* Register app with Executive services. */
    CFE_ES_RegisterApp();

    /* Create wakeup pipe. */
    /* Subscribe to wakeup messages. */
    CFE_SB_CreatePipe(&g_WakeupPipe, WAKEUP_PIPE_DEPTH, WAKEUP_PIPE_NAME);
    CFE_SB_Subscribe(SCH_INIT_DONE_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_READ_SENSOR_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_START_COMPUTE_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_START_AGREEMENT_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_START_SOURCE_SELECT_MID, g_WakeupPipe);
    CFE_SB_Subscribe(REPLICA_SEND_ACTUATOR_MID, g_WakeupPipe);
    
    printf("%s: App Initialized\n", APP_NAME);

    return;				
}

