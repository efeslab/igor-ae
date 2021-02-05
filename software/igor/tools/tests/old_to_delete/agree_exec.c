/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** Used for testing the consensus library.
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
#include "bcast_lib.h"
#include "agree_lib.h"

/*
** Function Declarations
*/

/*
** Constants
*/

/* Contents of value to agree on . */
#define VAL_BYTE (0xAB)

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
    uint8 inputs[AGREE_MAX_INSTANCES * AGREE_MAX_DATA_SIZE] = {0};
    uint8 outputs[AGREE_MAX_INSTANCES * AGREE_MAX_DATA_SIZE] = {0};
    size_t timeouts_ms[MAX_REPLICA_COUNT] = {0};       
    size_t max_faults = 0;
    int i = 0;

    size_t num_replicas;
    size_t num;
    size_t len;
    bool use_ef;
                  
    if (argc != 5) {
        printf("Usage: ./agree_exec.bin num_replicas num len use_ef\n");
        printf("num_replicas: Number of replicas running test.\n");        
        printf("num:          Number of instances of consensus to run.\n");
        printf("len:          Length of values to agree on in bytes.\n");
        printf("use_ef:       1 to use error-free protocol, else 0.\n");
        return -1;
    }

    num_replicas = atoi(argv[1]);
    num          = atoi(argv[2]);
    len          = atoi(argv[3]);
    use_ef       = atoi(argv[4]);

    memset(inputs, VAL_BYTE, sizeof(inputs));
    
    for (i = 0; i < MAX_REPLICA_COUNT; i++) {
        timeouts_ms[i] = 30000; /* 30 seconds */
    }

    printf("REPLICA_INDEX = %d\n", REPLICA_INDEX);
    printf("------------------------\n");
    
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }

    BCAST_LibInit();
    AGREE_LibInit();

    max_faults = (use_ef) ? floor((num_replicas - 1.0)/3.0) : \
                            floor((num_replicas - 1.0)/2.0);

    AGREE_Config(num_replicas, inputs, outputs, num, len, use_ef, timeouts_ms, max_faults + 1);

    /* This sleep needs to be large enough such that all RPis have */
    /* initialized the AFDX/bcast/agree libraries before and RPis */
    /* send their round 1 messages. */
    sleep(30);

    AGREE_RunProtocol();

    AGREE_GetResults();                  

    sleep(5);
    
    printf("Results:\n");
    printf("------------------------\n");
    for (i = 0; i < num; i++) {
        printf("Instance %d: 0x%02x (first byte)\n", i, outputs[i * AGREE_MAX_DATA_SIZE]);
    }

    AGREE_LibCleanup();
    BCAST_LibCleanup();   
    AFDX_LIB_Cleanup();

    return 0;
}

