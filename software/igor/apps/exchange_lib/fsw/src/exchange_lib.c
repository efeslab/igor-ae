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

#include "exchange_lib.h"
#include "afdx_api.h"
#include "afdx_lib_vldefs.h"

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
static struct timespec EXCHANGE_TimeOffset(struct timespec start,
                                           size_t offset_ms);

/**
** Broadcast your data for a specific instance of exchange.
**
** \param[in] instance    Specifies the instance to broadcast for.
** \param[in] p_have_data True if there is data to broadcast, else false.
**
** \returns None
*/
static void EXCHANGE_SendData(size_t instance,
                              bool have_data);
                       
/**
** Gather data for a specific instance of exchange.
**
** \param[in] instance Specifies the instance to gather data for.
** \returns            True if all data received or timeout, else false.
*/                       
static bool EXCHANGE_ReadData(size_t instance);
                                                         
/*
** Constants
*/

#define LIB_NAME "EXCHANGE_LIB"

/* Get the number of elements in an array. */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/* Internal data for each broadcast instance. */
EXCHANGE_Data_t EXCHANGE_Data[EXCHANGE_MAX_INSTANCES];

/*
** Internal Function Definitions
*/
  
/* Calculate a timestamp offset (offset_ms) from the given timestamp. */
struct timespec EXCHANGE_TimeOffset(struct timespec start,
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

/* Broadcast your data for a specific instance of exchange. */
void EXCHANGE_SendData(size_t instance,
                       bool have_data)
{
    EXCHANGE_Data_t* p_data = NULL; /* pointer to struct */    
    int vl_id = 0;                  /* VL to read or send through */
    size_t len = 0;                 /* length of data to exchange */
    uint8* p_input = NULL;          /* pointer to input data */
    uint8* p_outputs = NULL;        /* pointer to outputs */

    assert(instance < EXCHANGE_MAX_INSTANCES);
    assert(EXCHANGE_Data[instance].is_config);
   
    p_data       = &(EXCHANGE_Data[instance]);
    len          = p_data->len;
    vl_id        = p_data->vl_ids[REPLICA_INDEX];    
    p_input      = p_data->p_input;
    p_outputs    = p_data->p_outputs;

    if (have_data) {
        /* If I have a value, broadcast it. */    
        AFDX_SendMsg(p_input, len, vl_id);  
        /* Also send it to myself. */
        memcpy(&(p_outputs[REPLICA_INDEX * EXCHANGE_MAX_DATA_SIZE]), p_input, len);
        p_data->got_data[REPLICA_INDEX] = true;
        p_data->num_recv++;
    }

    return;
}

/* Gather data for a specific instance of exchange. */
bool EXCHANGE_ReadData(size_t instance)
{
    EXCHANGE_Data_t* p_data = NULL; /* pointer to struct */    
    size_t num_replicas;            /* number of replicas */
    int vl_id = 0;                  /* VL to read or send through */
    size_t len = 0;                 /* length of data to exchange */
    uint8* p_outputs = NULL;        /* pointer to outputs */
    bool*  p_got_data = NULL;       /* pointer to got data flags */
    size_t timeout_ms;              /* timeout in ms */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */

    struct timespec curr; /* current timestamp */

    assert(instance < EXCHANGE_MAX_INSTANCES);
    assert(EXCHANGE_Data[instance].is_config);
   
    p_data       = &(EXCHANGE_Data[instance]);
    num_replicas = p_data->num_replicas;
    len          = p_data->len;
    timeout_ms   = p_data->timeout_ms;
    vl_id        = p_data->vl_ids[REPLICA_INDEX];    
    p_outputs    = p_data->p_outputs;
    p_got_data   = p_data->p_got_data;

    /* Read for values from other replicas. */
    for (i = 0; i < num_replicas; i++) {
        if (i != REPLICA_INDEX) { /* not me */
            vl_id = EXCHANGE_Data[instance].vl_ids[i];        
            ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
            if ((ret == AFDX_SUCCESS) && (msg_len == len)) {
                if (p_data->got_data[i] == false) {
                    memcpy(&(p_outputs[i * EXCHANGE_MAX_DATA_SIZE]), msg_recv, len);
                    p_data->got_data[i] = true;
                    p_data->num_recv++;
                    if (p_data->num_recv >= num_replicas) { /* received all messages */
                        memcpy(p_got_data, p_data->got_data, sizeof(bool) * num_replicas);    
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
                                                 
/*
** Public Function Definitions
*/

/* Entry point for the library. */
int32 EXCHANGE_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Initialize the library. */
void EXCHANGE_LibInit(void)
{
    EXCHANGE_Data_t* p_data = NULL; /* pointer to struct */
    int index_base = 0;             /* index into AFDX table */
    int index = 0;                  /* index into AFDX table */    
    int vl_id = 0;                  /* virtual link ID */     
    int i = 0;
    int j = 0;

    memset(EXCHANGE_Data, 0x0, sizeof(EXCHANGE_Data));

    for (i = 0; i < NELEMS(EXCHANGE_Data); i++) {
        p_data = &(EXCHANGE_Data[i]);
        index_base = EXCHANGE_VL_OFFSET + (MAX_REPLICA_COUNT * i);
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
void EXCHANGE_LibCleanup(void) {
    printf("%s: Cleaned up library\n", LIB_NAME);
    return;
}

/* Configure an instance of the Exchange Protocol. */
void EXCHANGE_Config(size_t instance,
                     size_t num_replicas,
                     uint8* p_input,
                     uint8* p_outputs,
                     bool* p_got_data,
                     size_t len,
                     size_t timeout_ms)
{
    EXCHANGE_Data_t* p_data = NULL; /* pointer to struct */

    assert(instance < EXCHANGE_MAX_INSTANCES);
    assert(num_replicas <= MAX_REPLICA_COUNT);
    assert(p_input != NULL);
    assert(p_outputs != NULL); 
    assert(p_got_data != NULL);     
    assert(len <= EXCHANGE_MAX_DATA_SIZE);

    p_data = &(EXCHANGE_Data[instance]);

    p_data->is_config    = true;   
    p_data->num_replicas = num_replicas;   
    p_data->p_input      = p_input;
    p_data->p_outputs    = p_outputs;
    p_data->p_got_data   = p_got_data;    
    p_data->len          = len;
    p_data->timeout_ms   = timeout_ms;

    return; 
}

/* Exchange your input value with all other replicas. */
void EXCHANGE_Execute(size_t instance,
                      bool have_data)
{
    EXCHANGE_Data_t* p_data = NULL; /* pointer to struct */    
    size_t num_replicas;            /* number of replicas */
    int vl_id = 0;                  /* VL to read or send through */
    size_t len = 0;                 /* length of data to exchange */
    size_t timeout_ms = 0;          /* timeout for function */
    uint8* p_input = NULL;          /* pointer to input data */
    uint8* p_outputs = NULL;        /* pointer to outputs */
    bool*  p_got_data = NULL;       /* pointer to got data flags */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    bool got_data[MAX_REPLICA_COUNT] = {0};  /* true if got data from replica */
    uint16 msg_len = 0;                      /* length of message received */
    size_t num_recv = 0;                     /* number of values received */

    struct timespec curr; /* current timestamp */
    struct timespec stop; /* time to return from function */

    assert(instance < EXCHANGE_MAX_INSTANCES);
    assert(EXCHANGE_Data[instance].is_config);
   
    p_data       = &(EXCHANGE_Data[instance]);
    num_replicas = p_data->num_replicas;
    len          = p_data->len;
    timeout_ms   = p_data->timeout_ms;
    vl_id        = p_data->vl_ids[REPLICA_INDEX];    
    p_input      = p_data->p_input;
    p_outputs    = p_data->p_outputs;
    p_got_data   = p_data->p_got_data;

    /* Calculate timeout (only used if timeout is nonzero). */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    stop = EXCHANGE_TimeOffset(curr, timeout_ms);

    if (have_data) {
        /* If I have a value, broadcast it. */    
        AFDX_SendMsg(p_input, len, vl_id);  
        /* Also send it to myself. */
        memcpy(&(p_outputs[REPLICA_INDEX * EXCHANGE_MAX_DATA_SIZE]), p_input, len);
        got_data[REPLICA_INDEX] = true;
        num_recv++;
    }

    while (1) {

        /* Read for values from other replicas. */
        for (i = 0; i < num_replicas; i++) {
            if (i != REPLICA_INDEX) { /* not me */
                vl_id = EXCHANGE_Data[instance].vl_ids[i];        
                ret = AFDX_ReadMsg(msg_recv, &msg_len, vl_id);  
                if ((ret == AFDX_SUCCESS) && (msg_len == len)) {
                    if (got_data[i] == false) {
                        memcpy(&(p_outputs[i * EXCHANGE_MAX_DATA_SIZE]), msg_recv, len);
                        got_data[i] = true;
                        num_recv++;
                        if (num_recv >= num_replicas) { /* received all messages */
                            memcpy(p_got_data, got_data, sizeof(bool) * num_replicas);    
                            return;
                        }
                    }
                }
            }
        }
                
        /* Check for timeout. */
        clock_gettime(CLOCK_MONOTONIC, &curr);
        if ((timeout_ms != 0) &&
             ((curr.tv_sec >= stop.tv_sec) && (curr.tv_nsec > stop.tv_nsec))) {
            printf("%s: Instance %zu timeout occured\n", LIB_NAME, instance);     
            return;
        }

        usleep(EXCHANGE_RECV_POLL_US);
    }

    return;
}

/* Run multiple instances of Exchange simulatenously. */
void EXCHANGE_ExecuteMulti(size_t num_instances,
                           bool* p_run_these,
                           bool* p_have_data)
{
    EXCHANGE_Data_t* p_data = NULL;              /* pointer to struct */ 
    bool is_done[EXCHANGE_MAX_INSTANCES] = {0};  /* true if instance is done */
    size_t num_done = 0;                         /* number of done instances */
    int i = 0;

    struct timespec curr; /* current timestamp */
        
    assert(num_instances <= EXCHANGE_MAX_INSTANCES);
    assert(p_run_these != NULL);
    assert(p_have_data != NULL);

    /* Reset global data before running protocol. */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    for (i = 0; i < EXCHANGE_MAX_INSTANCES; i++) {
        p_data = &(EXCHANGE_Data[i]);     
        if ((p_data->is_config) && (p_run_these[i] == true)) {
            p_data->stop = EXCHANGE_TimeOffset(curr, p_data->timeout_ms);
            memset(p_data->got_data, 0x0, sizeof(p_data->got_data));            
            p_data->num_recv = 0;
        }
    }

    /* Loop through instances and send initial data. */
    for (i = 0; i < EXCHANGE_MAX_INSTANCES; i++) {
        p_data = &(EXCHANGE_Data[i]);
        if ((p_data->is_config) && (p_run_these[i] == true)) {
            EXCHANGE_SendData(i, p_have_data[i]);
        }
    }
    
    /* Read from each instance until all terminate. */
    while (1) {
        /* Loop through all instances. */
        for (i = 0; i < EXCHANGE_MAX_INSTANCES; i++) {
            p_data = &(EXCHANGE_Data[i]);
            if ((p_data->is_config) && (p_run_these[i] == true) && (is_done[i] == false)) {
                if (EXCHANGE_ReadData(i) == true) { /* instance is done */
                    is_done[i] = true;
                    num_done++;
                    if (num_done >= num_instances) {
                        return; /* all instances done */
                    }
                }
            }
        }
        usleep(EXCHANGE_RECV_POLL_MULTI_US);
    }
    
    return;
}
                      
/* Clear the receive VLs for the given exchange instance. */
void EXCHANGE_ClearVLs(size_t instance)
{
    EXCHANGE_Data_t* p_data = NULL; /* pointer to struct */
    int vl_id = 0;                  /* VL to read from */
    int ret = 0;
    int i = 0;

    uint8 msg_recv[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                      /* length of message received */
    
    assert(instance < EXCHANGE_MAX_INSTANCES);
    assert(EXCHANGE_Data[instance].is_config);

    p_data = &(EXCHANGE_Data[instance]);
    
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
void EXCHANGE_Test(void)
{                      
    return;
}

