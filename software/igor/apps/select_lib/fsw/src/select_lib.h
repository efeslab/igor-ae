/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _select_lib_
#define _select_lib_

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
int32 SELECT_LibEntry(void);

#if 0
/**
** Perform a source selection of the provides sensor values.
**
** This is simulated by doing mid-value selections of the provided data
** in 4 byte chunks.
**
** \param[in]  pp_vals Array of pointers to values to choose between.
** \param[in]  num     Number of pointers in array.
** \param[in]  len     Length of the values in bytes.
** \param[out] pp_out  Set to address of selected value.
**
** \return Index of the selected value in pp_vals.
*/
size_t SELECT_Execute(void** pp_vals,
                      size_t num,
                      size_t len,
                      void** pp_out);                   
#endif                      

/**
** Perform a source selection of the provides sensor values.
**
** \param[in]  time_ms Time for source selection to take in milliseconds.
** \param[in]  pp_vals Array of pointers to values to choose between.
** \param[in]  num     Number of pointers in array.
** \param[out] pp_out  Set to address of selected value.
**
** \return Index of the selected value in pp_vals.
*/
size_t SELECT_Execute(size_t time_ms,
                      void** pp_vals,
                      size_t num,
                      void** pp_out);

/**
** Execute some functions to test the library.
** \return None
*/              
void SELECT_Test(void);
           
#endif

