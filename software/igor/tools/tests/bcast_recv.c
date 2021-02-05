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
#include <math.h>
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
    uint8 msg_recv[BCAST_MAX_DATA_SIZE] = {0};      /* message I received */
    size_t timeouts_ms[MAX_REPLICA_COUNT] = {0};    /* timeouts for rounds */   
    double round_times_ms[MAX_REPLICA_COUNT] = {0}; /* actual round times */
    double resolve_time_ms = 0;                     /* actual resolve time */
    size_t max_faults = 0;                          /* max faults to tolerate */
    int i = 0;
    
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

    assert(src_index != REPLICA_INDEX);
    
    printf("REPLICA_INDEX = %d\n", REPLICA_INDEX);
    printf("------------------------\n");
    
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }

    BCAST_LibInit();

    max_faults = (use_ef) ? floor((num_replicas - 1.0)/3.0) : \
                            floor((num_replicas - 1.0)/2.0);

    BCAST_Config(instance, num_replicas, src_index, NULL,
                  msg_recv, len, use_ef, timeouts_ms, max_faults + 1);

    BCAST_ReadMsg(instance, round_times_ms, &resolve_time_ms);   
                  
    printf("Read message (first byte = 0x%02x)\n", msg_recv[0]);

    for (i = 1; i <= (max_faults + 1); i++) {
        printf("round %d:   %.03f ms\n", i, round_times_ms[i]);
    }
    printf("resolve: %.03f ms\n", resolve_time_ms);

    BCAST_LibCleanup();

    AFDX_LIB_Cleanup();

    return 0;
}

