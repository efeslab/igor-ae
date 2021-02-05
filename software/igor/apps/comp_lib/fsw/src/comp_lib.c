/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#define _GNU_SOURCE /* qsort_r() */

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

#include "comp_lib.h"

/*
** Function Declarations
*/
                                 
/*
** Constants
*/

#define LIB_NAME "COMP_LIB"

/* Number of iterations per millisecond. */
/* Determined experimentally on the RPi 3B+ at lowest clock speed. */
//#define ITERS_PER_MS (85000)
/* Determined experimentally on the RPi 3B+ at highest clock speed. */
#define ITERS_PER_MS (198333)

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

/* Entry point for the library. */
int32 COMP_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Perform a dummy computation that takes the given amount of time. */
void COMP_Execute(size_t time_ms,
                  uint8* p_input,
                  size_t len_in,
                  uint8* p_out,
                  size_t len_out)
{
    volatile long int i = 0;   

    size_t len = (len_out < len_in) ? len_out : len_in; /* shortest length */
        
    assert(p_input != NULL);
    assert(p_out != NULL);

    /* Calculate bitwise complement of input. */
    memset(p_out, 0x0, len_out);
    for (i = 0; i < len; i++) {
        p_out[i] = ~(p_input[i]);
    }

    /* Spin until time limit reached. */
    while (1) {
        if (i > (ITERS_PER_MS * time_ms)) {
            break;
        }
        i++;
    }

    return;
}

/* Execute some functions to test the library. */              
void COMP_Test(void)
{
    uint8 input[3] = {0xAA, 0xBB, 0xCC};
    uint8 output[3] = {0};
    size_t time_ms = 100;
    int i = 0;
    
    printf("Input: ");
    for (i = 0; i < 3; i++) {
        printf("0x%02x ", input[i]);
    }
    printf("\n");

    COMP_Execute(time_ms, input, 3, output, 3);

    printf("Output: ");
    for (i = 0; i < 3; i++) {
        printf("0x%02x ", output[i]);
    }
    printf("\n");
    
    return;
}

