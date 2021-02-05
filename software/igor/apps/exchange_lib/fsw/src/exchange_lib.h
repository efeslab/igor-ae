/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _exchange_lib_
#define _exchange_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdbool.h>
#include <time.h>

/*
** Public Defines
*/

/* Maximum size of data to broadcast in bytes. */
/* Must be <= AFDX_MAX_MSG_SIZE */
#define EXCHANGE_MAX_DATA_SIZE (1400)

/* Max instances of broadcast. */
/* Must agree with the AFDX config table. */
#define EXCHANGE_MAX_INSTANCES (10)

/* Index in VL table where broadcast VLs start. */
/* See afdx_lib_vls.c in igor_defs/tables/ */
#define EXCHANGE_VL_OFFSET (500)

/*
** Public Structure Declarations
*/

/* Data for one instance of broadcast. */
/* This is only public to facilitate testing. */
typedef struct {
    bool   is_config;                /* True if instance of broadcast is configured. */
    size_t num_replicas;             /* Number of replicas <= MAX_REPLICA_COUNT. */   
    uint8* p_input;                  /* Pointer to input data to exchange with others. */
    uint8* p_outputs;                /* Pointer where data received from replicas will be stored. */
                                     /* The data from Replica k is stored at index (k * EXCHANGE_MAX_DATA_SIZE). */
    bool* p_got_data;                /* Pointer to array used to indicate where data was received. */
                                     /* The array must contain at least num_replicas elements. */                                  
    size_t len;                      /* Size of data to exchange in bytes. */
                                     
    int vl_ids[MAX_REPLICA_COUNT];   /* VLs used for sending and receiving. */
                                     /* Index REPLICA_INDEX is used for sending. */
                                     /* Else, index k is used to receive from Replica k. */

    /* Timeout in ms for the protocol. */
    /* A timeout of 0 means no timeout is used. */                           
    size_t timeout_ms;
    
    /* Data added to accomodate EXCHANGE_ExecuteMulti(). */
    struct timespec stop;             /* time when timeout is reached. */
    bool got_data[MAX_REPLICA_COUNT]; /* true if got data from replica */
    size_t num_recv;                  /* number of values received */

} EXCHANGE_Data_t;

/*
** Public Global Variables
*/

/* Internal data for each exchange instance. */
/* This is only public to facilitate testing. */
extern EXCHANGE_Data_t EXCHANGE_Data[EXCHANGE_MAX_INSTANCES];

/*
** Public Function Declarations
*/

/* Used for initialization and cleanup. */
int32 EXCHANGE_LibEntry(void);
void  EXCHANGE_LibInit(void);
void  EXCHANGE_LibCleanup(void);

/**
** Configure an instance of the Exchange Protocol.
**
** \param[in]  instance     Instance of exchange protocol to use.
**                          Must be < EXCHANGE_MAX_INSTANCES.
** \param[in]  num_replicas Number of replicas in system.
**                          Must be <= MAX_REPLICA_COUNT.
** \param[in]  p_input      Pointer to location of data to exchange.
** \param[out] p_outputs    Pointer where output will be stored.
**                          Data from replica k is stored at index (k * EXCHANGE_MAX_DATA_SIZE).
** \param[out] p_got_data   Pointer to array to set indicating whether you
**                          received data from the replicas.
** \param[in]  len          Length of data to exchange in bytes.
**                          Must be <= EXCHANGE_MAX_DATA_SIZE.
** \param[in]  timeout_ms   Timeouts for the protocol in milliseconds.
**
** \return None
*/
void EXCHANGE_Config(size_t instance,
                     size_t num_replicas,
                     uint8* p_input,
                     uint8* p_outputs,
                     bool* p_got_data,
                     size_t len,
                     size_t timeout_ms);                  
              
/**
** Exchange your input value with all other replicas.
**
** \param[in] instance  Instance of exchange protocol < EXCHANGE_MAX_INSTANCES.
**                      The instance must have already been configured
**                      with EXCHANGE_Config().
** \param[in] have_data True if you have data to exchange, else False.
**
** \returns None
*/
void EXCHANGE_Execute(size_t instance,
                      bool have_data);

/**
** Run multiple instances of Exchange simulatenously.
**
** This is a convenience function that runs multiple instances of exchange that
** have been configured until (num_instances) of them timeout or finish.
**
** \param[in] num_instances Number of instances of exchange that must terminate
**                          in order for this function to return.
** \param[in] p_run_these   Pointer to array of size EXCHANGE_MAX_INSTANCES
**                          indicating which instances to run.
** \param[in] p_have_data   Pointer to array of size EXCHANGE_MAX_INSTANCES
**                          indicating which instances have data to send.
** \returns None
*/
void EXCHANGE_ExecuteMulti(size_t num_instances,
                           bool* p_run_these,
                           bool* p_have_data);
                           
/**
** Clear the receive VLs for the given exchange instance.
**
** \param[in] instance Instance of exchange protocol < EXCHANGE_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with EXCHANGE_Config().
** \returns None
*/
void EXCHANGE_ClearVLs(size_t instance);
                     
/**
** Execute some functions to test the library.
** \return None
*/              
void EXCHANGE_Test(void);
 
#endif

