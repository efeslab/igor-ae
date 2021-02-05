/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
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
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include "vote_lib.h"
#include "select_lib.h"

/*
** Function Declarations
*/

#if 0
/** 
** Calculate a timestamp offset (offset_ms) from the given timestamp.
**
** \param[in] start     Starting timestamp to add to.
** \param[in] offset_ms Time in milliseconds to add to timestamp.
**
** \return The new timestamp.
*/
struct timespec SELECT_TimeOffset(struct timespec start,
                                  size_t offset_ms);
#endif
                                
/*
** Constants
*/

#define LIB_NAME "SELECT_LIB"

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

#if 0
/* Calculate difference between two timestamps in milliseconds. */
double SELECT_TimeDiffMs(struct timespec start,
                         struct timespec stop)
{
    double time;
    time = (stop.tv_sec - start.tv_sec) * 1e9; /* time in seconds as ns */
    time = (time + (stop.tv_nsec - start.tv_nsec)) * 1e-6; /* add in ns, convert to ms */
    return time;
}
#endif

/*
** Public Function Definitions
*/

/* Entry point for the library. */
int32 SELECT_LibEntry(void) {
    return CFE_SUCCESS;
}

#if 0
/* Perform a source selection of the provides sensor values. */
size_t SELECT_Execute(void** pp_vals,
                      size_t num,
                      size_t len,
                      void** pp_out)
{
    void* pp_words[VOTE_MAX_COMPARE] = {0};   /* pointers to words to compare */
    size_t win_count[VOTE_MAX_COMPARE] = {0}; /* number of times each value "won". */
    size_t win_index = 0;                     /* index of current winner */
    size_t word_size = 4;                     /* word size in bytes */
    int i = 0;
    int j = 0;

    assert(pp_vals != NULL); 
    assert(pp_out != NULL); 
    assert(num <= VOTE_MAX_COMPARE);

    /* Loop through data one word at a time. */
    for (i = 0; i < (len/word_size); i++) {
        /* Get pointers to words to compare. */
        for (j = 0; j < num; j++) {
            pp_words[j] = ((uint8*)pp_vals[j]) + (i * word_size);
        }
        VOTE_FindMidVal(pp_words, num, word_size, &win_index);
        win_count[win_index] += 1;
    }

    *pp_out = pp_vals[0];
    win_index = 0;

    /* Determine value with the most wins. */
    for (i = 0; i < num; i++) {
        if (win_count[i] > win_count[win_index]) {
            *pp_out = pp_vals[i];
            win_index = i;
        }
    }
    
    #if 0
    printf("---------------------\n");    
    for (i = 0; i < num; i++) {
        printf("value %d wins = %zu\n", i, win_count[i]);
    }
    printf("---------------------\n");
    #endif
    
    return win_index;
}
#endif
                                      
/* Perform a source selection of the provides sensor values. */
size_t SELECT_Execute(size_t time_ms,
                      void** pp_vals,
                      size_t num,
                      void** pp_out)
{
    volatile long int i = 0;   

    /* For latency tests, always choose the first value. */         
    size_t select_index = 0;  
    
    assert(pp_vals != NULL); 
    assert(num > 0);
    assert(pp_out != NULL); 

    *pp_out = pp_vals[select_index];

    /* Spin until time limit reached. */
    while (1) {
        if (i > (ITERS_PER_MS * time_ms)) {
            break;
        }
        i++;
    }

    return select_index;
}

#if 0
/* Execute some functions to test the library. */              
void SELECT_Test(void)
{
    uint8 data1[1300] = {0};
    uint8 data2[1300] = {0};
    uint8 data3[1300] = {0};   
    uint8 data4[1300] = {0};
    uint8 data5[1300] = {0};
    void* pp_vals[5]; 
    void* p_select = NULL;
    size_t win_index = 0;

    struct timespec start;
    struct timespec stop;   
        
    memset(data1, 0xBC, sizeof(data1));
    memset(data2, 0xAA, sizeof(data2));
    memset(data3, 0xCC, sizeof(data3));
    memset(data4, 0xCD, sizeof(data4));
    memset(data5, 0xBA, sizeof(data5));
    
    pp_vals[0] = data1;
    pp_vals[1] = data2;
    pp_vals[2] = data3;
    pp_vals[3] = data4;
    pp_vals[4] = data5;

    clock_gettime(CLOCK_MONOTONIC, &start);    
    win_index = SELECT_Execute(pp_vals, 5, sizeof(data1), &p_select);
    clock_gettime(CLOCK_MONOTONIC, &stop);
    
    printf("win_data = 0x%02x\n", ((uint8*)p_select)[0]);
    printf("win_index = %zu\n", win_index);

    printf("Selection time: %f ms\n", SELECT_TimeDiffMs(start, stop));
    
    return;
}
#endif

/* Execute some functions to test the library. */              
void SELECT_Test(void)
{
    uint8 vals[4] = {1, 2, 3, 4};
    void* pp_vals[4]; 
    void* p_select = NULL;
    int i = 0;
    
    printf("Input: ");
    for (i = 0; i < 4; i++) {
        pp_vals[i] = &(vals[i]);    
        printf("%p ", pp_vals[i]);
    }
    printf("\n");
    
    SELECT_Execute(1000, pp_vals, 4, &p_select);
    
    printf("Selection: %p\n", p_select);
    
    return;
}

