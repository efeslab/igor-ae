/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _afdx_lib_
#define _afdx_lib_

#include <stdbool.h>
#include <time.h>

#include "afdx_lib_vldefs.h"

/*
** Public Defines
*/

#define LIB_NAME "AFDX_LIB"

/*
** Public Structure Declarations
*/

/* Table entry mapping VL to socket.*/
typedef struct {
    /* Index of array is the VL ID. */
    int socks[AFDX_LIB_VL_COUNT];            /* socket file descriptor */
    struct timespec last[AFDX_LIB_VL_COUNT]; /* time last frame was sent */
                                             /* only used for send VLs */
} AFDX_SockTable_t;

/*
** Exported Global Variables
*/

/* Sockets used for sending/receiving. */
extern AFDX_SockTable_t AFDX_SendSocks;
extern AFDX_SockTable_t AFDX_RecvSocks;

/*
** Public Function Prototypes
*/

int AFDX_LIB_GetPortNum(int vl_id);
int32 AFDX_LIB_LibEntry(void);
int AFDX_LIB_Init(void);
void AFDX_LIB_Cleanup(void);

#endif
