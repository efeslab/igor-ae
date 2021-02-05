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

uint8 msg_recv[BCAST_MAX_INSTANCES * BCAST_MAX_DATA_SIZE] = {0}; /* message I received */
    
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
    double round_times_ms[MAX_REPLICA_COUNT] = {0}; /* actual round times */
    double resolve_time_ms = 0;                     /* actual resolve time */
    size_t max_faults = 0;                      
    char buffer[1000] = {0};
    size_t bytes = 0;
    int i = 0;
    
    max_faults = BCAST_Data[instance].max_faults;
        
    BCAST_ReadMsg(instance, round_times_ms, &resolve_time_ms);   
    bytes += sprintf(buffer, "Instance %zu: Read message (first byte = 0x%02x); ",
                     instance, msg_recv[instance * BCAST_MAX_DATA_SIZE]);
    for (i = 1; i <= (max_faults + 1); i++) {
        bytes += sprintf(&(buffer[bytes]), "round %d: %.03f ms, ", i, round_times_ms[i]);
    }             
    sprintf(&(buffer[bytes]), "resolve: %.03f ms\n", resolve_time_ms);

    printf("%s", buffer);
    
    //printf("Instance %zu: Read message (first byte = 0x%02x)\n",
    //       instance, msg_recv[instance * BCAST_MAX_DATA_SIZE]);

    //for (i = 1; i <= (max_faults + 1); i++) {
    //    printf("round %d:   %.03f ms\n", i, round_times_ms[i]);
    //}
    //printf("resolve: %.03f ms\n", resolve_time_ms);
    
    return NULL;
}

/*
** Public Function Definitions
*/

int main(int argc, char** argv)
{
    size_t timeouts_ms[MAX_REPLICA_COUNT] = {0}; /* timeouts for rounds */   
    size_t max_faults = 0;                       /* max faults to tolerate */
    int ret = 0;
    int i = 0;

    pthread_t threads[BCAST_MAX_INSTANCES];
    
    size_t num_instances;
    size_t num_replicas;
    size_t src_index;
    size_t len;
    bool use_ef;
                      
    if (argc != 6) {
        printf("Usage: ./bcast_send.bin instance num_replicas src_index len use_ef\n");
        printf("num_instances: Number of instances of Byzantine broadcast.\n");        
        printf("num_replicas: Number of replicas running test including source.\n");        
        printf("src_index:    Index of source replica (0 to (MAX_REPLICA_COUNT - 1)).\n");
        printf("len:          Length of message to broadcast in bytes.\n");
        printf("use_ef:       1 to use error-free protocol, else 0.\n");
        return -1;
    }

    num_instances = atoi(argv[1]);
    num_replicas  = atoi(argv[2]);
    src_index     = atoi(argv[3]);
    len           = atoi(argv[4]);
    use_ef        = atoi(argv[5]);

    assert(num_instances <= BCAST_MAX_INSTANCES);
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

    /* Set large timeouts for each round. */
    for (i = 0; i < MAX_REPLICA_COUNT; i++) {
        timeouts_ms[i] = 30000; /* 30 seconds */
    }

    /* Initialize broadcast instances. */
    for (i = 0; i < num_instances; i++) {
        BCAST_Config(i, num_replicas, src_index, NULL,
                     &(msg_recv[i * BCAST_MAX_DATA_SIZE]), len, use_ef,
                     timeouts_ms, max_faults + 1);
    }

    /* Execute each broadcast receive in different thread. */
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

