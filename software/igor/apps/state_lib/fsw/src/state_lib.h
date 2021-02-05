/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _state_lib_
#define _state_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdbool.h>

/*
** Public Defines
*/

/* Maximum size of data to disperse in bytes. */
#define STATE_MAX_DATA_SIZE (1400)

/* Max instances of state dispersal. */
/* Must agree with the AFDX config table. */
#define STATE_MAX_INSTANCES (10)

/* Index in VL table where dispersal VLs start. */
/* See afdx_lib_vls.c in igor_defs/tables/ */
#define STATE_VL_OFFSET (630)

/*
** Public Structure Declarations
*/

/* Data for one instance of state dispersal. */
/* This is only public to facilitate testing. */
typedef struct {
    bool   is_config;              /* True if instance of dispersal is configured. */
    size_t num_replicas;           /* Number of replicas <= MAX_REPLICA_COUNT. */   
    uint8* p_input;                /* Pointer to state data to disperse to others. */
    uint8* p_output;               /* Pointer to where resulting state is stored. */
                                   /* Only used if the replica does not start with p_input. */
    size_t len;                    /* Size of state data to disperse in bytes. */

    int vl_ids[MAX_REPLICA_COUNT]; /* VLs used for sending and receiving. */
                                   /* Index REPLICA_INDEX is used for sending. */
                                   /* Else, index k is used to receive from Replica k. */

    /* Timeout in ms for the protocol. */
    /* A timeout of 0 means no timeout is used. */                           
    size_t timeout_ms;
    
    /* Data added to accomodate STATE_DisperseMulti(). */
    struct timespec stop;             /* time when timeout is reached. */
    bool got_data[MAX_REPLICA_COUNT]; /* true if got data from replica */

} STATE_Data_t;

/*
** Public Global Variables
*/

/* Internal data for each dispersal instance. */
/* This is only public to facilitate testing. */
extern STATE_Data_t STATE_Data[STATE_MAX_INSTANCES];

/*
** Public Function Declarations
*/

/* Used for initialization and cleanup. */
int32 STATE_LibEntry(void);
void  STATE_LibInit(void);
void  STATE_LibCleanup(void);

/**
** Configure an instance of the state dispersal protocol.
**
** \param[in]  instance     Instance of dispersal protocol to use.
**                          Must be < STATE_MAX_INSTANCES.
** \param[in]  num_replicas Number of replicas in system.
**                          Must be <= MAX_REPLICA_COUNT.
** \param[in]  p_input      Pointer to location of state to disperse.
** \param[out] p_output     Pointer where recovered state will be stored.
** \param[in]  len          Length of state to exchange in bytes.
**                          Must be <= STATE_MAX_DATA_SIZE.
** \param[in]  timeout_ms   Timeouts for the protocol in milliseconds.
**
** \return None
*/
void STATE_Config(size_t instance,
                  size_t num_replicas,
                  uint8* p_input,
                  uint8* p_output,
                  size_t len,
                  size_t timeout_ms);                  
         
/**
** Disperse your state data to other replicas.
**
** Note that the implementation of state dispersal is purposefully somewhat
** inefficient to make measuring worst-case latency easier. For example, even
** if a replica already has the state, they wait to read it from others. This
** would not be needed in a real implementation, since replicas who already
** have the state don't need to recover it.
**
** \param[in] instance   Instance of dispersal protocol < STATE_MAX_INSTANCES.
**                       The instance must have already been configured
**                       with STATE_Config().
** \param[in] have_state True if replica has state to disperse, else false.
**
** \returns None
*/
void STATE_Disperse(size_t instance,
                    bool have_state);
                    
/**
** Run multiple instances of state dispersal simultaneously.
**
** This is a convenience function that runs multiple instances of dispersal that
** have been configured until (num_instances) of them timeout or finish.
**
** \param[in] num_instances Number of instances of dispersal that must terminate
**                          in order for this function to return.
** \param[in] p_run_these   Pointer to array of size STATE_MAX_INSTANCES
**                          indicating which instances to run.
** \param[in] p_have_data   Pointer to array of size STATE_MAX_INSTANCES
**                          indicating which instances have state to disperse.
** \returns None
*/
void STATE_DisperseMulti(size_t num_instances,
                         bool* p_run_these,
                         bool* p_have_state);                    
/**
** Clear the receive VLs for the given dispersal instance.
**
** \param[in] instance Instance of exchange protocol < EXCHANGE_MAX_INSTANCES.
**                     The instance must have already been configured
**                     with EXCHANGE_Config().
** \returns None
*/
void STATE_ClearVLs(size_t instance);
                     
/**
** Execute some functions to test the library.
** \return None
*/              
void STATE_Test(void);
 
#endif

