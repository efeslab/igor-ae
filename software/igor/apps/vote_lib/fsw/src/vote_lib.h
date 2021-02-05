/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _vote_lib_
#define _vote_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdbool.h>

/*
** Public Defines
*/

/* Return codes for library. */
#define VOTE_SUCCESS (0)
#define VOTE_FAILED  (-1)

/* Maximum number of values to compare. */
#define VOTE_MAX_COMPARE (50)

/*
** Public Function Declarations
*/

/* Used for initialization and cleanup. */
int32 VOTE_LibEntry(void);

/**
** Perform a majority vote of binary values in an array.
**
** Each value must be either a 0x01 or 0x00 byte.
**
** \param[in]  pp_vals Array of pointers to values to vote.
** \param[in]  num     Number of pointers in array.
** \param[out] p_out   Set to majority (if exists).
**
** \return VOTE_SUCCESS if majority is found.
** \return VOTE_FAILED if no majority.
*/
int VOTE_FindMajBin(uint8** pp_vals,
                    size_t num,
                    uint8* p_out);

/**
** Perform a majority vote of arbitrary values in an array.
**
** The vote uses the Boyer-Moore algorithm, which takes O(n) time.
**
** \param[in]  pp_vals Array of pointers to values to vote.
** \param[in]  num     Number of pointers in array.
** \param[in]  len     Length of the values in bytes.
** \param[out] pp_out  Set to address of majority (if exists).
**
** \return VOTE_SUCCESS if majority is found.
** \return VOTE_FAILED if no majority.
*/
int VOTE_FindMaj(void** pp_vals,
                 size_t num,
                 size_t len,
                 void** pp_out);

/**
** Find the most common arbitrary value in an array.
**
** Done by sorting the array then iterating, which takes O(nlogn) time.
** If there are multiple most common values, then any could be the result.
**
** \param[in]  pp_vals Array of pointers to values to compare.
**                     Note that it gets reordered by the function.
** \param[in]  num     Number of pointers in array.
** \param[in]  len     Length of the values in bytes.
** \param[out] pp_out  Set to address of most common value.
** \param[out] p_count Set to frequency of most common value.
**
** \return None
*/
void VOTE_FindCom(void** pp_vals,
                  size_t num,
                  size_t len,
                  void** pp_out,
                  size_t* p_count);

/**
** Perform a mid-value selection of arbitrary values in an array.
**
** Done by sorting the array, then selecting the (num/2) largest value.
**
** \param[in]  pp_vals Array of pointers to values to compare.
** \param[in]  num     Number of pointers in array.
** \param[in]  len     Length of the values in bytes.
** \param[out] p_index Set to index of selection in pp_vals array.
**
** \return None
*/
void VOTE_FindMidVal(void** pp_vals,
                     size_t num,
                     size_t len,
                     size_t* p_index);
                        
/**
** Execute some functions to test the library.
** \return None
*/              
void VOTE_Test(void);

#endif

