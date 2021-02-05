/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include "state_lib.h"
#include "afdx_api.h"
#include "afdx_lib_vldefs.h"
#include "vote_lib.h"

/*
** Function Declarations
*/

/** 
** Calculate a timestamp offset (offset_ms) from the given timestamp.
**
** \param[in] start     Starting timestamp to add to.
** \param[in] offset_ms Time in milliseconds to add to timestamp.
**
** \return The new timestamp.
*/
struct timespec STATE_TimeOffset(struct timespec start,
                                 size_t offset_ms);
                                                         
/*
** Constants
*/

#define LIB_NAME "STATE_LIB"

/* Get the number of elements in an array. */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/* Internal data for each broadcast instance. */
STATE_Data_t STATE_Data[STATE_MAX_INSTANCES];

/*
** Internal Function Definitions
*/
  
/* Calculate a timestamp offset (offset_ms) from the given timestamp. */
struct timespec STATE_TimeOffset(struct timespec start,
                                 size_t offset_ms)
{
    struct timespec stop = start;
 
    /* Add seconds. */
    while (offset_ms >= 1000) {
        stop.tv_sec++;   
        offset_ms -= 1000;
    }
 
    /* Add remaining milliseconds. */
    stop.tv_nsec += (offset_ms * 1000000);
    if (stop.tv_nsec >= 1000000000) {
        stop.tv_nsec -= 1000000000;
        stop.tv_sec++;
    }

    return stop;
}
                                           
/*
** Public Function Definitions
*/

/* Entry point for the library. */
int32 STATE_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Initialize the library. */
void STATE_LibInit(void)
{
    STATE_Data_t* p_data = NULL; /* pointer to struct */
    int index_base = 0;          /* index into AFDX table */
    int index = 0;               /* index into AFDX table */    
    int vl_id = 0;               /* virtual link ID */     
    int i = 0;
    int j = 0;

    memset(STATE_Data, 0x0, sizeof(STATE_Data));

    for (i = 0; i < NELEMS(STATE_Data); i++) {
        p_data = &(STATE_Data[i]);
        index_base = STATE_VL_OFFSET + (MAX_REPLICA_COUNT * i);
        for (j = 0; j < MAX_REPLICA_COUNT; j++) {
            index = index_base + j;
            vl_id = AFDX_LIB_VlTable[index].vl_id;
            p_data->vl_ids[j] = vl_id;
        }
    }   
    
    printf("%s: Lib Initialized\n", LIB_NAME);
    
    return;
}

/* Cleanup the library. */
void STATE_LibCleanup(void) {
    printf("%s: Cleaned up library\n", LIB_NAME);
    return;
}

/* Configure an instance of the Exchange Protocol. */
void STATE_Config(size_t instance,
                  size_t num_replicas,
                  uint8* p_input,
                  uint8* p_output,
                  size_t len,
                  size_t timeout_ms)
{
    STATE_Data_t* p_data = NULL; /* pointer to struct */

    assert(instance < STATE_MAX_INSTANCES);
    assert(num_replicas <= MAX_REPLICA_COUNT);
    assert(p_input != NULL);
    assert(p_output != NULL); 
    assert(len <= STATE_MAX_DATA_SIZE);

    p_data = &(STATE_Data[instance]);

    p_data->is_config    = true;   
    p_data->num_replicas = num_replicas;   
    p_data->p_input      = p_input;
    p_data->p_output     = p_output;
    p_data->len          = len;
    p_data->timeout_ms   = timeout_ms;

    return; 
}

