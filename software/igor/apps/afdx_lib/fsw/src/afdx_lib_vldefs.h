/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _afdx_lib_vldefs_
#define _afdx_lib_vldefs_

#include <stdbool.h>

/*
** Public Definitions
*/

/* Number of VLs that can be enabled. */
#define AFDX_LIB_VL_COUNT (730)

/*
** Public Structure Declarations
*/

/* IP address table entry */
typedef struct {
    uint8 cpu_id;  /* CFE CPU ID of target (e.g. 3) */
    char* ip_addr; /* IP address of target (e.g. "10.0.0.21") */
} AFDX_IpEntry_t;

/* Virtual link definition table entry */
typedef struct {
    bool  is_used; /* true to enable, else false */
    int   vl_id;   /* critical traffic ID (0 - (AFDX_LIB_VL_COUNT - 1)) */
    char* src_id;  /* CFE CPU ID of source (e.g. "1") */
    char* dst_ids; /* CFE CPU IDs of destinations (e.g. "2,3") */
} AFDX_VlEntry_t;

/*
** Exported Global Variables
*/

/* IP addresses defined in tables/ directory. */
extern AFDX_IpEntry_t AFDX_LIB_IpTable[CPU_COUNT];

/* VLs defined in tables/ directory. */
extern AFDX_VlEntry_t AFDX_LIB_VlTable[AFDX_LIB_VL_COUNT];

/*
** Public Function Prototypes
*/

#endif
