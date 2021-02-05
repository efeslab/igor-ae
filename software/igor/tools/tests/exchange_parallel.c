/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Used for testing the exchange library.
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
#include "exchange_lib.h"

/*
** Function Declarations
*/

/*
** Constants
*/

/* Contents of value to exchange. */
#define VAL_BYTE (0xAB)

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/* Used to store messages I receive. */
uint8 outputs[EXCHANGE_MAX_INSTANCES * MAX_REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE] = {0};

/* Used to indicate whether I received data. */
bool got_data[EXCHANGE_MAX_INSTANCES * MAX_REPLICA_COUNT] = {0};
    
/* Used to differentiate exchange instances. */    
size_t instances[EXCHANGE_MAX_INSTANCES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
      
/*
** Internal Function Definitions
*/

void* EXCHANGE_Helper(void* p_arg)
{
    size_t instance = *((size_t*)p_arg);
    char buffer[1000] = {0};
    size_t bytes = 0;
    size_t index = 0;
    int i = 0;

    size_t num_replicas = EXCHANGE_Data[instance].num_replicas;
        
    EXCHANGE_Execute(instance, true);

    bytes += sprintf(&(buffer[bytes]), "Instance %zu: Data: ", instance);      
    for (i = 0; i < num_replicas; i++) {
        index = (instance * MAX_REPLICA_COUNT) + i;
        index = index * EXCHANGE_MAX_DATA_SIZE;
        bytes += sprintf(&(buffer[bytes]), "0x%02x ", outputs[index]);
    }
    bytes += sprintf(&(buffer[bytes]), "\n");

    bytes += sprintf(&(buffer[bytes]), "Instance %zu: Got Data: ", instance);          
    for (i = 0; i < num_replicas; i++) {
        index = (instance * MAX_REPLICA_COUNT) + i;
        bytes += sprintf(&(buffer[bytes]), "%d ", got_data[index] ? 1 : 0);
    }
    bytes += sprintf(&(buffer[bytes]), "\n");
             
    printf("%s", buffer);
    
    return NULL;
}

/*
** Public Function Definitions
*/

int main(int argc, char** argv)
{
    uint8 input[EXCHANGE_MAX_DATA_SIZE] = {0};
    int ret = 0;
    int i = 0;

    pthread_t threads[EXCHANGE_MAX_INSTANCES];
    
    size_t num_instances;
    size_t num_replicas;
    size_t len;
    size_t timeout_ms;
                      
    if (argc != 5) {
        printf("Usage: ./exchange_parallel.bin num_instances num_replicas len timeout_ms\n");
        printf("num_instances: Number of instances of exchange.\n");        
        printf("num_replicas:  Number of replicas running test.\n");        
        printf("len:           Length of message to exchange.\n");
        printf("timeout_ms:    Timeout for exchange in milliseconds.\n");
        return -1;
    }

    num_instances = atoi(argv[1]);
    num_replicas  = atoi(argv[2]);
    len           = atoi(argv[3]);
    timeout_ms    = atoi(argv[4]);

    assert(num_instances <= EXCHANGE_MAX_INSTANCES);
    assert(num_replicas <= MAX_REPLICA_COUNT);

    memset(input, VAL_BYTE, sizeof(input));

    printf("REPLICA_INDEX = %d\n", REPLICA_INDEX);
    printf("------------------------\n");
    
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }

    EXCHANGE_LibInit();

    /* Initialize exchange instances. */
    for (i = 0; i < num_instances; i++) {
        EXCHANGE_Config(i, num_replicas, input,
                        &(outputs[i * MAX_REPLICA_COUNT * EXCHANGE_MAX_DATA_SIZE]),    
                        &(got_data[i * MAX_REPLICA_COUNT]), len, timeout_ms);
    }

    /* This sleep needs to be large enough such that all RPis have inited */
    /* the exchange librarybefore anyone sends their messages. */
    sleep(20);

    /* Execute each exchange instance in different thread. */
    for (i = 0; i < num_instances; i++) {
        ret = pthread_create(&(threads[i]), NULL, &EXCHANGE_Helper, &(instances[i]));
        if (ret != 0) {
            printf("Error spawning worker thread\n");
            return -1;        
        }    
    }

    /* Wait for threads to terminate. */
    for (i = 0; i < num_instances; i++) {
        pthread_join(threads[i], NULL);
    }    

    EXCHANGE_LibCleanup();
    AFDX_LIB_Cleanup();

    return 0;
}

