/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Used for testing send/receive with the AFDX library.
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
#include <assert.h>
#include <pthread.h>

#include "afdx_lib.h"
#include "afdx_api.h"

/*
** Function Declarations
*/

/*
** Constants
*/

/*
** Structure Declarations
*/

/*
** Global Variables
*/
   
/*
** Internal Function Definitions
*/

/*
** Public Function Definitions
*/

int main(int argc, char** argv)
{
    uint8 data[AFDX_MAX_MSG_SIZE] = {0}; /* message to send */
    time_t t;
    int i = 0;
    int ret = 0;
    
    size_t num_msgs = 0;
    size_t len = 0;

    if (argc != 3) {
        printf("Usage: ./afdx_send.bin num_msgs len\n");
        printf("num_msgs: Number of messages to send.\n");
        printf("len:      Length of message to send in bytes.\n");
        return -1;
    }
    
    num_msgs  = atoi(argv[1]);
    len       = atoi(argv[2]);
    
    assert(num_msgs <= 1000);
    assert(len <= AFDX_MAX_MSG_SIZE);
               
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }
    
    srand((unsigned)time(&t));    
    memset(data, rand() % 256, len);    
        
    for (i = 0; i < num_msgs; i++) {
        ret = AFDX_SendMsg(data, len, REPLICA_INDEX);
        if (ret == AFDX_SUCCESS) {
            printf("Sent a %zu byte message on VL %d (message %d)\n", len, REPLICA_INDEX, i + 1);
        } else {
            printf("Error sending message on VL %d\n", REPLICA_INDEX);
        }
    }
        
    AFDX_LIB_Cleanup();

    return 0;
}

