/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _io_lib_
#define _io_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdbool.h>

/*
** Public Defines
*/

/* Return codes for the library. */
#define IO_SUCCESS (0)
#define IO_FAILED  (-1)

/* Maximum size for of messages to send. */
/* Must be <= AFDX_MAX_MSG_SIZE */
#define IO_MAX_DATA_SIZE (1400)

/* Index in VL table where Replica to Actuator VLs start. */
/* See afdx_lib_vls.c in igor_defs/tables/ */
#define IO_ACTUATOR_VL_OFFSET (610)

/* Index in VL table where Sensor to Replica VLs start. */
/* See afdx_lib_vls.c in igor_defs/tables/ */
#define IO_SENSOR_VL_OFFSET (620)

/*
** Public Structure Declarations
*/

/* Internal data for the IO library. */
typedef struct {
    int actuator_vl_ids[MAX_REPLICA_COUNT]; /* VLs used for replicas to actuators. */
                                            /* Index k is the VL from replica k to all actuators. */
    int sensor_vl_ids[MAX_REPLICA_COUNT];   /* VLs used for sensors to replicas. */
                                            /* Index k is the VL from sensor k to all replicas. */
                                            /* Note that we assume there will never be more */
                                            /* redundant sensors than replicas. */
} IO_Data_t;

/*
** Public Global Variables
*/

/* Internal data for the IO library. */
extern IO_Data_t IO_Data;

/*
** Public Function Declarations
*/

/* Used for initialization and cleanup. */
int32 IO_LibEntry(void);
void  IO_LibInit(void);
void  IO_LibCleanup(void);

/**
** Send a message from a replica to the actuators.
**
** \param[in] replica_idx Index of replica to send from.
** \param[in] p_msg       Message to send to the actuators.
** \param[in] len         Length of the message to send in bytes.
**
** \returns None
*/
void IO_Send_Replica2Actuators(size_t replica_idx,
                               void* p_msg,
                               size_t len);
                               
/**
** Read a message sent from a replica to the actuators.
**
** \param[in]  replica_idx Index of replica to read from.
** \param[out] p_buffer    Pointer to buffer where message is stored.
** \param[out] p_len       Length of the message that was read.
**
** \returns IO_SUCCESS if message was read.
** \returns IO_FAILED if no message to read.
*/
int IO_Recv_Replica2Actuators(size_t replica_idx,
                              void* p_buffer,
                              size_t* p_len);
                              
/**
** Send a message from a sensor to the replicas.
**
** \param[in] sensor_idx Index of sensor to send from. 
** \param[in] p_msg      Message to send to the replicas.
** \param[in] len        Length of the message to send in bytes.
**
** \returns None
*/
void IO_Send_Sensor2Replicas(size_t sensor_idx,
                             void* p_msg,
                             size_t len);

/**
** Read a message sent from a sensor to the replicas.
**
** \param[in]  sensor_idx Index of sensor to send from. 
** \param[out] p_buffer   Pointer to buffer where message is stored.
** \param[out] p_len      Length of the message that was read.
**
** \returns IO_SUCCESS if message was read.
** \returns IO_FAILED if no message to read.
*/
int IO_Recv_Sensor2Replicas(size_t sensor_idx,
                            void* p_buffer,
                            size_t* p_len);

#endif

