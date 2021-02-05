/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _log_lib_
#define _log_lib_

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include "log_codes.h"

/*
** Public Defines
*/

/* Used for initialization and cleanup. */
int32 LOG_LibEntry(void);
int   LOG_LibInit(void);
void  LOG_LibCleanup(void);

/* Log an activity for a given application. */
void LOG_LogWrite(LOG_AppCode_t app,
                  LOG_ActCode_t act);

#endif
