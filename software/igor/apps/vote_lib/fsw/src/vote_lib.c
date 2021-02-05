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

#include "vote_lib.h"

/*
** Function Declarations
*/

/*
** Constants
*/

#define LIB_NAME "VOTE_LIB"

/* Get the number of elements in an array. */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/*
** Internal Function Definitions
*/

/* Comparison function used by qsort_r(). */
int VOTE_CompareVals(const void* a, const void* b, void* c)
{
    void* p_val1; /* pointer to first value. */
    void* p_val2; /* pointer to second value. */    
    size_t len;   /* length of values to compare */

    assert(a != NULL);
    assert(b != NULL);
    assert(c != NULL);
    
    p_val1 = *(void**)a;
    p_val2 = *(void**)b;
    len    = *(size_t*)c;
    
    return (memcmp(p_val1, p_val2, len));
}

/*
** Public Function Definitions
*/

/* Entry point for the library. */
int32 VOTE_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Perform a majority vote of binary values in an array. */
int VOTE_FindMajBin(uint8** pp_vals,
                    size_t num,
                    uint8* p_out)
{
    size_t sum = 0;
    int i = 0;

    assert(pp_vals != NULL); 
    assert(p_out != NULL); 
        
    for (i = 0; i < num; i++) {
        sum += *(pp_vals[i]);
    }

    if (sum > (num/2)) {
        *p_out = 0x01;
        return VOTE_SUCCESS;
    }

    if ((num - sum) > (num/2)) {
        *p_out = 0x00;
        return VOTE_SUCCESS;
    }    

    return VOTE_FAILED;
}

/* Perform a majority vote of arbitrary values in an array. */
int VOTE_FindMaj(void** pp_vals,
                 size_t num,
                 size_t len,
                 void** pp_out)
{
    int maj_index = 0; /* index of majority candidate */
    size_t count = 0;
    int i = 0;

    assert(pp_vals != NULL); 
    assert(pp_out != NULL); 
    assert(num > 0);

    /* Find a candidate for majority. */
    count = 1;
    for (i = 1; i < num; i++) {
        if (memcmp(pp_vals[maj_index], pp_vals[i], len) == 0) {
            count ++;
        } else {
            count--;
        }
        if (count == 0) {
            maj_index = i;
            count = 1;
        }
    }

    /* Check how often candidate occurs. */
    count = 0;
    for (i = 0; i < num; i++) {
        if ((i == maj_index) ||
            (memcmp(pp_vals[maj_index], pp_vals[i], len) == 0))
        {
            count++;
        }
    }   

    /* Check if candidate is majority. */
    if (count > (num/2)) {
        *pp_out = pp_vals[maj_index];
        return VOTE_SUCCESS;
    }
    
    /* No majority found. */
    *pp_out = NULL;    
    return VOTE_FAILED;
}

/* Find the most common arbitrary value in an array. */
void VOTE_FindCom(void** pp_vals,
                  size_t num,
                  size_t len,
                  void** pp_out,
                  size_t* p_count)
{
    int win_index = 0; /* index of winning value */
    int win_count = 0; /* matches for winning value */
    int count = 0;     /* number of matches */
    int i = 0;

    assert(pp_vals != NULL); 
    assert(pp_out != NULL); 
    assert(p_count != NULL);
    assert(num > 0);    

    if (num == 1) { /* only one item */
        *pp_out = pp_vals[0];
        return;
    }

    /* Sort pointers by the vals they point to. */
    /* Now all matching elems are next to each other. */
    qsort_r(pp_vals, num, sizeof(void*), VOTE_CompareVals, &len);

    /* Find most common element. */
    for (i = 1; i < num; i++) {
        if (memcmp(pp_vals[i], pp_vals[i - 1], len) == 0) {
            count++;
            if (count > win_count) {
                win_index = i;
                win_count = count;
            }
        } else {
            count = 0;
        }
    }

    *pp_out = pp_vals[win_index];
    *p_count = win_count + 1;

    return;
}

