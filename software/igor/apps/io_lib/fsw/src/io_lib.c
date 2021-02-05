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

#include "io_lib.h"
#include "afdx_api.h"
#include "afdx_lib_vldefs.h"

/*
** Function Declarations
*/

/*
** Constants
*/

#define LIB_NAME "IO_LIB"

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/* Internal data for the library. */
IO_Data_t IO_Data;

/*
** Internal Function Definitions
*/
                
/*
** Public Function Definitions
*/

/* Entry point for the library. */
int32 IO_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Initialize the library. */
void IO_LibInit(void)
{
    int index = 0; /* index into AFDX table */    
    int vl_id = 0; /* virtual link ID */     
    int i = 0;

    memset(&IO_Data, 0x0, sizeof(IO_Data));

    /* Initialize replica to actuator VLs. */
    for (i = 0; i < MAX_REPLICA_COUNT; i++) {
        index = IO_ACTUATOR_VL_OFFSET + i;
        vl_id = AFDX_LIB_VlTable[index].vl_id;
        IO_Data.actuator_vl_ids[i] = vl_id;
    }
 
    /* Initialize sensor to replica VLs. */
    for (i = 0; i < MAX_REPLICA_COUNT; i++) {
        index = IO_SENSOR_VL_OFFSET + i;
        vl_id = AFDX_LIB_VlTable[index].vl_id;
        IO_Data.sensor_vl_ids[i] = vl_id;
    }

    printf("%s: Lib Initialized\n", LIB_NAME);
    
    return;
}

/* Cleanup the library. */
void IO_LibCleanup(void) {
    printf("%s: Cleaned up library\n", LIB_NAME);
    return;
}

/* Send a message from a replica to the actuators. */
void IO_Send_Replica2Actuators(size_t replica_idx,
                               void* p_msg,
                               size_t len)
{
    assert(replica_idx < MAX_REPLICA_COUNT);
    assert(p_msg != NULL);
    assert(len <= IO_MAX_DATA_SIZE);
    
    AFDX_SendMsg(p_msg, len, IO_Data.actuator_vl_ids[replica_idx]);

    return;
}
                             
/* Read a message sent from a replica to the actuators. */
int IO_Recv_Replica2Actuators(size_t replica_idx,
                              void* p_buffer,
                              size_t* p_len)
{
    int ret = 0;
    uint8 buffer[AFDX_MAX_MSG_SIZE] = {0};
    uint16 msg_len = 0;

    assert(replica_idx < MAX_REPLICA_COUNT);
    assert(p_buffer != NULL);
    assert(p_len != NULL);

    ret = AFDX_ReadMsg(buffer, &msg_len, IO_Data.actuator_vl_ids[replica_idx]);
    if (ret != AFDX_SUCCESS) {
        return IO_FAILED;
    } else if (msg_len > IO_MAX_DATA_SIZE) {
        printf("%s: Received message is too large (bytes = %d)\n",
               LIB_NAME, msg_len);    
        return IO_FAILED;    
    }

    memcpy(p_buffer, buffer, msg_len);
    *p_len = msg_len;

    return IO_SUCCESS;
}

/* Send a message from a sensor to the replicas. */
void IO_Send_Sensor2Replicas(size_t sensor_idx,
                             void* p_msg,
                             size_t len)
{
    assert(sensor_idx < MAX_REPLICA_COUNT);
    assert(p_msg != NULL);
    assert(len <= IO_MAX_DATA_SIZE);
    
    AFDX_SendMsg(p_msg, len, IO_Data.sensor_vl_ids[sensor_idx]);

    return;
}

/* Read a message sent from a sensor to the replicas. */
int IO_Recv_Sensor2Replicas(size_t sensor_idx,
                            void* p_buffer,
                            size_t* p_len)
{
    int ret = 0;
    uint8 buffer[AFDX_MAX_MSG_SIZE] = {0};
    uint16 msg_len = 0;

    assert(sensor_idx < MAX_REPLICA_COUNT);
    assert(p_buffer != NULL);
    assert(p_len != NULL);

    ret = AFDX_ReadMsg(buffer, &msg_len, IO_Data.sensor_vl_ids[sensor_idx]);
    if (ret != AFDX_SUCCESS) {
        return IO_FAILED;
    } else if (msg_len > IO_MAX_DATA_SIZE) {
        printf("%s: Received message is too large (bytes = %d)\n",
               LIB_NAME, msg_len);    
        return IO_FAILED;    
    }
    
    memcpy(p_buffer, buffer, msg_len);
    *p_len = msg_len;

    return IO_SUCCESS;
}

