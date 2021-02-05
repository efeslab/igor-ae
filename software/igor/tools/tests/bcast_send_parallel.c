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

size_t instances[BCAST_MAX_INSTANCES] = {
0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
30, 31, 32, 33, 34, 35, 36, 37, 38, 39
};
   
/*
** Internal Function Definitions
*/

void* BCAST_Helper(void* p_arg)
{
    size_t instance = *((size_t*)p_arg);
    BCAST_SendMsg(instance);                 
    printf("Instance %zu: Sent message\n", instance);
    return NULL;
}
    
/*
** Public Function Definitions
*/

int main(int argc, char** argv)
{
    uint8 data[BCAST_MAX_DATA_SIZE] = {0}; /* message to send */
    time_t t;                              /* for seeding rand() */
    int ret = 0;
    int i = 0;
    
    pthread_t threads[BCAST_MAX_INSTANCES];

    size_t num_instances;
    size_t num_replicas;
    size_t src_index;
    size_t len;
    bool use_ef;

    if (argc != 6) {
        printf("Usage: ./bcast_send.bin num_instances num_replicas src_index len use_ef\n");
        printf("num_instances: Number of instances of Byzantine broadcast.\n");        
        printf("num_replicas:  Number of replicas running test including source.\n");        
        printf("src_index:     Index of source replica (0 to (MAX_REPLICA_COUNT - 1)).\n");
        printf("len:           Length of message to broadcast in bytes.\n");
        printf("use_ef:        1 to use error-free protocol, else 0.\n");
        return -1;
    }

    num_instances = atoi(argv[1]);
    num_replicas  = atoi(argv[2]);
    src_index     = atoi(argv[3]);
    len           = atoi(argv[4]);
    use_ef        = atoi(argv[5]);

    assert(num_instances <= BCAST_MAX_INSTANCES);
    assert(src_index == REPLICA_INDEX);

    printf("REPLICA_INDEX = %d\n", REPLICA_INDEX);
    printf("------------------------\n");
    
    srand((unsigned) time(&t));  
    memset(data, rand() % 256, sizeof(data));
    
    printf("Data to send: 0x%02x\n", data[0]);
    
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }

    BCAST_LibInit();
    
    /* Initialize broadcast instances. */
    for (i = 0; i < num_instances; i++) {
        BCAST_Config(i, num_replicas, src_index, data,
                     NULL, len, use_ef, NULL, 0);
    }

    /* Execute each broadcast in a different thread. */
    for (i = 0; i < num_instances; i++) {
        ret = pthread_create(&(threads[i]), NULL, &BCAST_Helper, &(instances[i]));
        if (ret != 0) {
            printf("Error spawning worker thread\n");    
            return -1;        
        }    
    }
    
    /* Wait for threads to terminate. */
    for (i = 0; i < num_instances; i++) {
        pthread_join(threads[i], NULL);
    }    

    BCAST_LibCleanup();
    
    AFDX_LIB_Cleanup();

    return 0;
}

