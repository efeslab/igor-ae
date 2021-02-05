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
    uint8 data[AFDX_MAX_MSG_SIZE] = {0}; /* message received */
    uint16 msg_len = 0;                  /* length of message read */
    struct timespec curr;                /* current timestamp */
    time_t stop_sec = 0;                 /* time to stop in seconds */
    int i = 0;
    
    size_t time_sec = 0;

    if (argc != 2) {
        printf("Usage: ./afdx_recv.bin time_sec\n");     
        printf("time_sec: Type to read messages before exiting (sec).\n");
        return -1;
    }
    
    time_sec = atoi(argv[1]);
               
    /* Initialize the AFDX library. */
    if (AFDX_LIB_Init() != 0) {
        printf("Error initializing AFDX library\n");    
        return -1;
    }
    
    /* Calculate time to stop. */
    clock_gettime(CLOCK_MONOTONIC, &curr);
    stop_sec = curr.tv_sec + time_sec;
    
    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &curr);
        if (curr.tv_sec > stop_sec) {
            break;
        }
        
        for (i = 0; i <= 9; i++) {
            if (i != REPLICA_INDEX) { /* not me */
                if (AFDX_ReadMsg(data, &msg_len, i) == AFDX_SUCCESS) {
                    printf("Received a %d byte message from Replica %d\n", msg_len, i);                
                }
            }             
        }
        
        usleep(10);
    } 

    AFDX_LIB_Cleanup();

    return 0;
}

