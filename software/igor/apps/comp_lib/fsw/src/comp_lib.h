/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _comp_lib_
#define _comp_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdbool.h>

/*
** Public Defines
*/

/*
** Public Function Declarations
*/

/* Used for initialization and cleanup. */
int32 COMP_LibEntry(void);

/**
** Perform a dummy computation that takes the given amount of time.
** The output of the computation is the bitwise complement of the input.
**
** \param[in]  time_ms Time for computation to take in milliseconds.
** \param[in]  p_input Input data to perform computation on.
** \param[in]  len_in  Length of input in bytes.
** \param[out] p_out   Bitwise complement of the input.
** \param[in]  len_out Length of output in bytes.
**
** \return None
*/
void COMP_Execute(size_t time_ms,
                  uint8* p_input,
                  size_t len_in,
                  uint8* p_out,
                  size_t len_out);
                  
/**
** Execute some functions to test the library.
** \return None
*/              
void COMP_Test(void);

#endif

