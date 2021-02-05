/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <pigpio.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>

#include "log_codes.h"
#include "log_lib.h"

/*
** Function Declarations
*/

/*
** Constants
*/

#define LIB_NAME "LOG_LIB"

/* Path to log file. */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define LOG_FILE_PATH "./log_cpu=" STR(CFE_CPU_ID) ".dat"

/*
** Global Variables
*/

/* File stream to write to. */
static FILE* p_log = NULL;

/* Mutex to protect access to stream. */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* System tick at startup. */
static uint32 tick_start;

/*
** Function Definitions
*/

/* Entry point for the library. */
int32 LOG_LibEntry(void) {
    return CFE_SUCCESS;
}

/* Initialize the library. */
int LOG_LibInit(void)
{        
    char* header = "App, Activity, Time (us)\n";

    if (!LOG_ENABLE) {
        return 0; /* nothing to do */
    }

    if ((p_log = fopen(LOG_FILE_PATH, "w")) == NULL) {
        printf("%s: Error opening file %s\n", LIB_NAME, LOG_FILE_PATH);
        return -1; 
    }  

    fprintf(p_log, "%s", header);

    tick_start = gpioTick();
    
    OS_printf("%s: Lib Initialized\n", LIB_NAME);
    
    return 0;
}

/* Shutdown the library. */
void LOG_LibCleanup(void)
{
    if (!LOG_ENABLE) {
        return; /* nothing to do */
    }
    
    fclose(p_log);

    return;
}

/* Log an activity for a given application. */
void LOG_LogWrite(LOG_AppCode_t app,
                  LOG_ActCode_t act)
{
    uint32 tick_now = 0;
    uint32 tick_log = 0;

    assert(app < LOG_APP_NUM);
    assert(act < LOG_ACT_NUM);

    if (!LOG_ENABLE) {
        return; /* nothing to do */
    }
    
    pthread_mutex_lock(&log_mutex);

    tick_now = gpioTick();
    tick_log = tick_now - tick_start;
    fprintf(p_log, "%d %d %ld\n", app, act, (long int)tick_log);    
    
    pthread_mutex_unlock(&log_mutex);
    
    return;
}

