/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Used for testing send/receive with the broadcast library.
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
#include "bcast_lib.h"

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
    uint8 data[BCAST_MAX_DATA_SIZE] = {0}; /* message to send */
    time_t t;                              /* for seeding rand() */

    size_t instance;
    size_t num_replicas;
    size_t src_index;
    size_t len;
    bool use_ef;
                  
    if (argc != 6) {
        printf("Usage: ./bcast_send.bin instance num_replicas src_index len use_ef\n");
        printf("instance:     Instace of broadcast to use (0 to (BCAST_MAX_INSTANCES - 1)).\n");        
        printf("num_replicas: Number of replicas running test including source.\n");        
        printf("src_index:    Index of source replica (0 to (MAX_REPLICA_COUNT - 1)).\n");
        printf("len:          Length of message to broadcast in bytes.\n");
        printf("use_ef:       1 to use error-free protocol, else 0.\n");
        return -1;
    }

    instance     = atoi(argv[1]);
    num_replicas = atoi(argv[2]);
    src_index    = atoi(argv[3]);
    len          = atoi(argv[4]);
    use_ef       = atoi(argv[5]);

    assert(src_index == REPLICA_INDEX);

    printf("REPLICA_INDEX = %d\n", REPLICA_INDEX);
    printf("------------------------\n");
    
    srand((unsigned) time(&t));  
    memset(data, rand() % 256, sizeof(data));
    
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }

    BCAST_LibInit();
    
    BCAST_Config(instance, num_replicas, src_index, data,
                  NULL, len, use_ef, NULL, 0);
                  
    BCAST_SendMsg(instance);                  
                  
    printf("Send message filled with 0x%02x bytes\n", data[0]);

    BCAST_LibCleanup();
    
    AFDX_LIB_Cleanup();

    return 0;
}