/* Perform a mid-value selection of arbitrary values in an array. */
void VOTE_FindMidVal(void** pp_vals,
                     size_t num,
                     size_t len,
                     size_t* p_index)
{
    void* pp_vals_sort[VOTE_MAX_COMPARE]; /* copy of input array */
    int i = 0;

    assert(pp_vals != NULL); 
    assert(p_index != NULL); 
    assert(num > 0);
    assert(num <= VOTE_MAX_COMPARE);

    if (num == 1) { /* only one item */
        *p_index = 0;
        return;
    }

    /* Get copy of array of pointers. */
    /* Needed since qsort will change the array. */
    memcpy(pp_vals_sort, pp_vals, num * sizeof(void*));

    /* Sort pointers by the vals they point to. */
    qsort_r(pp_vals_sort, num, sizeof(void*), VOTE_CompareVals, &len);
        
    /* Get index of winner in original array. */
    for (i = 0; i < num; i++) {
        if (pp_vals[i] == pp_vals_sort[num/2]) {
            *p_index = (size_t)i;
            break;
        }
    }

    return;
}

/* Execute some functions to test the library. */              
void VOTE_Test(void)
{
    uint16 vals[5] = {0xAABB, 0xBBCC, 0xBBCC, 0xAABB, 0xAABB};
    void* pp_vals[5]; 
    size_t num = NELEMS(vals);
    size_t len = sizeof(uint16);
    void* p_out = NULL;
    size_t count = 0;
    int ret = 0;
    int i = 0;

    for (i = 0; i < num; i++) {
        pp_vals[i] = &(vals[i]);    
    }
        
    //----------------------------
    printf("Values: ");
    for (i = 0; i < num; i++) {
        printf("0x%02x ", *((uint16*)pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMaj(pp_vals, num, len, &p_out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", *((uint16*)p_out));
    } else {
        printf("> Vote failed - no majority\n");    
    }
    VOTE_FindCom(pp_vals, num, len, &p_out, &count);
    printf("> Most common value: 0x%02x (count = %zu)\n", *((uint16*)p_out), count); 

    //----------------------------
    vals[4] = 0xBBCC;
    printf("Values: ");
    for (i = 0; i < num; i++) {
        printf("0x%02x ", *((uint16*)pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMaj(pp_vals, num, len, &p_out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", *((uint16*)p_out));
    } else {
        printf("> Vote failed - no majority\n");    
    }
    VOTE_FindCom(pp_vals, num, len, &p_out, &count);
    printf("> Most common value: 0x%02x (count = %zu)\n", *((uint16*)p_out), count); 
    
    //----------------------------
    vals[1] = 0xCCDD;
    printf("Values: ");
    for (i = 0; i < num; i++) {
        printf("0x%02x ", *((uint16*)pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMaj(pp_vals, num, len, &p_out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", *((uint16*)p_out));
    } else {
        printf("> Vote failed - no majority\n");    
    }
    VOTE_FindCom(pp_vals, num, len, &p_out, &count);
    printf("> Most common value: 0x%02x (count = %zu)\n", *((uint16*)p_out), count); 
        
    //----------------------------
    vals[0] = 0xCCDD;
    vals[2] = 0xCCDD;    
    printf("Values: ");
    for (i = 0; i < num; i++) {
        printf("0x%02x ", *((uint16*)pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMaj(pp_vals, num, len, &p_out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", *((uint16*)p_out));
    } else {
        printf("> Vote failed - no majority\n");    
    }    
    VOTE_FindCom(pp_vals, num, len, &p_out, &count);
    printf("> Most common value: 0x%02x (count = %zu)\n", *((uint16*)p_out), count); 

    //----------------------------
    vals[1] = 0xDDAA;
    vals[2] = 0xBBFF;    
    printf("Values: ");
    for (i = 0; i < num; i++) {
        printf("0x%02x ", *((uint16*)pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMaj(pp_vals, num, len, &p_out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", *((uint16*)p_out));
    } else {
        printf("> Vote failed - no majority\n");    
    }    
    VOTE_FindCom(pp_vals, num, len, &p_out, &count);
    printf("> Most common value: 0x%02x (count = %zu)\n", *((uint16*)p_out), count); 
    
    //----------------------------
    vals[3] = 0xBBCC;
    printf("Values: ");
    for (i = 0; i < num; i++) {
        printf("0x%02x ", *((uint16*)pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMaj(pp_vals, num, len, &p_out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", *((uint16*)p_out));
    } else {
        printf("> Vote failed - no majority\n");    
    }    
    VOTE_FindCom(pp_vals, num, len, &p_out, &count);
    printf("> Most common value: 0x%02x (count = %zu)\n", *((uint16*)p_out), count); 

    printf("====================================\n");
        
    uint8 bin_vals[5] = {1, 0, 0, 1, 1};
    uint8* bin_pp_vals[5]; 
    size_t bin_num = NELEMS(bin_vals);
    uint8 out;

    for (i = 0; i < bin_num; i++) {
        bin_pp_vals[i] = &(bin_vals[i]);    
    }
    
    //----------------------------
    printf("Bin Values: ");
    for (i = 0; i < bin_num; i++) {
        printf("0x%02x ", *(bin_pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMajBin(bin_pp_vals, bin_num, &out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", out);
    } else {
        printf("> Vote failed - no majority\n");
    }
    
    //----------------------------
    bin_vals[4] = 0x00;
    printf("Bin Values: ");
    for (i = 0; i < bin_num; i++) {
        printf("0x%02x ", *(bin_pp_vals[i]));    
    }
    printf("\n");
    ret = VOTE_FindMajBin(bin_pp_vals, bin_num, &out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", out);
    } else {
        printf("> Vote failed - no majority\n");
    }
    
    uint8 bin_vals2[6] = {1, 0, 0, 1, 1, 0};
    uint8* bin_pp_vals2[6]; 
    size_t bin_num2 = NELEMS(bin_vals2);

    for (i = 0; i < bin_num2; i++) {
        bin_pp_vals2[i] = &(bin_vals2[i]);    
    }

    //----------------------------
    printf("Bin Values: ");
    for (i = 0; i < bin_num2; i++) {
        printf("0x%02x ", *(bin_pp_vals2[i]));    
    }
    printf("\n");
    ret = VOTE_FindMajBin(bin_pp_vals2, bin_num2, &out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", out);
    } else {
        printf("> Vote failed - no majority\n");
    }
    
    //----------------------------
    bin_vals2[1] = 0x01;
    printf("Bin Values: ");
    for (i = 0; i < bin_num2; i++) {
        printf("0x%02x ", *(bin_pp_vals2[i]));    
    }
    printf("\n");
    ret = VOTE_FindMajBin(bin_pp_vals2, bin_num2, &out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", out);
    } else {
        printf("> Vote failed - no majority\n");
    }        
    
    //----------------------------
    bin_vals2[0] = 0x00;
    bin_vals2[1] = 0x00;
    printf("Bin Values: ");
    for (i = 0; i < bin_num2; i++) {
        printf("0x%02x ", *(bin_pp_vals2[i]));    
    }
    printf("\n");
    ret = VOTE_FindMajBin(bin_pp_vals2, bin_num2, &out);
    if (ret == VOTE_SUCCESS) {
        printf("> Vote succeeded (maj = 0x%02x)\n", out);
    } else {
        printf("> Vote failed - no majority\n");
    }       

    printf("====================================\n");

    uint16 mv_vals[3] = {0xCC, 0xAA, 0xBB};
    void* mv_pp_vals[3]; 
    size_t mv_num = 3;
    size_t win_index= 0;

    for (i = 0; i < mv_num; i++) {
        mv_pp_vals[i] = &(mv_vals[i]);    
    }    
    
    VOTE_FindMidVal(mv_pp_vals, mv_num, 2, &win_index);    
    
    printf("Mid-value index = %zu\n", win_index);
    
    return;
}