/* Disperse your state data to other replicas. */
void STATE_Disperse(size_t instance,
                    bool have_state)
{
    STATE_Data_t* p_data = NULL; /* pointer to struct */    
    size_t num_replicas;         /* number of replicas */
    int vl_id = 0;               /* VL to read or send through */
    size_t len = 0;              /* length of data to exchange */
    size_t timeout_ms = 0;       /* timeout for function */
    uint8* p_input = NULL;       /* pointer to input data */
    uint8* p_output = NULL;      /* pointer to output */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint8 msg_send[AFDX_MAX_MSG_SIZE] = {0}; /* message to send */    
    uint16 msg_len = 0;                      /* length of message received */

    /* Used for reassembling the state. */
    size_t frag_expect = 0;                      /* number of fragments I expect from each replica */
    size_t max_offset = 0;                       /* max offset for a fragment */
    size_t num_frags[MAX_REPLICA_COUNT] = {0};   /* number of fragments from each replica */
    bool   is_complete[MAX_REPLICA_COUNT] = {0}; /* True if not accepting new fragments from replica */
    size_t num_states = 0;                       /* number of complete states I received */
    bool all_complete = false;                   /* true when all states are complete */
    
    /* Used to store the states from the replicas. */
    /* The state from Replica k is stored at index (k * STATE_MAX_DATA_SIZE). */
    uint8 state_buffer[MAX_REPLICA_COUNT * STATE_MAX_DATA_SIZE] = {0};
    uint8* p_helper = NULL;
    size_t bytes_remain = 0;
    size_t max_msg_size = 0;
    uint32 offset = 0;
    
    /* Used for voting the state. */
    void* pp_vals[MAX_REPLICA_COUNT]; /* state values to vote */
    void* p_result = NULL;            /* result of voting state values */
    size_t num_vals = 0;              /* number of values to vote */

    struct timespec curr; /* current timestamp */
    struct timespec stop; /* time to return from function */

    assert(instance < STATE_MAX_INSTANCES);
    assert(STATE_Data[instance].is_config);
   
    p_data       = &(STATE_Data[instance]);
    num_replicas = p_data->num_replicas;
    len          = p_data->len;
    timeout_ms   = p_data->timeout_ms;
    vl_id        = p_data->vl_ids[REPLICA_INDEX];    
    p_input      = p_data->p_input;
    p_output     = p_data->p_output;

    /* Calculate timeout (only used if timeout is nonzero). */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    stop = STATE_TimeOffset(curr, timeout_ms);

    p_helper     = p_input;               /* next data to send */
    bytes_remain = len;                   /* bytes of data left to send */
    max_msg_size = AFDX_MAX_MSG_SIZE - 4; /* max size of data in fragment */
    offset       = 0;                     /* offset of data in state */
    max_offset   = 0;                     /* max offset I use */
    frag_expect  = 0;                     /* total number of fragments */     

    /* Send the state one frame at a time. */
    while (bytes_remain > 0) 
    {
        msg_len = (bytes_remain <= max_msg_size) ? bytes_remain : max_msg_size;

        /* Only send state if I actually have it. */
        if (have_state) {        
            memcpy(msg_send, &offset, 4); /* 4 bytes header */
            memcpy(&(msg_send[4]), p_helper, msg_len); /* the actual data */
            AFDX_SendMsg(msg_send, msg_len + 4, vl_id);
        }
        
        p_helper += msg_len;
        max_offset = offset;        
        offset += msg_len;
        bytes_remain -= msg_len;
        frag_expect++;
    } 

    /* If I have state, send it to myself. */
    if (have_state) {
        memcpy(&(state_buffer[REPLICA_INDEX * STATE_MAX_DATA_SIZE]), p_input, len);
        is_complete[REPLICA_INDEX] = true;
        num_states++;
    }

    while (1) {

        /* Read for values from other replicas. */
        for (i = 0; i < num_replicas; i++) {
            if (i != REPLICA_INDEX) { /* not me */
                vl_id = STATE_Data[instance].vl_ids[i];        
                ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
                if ((ret == AFDX_SUCCESS) && (msg_len > 4)) {
                    if (is_complete[i] == false) {
                        memcpy(&offset, msg_recv, 4);
                        if (offset <= max_offset) {
                            p_helper = &(state_buffer[(i * STATE_MAX_DATA_SIZE) + offset]);
                            memcpy(p_helper, &(msg_recv[4]), msg_len - 4);
                            num_frags[i]++;
                            if (num_frags[i] >= frag_expect) {
                                is_complete[i] = true;
                                num_states++;
                                if (num_states >= num_replicas) { /* received all states */
                                    all_complete = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
                
        /* Exit if all states are received. */
        if (all_complete) {     
            break;
        }        
                
        /* Check for timeout. */
        clock_gettime(CLOCK_MONOTONIC, &curr);
        if ((timeout_ms != 0) &&
             ((curr.tv_sec >= stop.tv_sec) && (curr.tv_nsec >= stop.tv_nsec))) {
            printf("%s: Instance %zu timeout occured\n", LIB_NAME, instance);
            break;
        }

        usleep(STATE_RECV_POLL_US);
    }

    /* Recover state if you need it. */
    if (have_state == false) {

        /* Get state values to vote. */
        num_vals = 0;
        for (i = 0; i < num_replicas; i++) {
            if (is_complete[i]) {
                pp_vals[num_vals] = &(state_buffer[i * STATE_MAX_DATA_SIZE]);
                num_vals++;
            }
        }

        /* Vote the state values; should always succeed. */
        ret = VOTE_FindMaj(pp_vals, num_vals, len, &p_result);
        if (ret == VOTE_SUCCESS) {
            memcpy(p_output, p_result, len);
        } else {
            printf("%s: Instance %zu error voting state\n", LIB_NAME, instance);       
        }
    }

    return;
}
                   
/* Broadcast your state for a specific instance of dispersal. */
void STATE_SendData(size_t instance)
{
    STATE_Data_t* p_data = NULL; /* pointer to struct */    
    int vl_id = 0;               /* VL to read or send through */
    size_t len = 0;              /* length of data to exchange */
    uint8* p_input = NULL;       /* pointer to input data */

    assert(instance < STATE_MAX_INSTANCES);
    assert(STATE_Data[instance].is_config);
   
    p_data   = &(STATE_Data[instance]);
    len      = p_data->len;
    vl_id    = p_data->vl_ids[REPLICA_INDEX];    
    p_input  = p_data->p_input;

    AFDX_SendMsg(p_input, len, vl_id);  

    return;
}                                     

/* Gather data for a specific instance of dispersal. */
bool STATE_ReadData(size_t instance)
{
    STATE_Data_t* p_data = NULL; /* pointer to struct */    
    size_t num_replicas;         /* number of replicas */
    int vl_id = 0;               /* VL to read or send through */
    size_t len = 0;              /* length of data to exchange */
    size_t timeout_ms;           /* timeout in ms */
    int ret = 0;
    int i = 0;
    int k = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */

    /* The state from Replica k is stored at index (k * STATE_MAX_DATA_SIZE). */
    uint8 state_buffer[MAX_REPLICA_COUNT * STATE_MAX_DATA_SIZE] = {0};
    
    size_t max_faults = floor((REPLICA_COUNT - 1.0)/3.0); /* max faults to tolerate */
    
    /* Used to find most common state. */
    /* We wait for f + 1 matching states, which outvotes faulty replicas. */
    void* pp_vals[MAX_REPLICA_COUNT]; /* state values to vote */
    void* p_result = NULL;            /* result of voting state values */
    size_t num_vals = 0;              /* number of values to vote */ 
    size_t count = 0;                 /* number of types value appears */
    
    struct timespec curr; /* current timestamp */

    assert(instance < STATE_MAX_INSTANCES);
    assert(STATE_Data[instance].is_config);

    p_data       = &(STATE_Data[instance]);
    num_replicas = p_data->num_replicas;
    len          = p_data->len;
    timeout_ms   = p_data->timeout_ms; 

    /* Read for values from other replicas. */
    for (i = 0; i < num_replicas; i++) {
        if (i != REPLICA_INDEX) { /* not me */
            vl_id = STATE_Data[instance].vl_ids[i];        
            ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
            if ((ret == AFDX_SUCCESS) && (msg_len == len)) {
                if (p_data->got_data[i] == false) {
                    memcpy(&(state_buffer[i * STATE_MAX_DATA_SIZE]), msg_recv, len);
                    p_data->got_data[i] = true;
                    
                    /* Get list of states received so far. */
                    num_vals = 0;
                    for (k = 0; k < num_replicas; k++) {
                        if (p_data->got_data[k] == true) {
                            pp_vals[num_vals] = &(state_buffer[k * STATE_MAX_DATA_SIZE]);
                            num_vals++;
                        }
                    }

                    /* Find most common state and how often it appears. */
                    /* If most common state appears >= f + 1 times, that is correct answer. */
                    VOTE_FindCom(pp_vals, num_vals, len, &p_result, &count);
                    if (count >= (max_faults + 1)) {
                        memcpy(p_data->p_output, p_result, len); 
                        return true; /* I am done */                    
                    }
                }
            }
        }
    }
            
    /* Check for timeout. */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    if ((timeout_ms != 0) &&
         ((curr.tv_sec >= p_data->stop.tv_sec) && (curr.tv_nsec >= p_data->stop.tv_nsec))) {
        printf("%s: Instance %zu timeout occured\n", LIB_NAME, instance);
        return true;
    }

    return false;
}

/* Run multiple instances of state dispersal simultaneously. */
void STATE_DisperseMulti(size_t num_instances,
                         bool* p_run_these,
                         bool* p_have_state)
{
    STATE_Data_t* p_data = NULL;              /* pointer to struct */ 
    bool is_done[STATE_MAX_INSTANCES] = {0};  /* true if instance is done */
    size_t num_done = 0;                      /* number of done instances */
    int i = 0;

    struct timespec curr; /* current timestamp */
        
    assert(num_instances <= EXCHANGE_MAX_INSTANCES);
    assert(p_run_these != NULL);
    assert(p_have_state != NULL);

    /* Reset global data before running protocol. */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    for (i = 0; i < STATE_MAX_INSTANCES; i++) {
        p_data = &(STATE_Data[i]);     
        if ((p_data->is_config) && (p_run_these[i] == true)) {
            p_data->stop = STATE_TimeOffset(curr, p_data->timeout_ms);
            memset(p_data->got_data, 0x0, sizeof(p_data->got_data));
        }
    }

    /* Loop through instances and send initial data. */
    for (i = 0; i < STATE_MAX_INSTANCES; i++) {
        p_data = &(STATE_Data[i]);
        if ((p_data->is_config) && (p_run_these[i] == true) && (p_have_state[i] == true)) {
            STATE_SendData(i);
            num_instances--;
        }
    }
    
    if (num_instances == 0) {
        return; /* started with state for all instances */
    }

    /* Read from each instance until all terminate. */
    while (1) {
        /* Loop through all instances. */
        for (i = 0; i < STATE_MAX_INSTANCES; i++) {
            p_data = &(STATE_Data[i]);
            if ((p_data->is_config) && (p_run_these[i] == true) && (p_have_state[i] == false) && (is_done[i] == false)) {
                if (STATE_ReadData(i) == true) { /* instance is done */
                    is_done[i] = true;
                    num_done++;
                    if (num_done >= num_instances) {
                        return; /* all instances done */
                    }
                }
            }
        }
        usleep(STATE_RECV_POLL_MULTI_US);
    }
    
    return;
}
                   
/* Clear the receive VLs for the given dispersal instance. */
void STATE_ClearVLs(size_t instance)
{
    STATE_Data_t* p_data = NULL; /* pointer to struct */
    int vl_id = 0;               /* VL to read from */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */
    
    assert(instance < STATE_MAX_INSTANCES);
    assert(STATE_Data[instance].is_config);

    p_data = &(STATE_Data[instance]);
    
    for (i = 0; i < NELEMS(p_data->vl_ids); i++) {
        if (i != REPLICA_INDEX) { /* not me */
            vl_id = p_data->vl_ids[i];
            do {
                ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
            } while (ret == AFDX_SUCCESS);
        }
    }

    return;
}

/* Execute some functions to test the library. */              
void STATE_Test(void)
{                      
    return;
}

