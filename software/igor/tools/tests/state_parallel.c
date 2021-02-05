/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Used for testing the state dispersal library.
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
#include "state_lib.h"

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

/* Used to store inputs for instances. */
uint8 inputs[STATE_MAX_INSTANCES * STATE_MAX_DATA_SIZE] = {0};

/* Used to store outputs for instances. */
uint8 outputs[STATE_MAX_INSTANCES * STATE_MAX_DATA_SIZE] = {0};
    
/* Used to differentiate dispersal instances. */    
size_t instances[STATE_MAX_INSTANCES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

/* True if I have the state, else false. */
bool g_have_state = false;

/*
** Internal Function Definitions
*/

void* STATE_Helper(void* p_arg)
{
    size_t instance = *((size_t*)p_arg);
        
    STATE_Disperse(instance, g_have_state);

    if (g_have_state == false) {
        printf("Instance %zu: State: 0x%02x\n",
               instance, outputs[instance * STATE_MAX_DATA_SIZE]);      
    } else {
        printf("Instance %zu: already has state\n", instance);    
    }
    
    return NULL;
}

/*
** Public Function Definitions
*/

int main(int argc, char** argv)
{
    int ret = 0;
    int i = 0;

    pthread_t threads[STATE_MAX_INSTANCES];

    size_t num_instances;
    size_t num_replicas;
    size_t len;
    size_t timeout_ms;
                      
    if (argc != 6) {
        printf("Usage: ./state_parallel.bin num_instances num_replicas len timeout_ms have_state\n");
        printf("num_instances: Number of instances of state dispersal.\n");        
        printf("num_replicas:  Number of replicas running test.\n");        
        printf("len:           Length of state to exchange per instance.\n");
        printf("timeout_ms:    Timeout for exchange in milliseconds.\n");     
        printf("have_state:    True if have state to disperse, else false.\n");
        return -1;
    }

    num_instances = atoi(argv[1]);
    num_replicas  = atoi(argv[2]);
    len           = atoi(argv[3]);
    timeout_ms    = atoi(argv[4]);
    g_have_state  = atoi(argv[5]);
    
    assert(num_instances <= STATE_MAX_INSTANCES);
    assert(num_replicas  <= MAX_REPLICA_COUNT);
    assert(len           <= STATE_MAX_DATA_SIZE);

    memset(inputs, VAL_BYTE, sizeof(inputs));

    printf("REPLICA_INDEX = %d\n", REPLICA_INDEX);
    printf("------------------------\n");
    
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }

    STATE_LibInit();

    /* Initialize exchange instances. */
    for (i = 0; i < num_instances; i++) {
        STATE_Config(i, num_replicas,
                     &(inputs[i * STATE_MAX_DATA_SIZE]),            
                     &(outputs[i * STATE_MAX_DATA_SIZE]),    
                     len, timeout_ms);
    }

    /* This sleep needs to be large enough such that all RPis have inited */
    /* the state library before anyone sends their messages. */
    sleep(20);

    /* Execute each exchange instance in different thread. */
    for (i = 0; i < num_instances; i++) {
        ret = pthread_create(&(threads[i]), NULL, &STATE_Helper, &(instances[i]));
        if (ret != 0) {
            printf("Error spawning worker thread\n");
            return -1;        
        }    
    }

    /* Wait for threads to terminate. */
    for (i = 0; i < num_instances; i++) {
        pthread_join(threads[i], NULL);
    }    

    STATE_LibCleanup();
    AFDX_LIB_Cleanup();

    return 0;
}

