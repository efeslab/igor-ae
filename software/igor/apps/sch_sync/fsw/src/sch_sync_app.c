/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#include <pigpio.h>

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#include "msg_ids.h"

#include "sch_sync_platform_cfg.h"
#include "sch_sync_tbldefs.h"
#include "afdx_lib.h"
#include "log_lib.h"
#include "log_codes.h"
#include "io_lib.h"
#include "bcast_lib.h"
#include "exchange_lib.h"
#include "state_lib.h"

/*
** Function Declarations
*/

void SCH_SYNC_IrqHandler(int gpio, int level, uint32 curr);
void SCH_SYNC_AppMain(void);
void SCH_SYNC_Cleanup(void);
int  SCH_SYNC_StartLibs(void);
void SCH_SYNC_StopLibs(void);
void SCH_SYNC_UpdateTablePos(void);
void SCH_SYNC_ProcessTableEntries(void);
void SCH_SYNC_ProcessTableEntry(uint32 index);

/*
** Constants
*/

#define APP_NAME "SCH_SYNC"

/* Mapping for GPIO pins. */
#define PIN_LED (17) /* GPIO 17 */
#define PIN_IRQ (4)  /* GPIO 4 */

/*
** Global Variables
*/

/* State variables. */
uint32 g_major_frame  = 0;
uint32 g_minor_frame  = 0; /* wraps each major frame */

/* Timing semaphore */
sem_t g_time_sem;

/*
** Function Definitions
*/

/* Handle interrupt from GPIO. */
void SCH_SYNC_IrqHandler(int gpio, int level, uint32 curr) {
    sem_post(&g_time_sem);
    return;
}

/* Entry point for the application. */
void SCH_SYNC_AppMain(void)
{
    uint32 RunStatus = CFE_ES_APP_RUN;
    int wait_again = 0;
    int ret = 0;

    #if 0
    /* Keep RPi at lowest clock speed. */
    system("sudo cp /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq "
           "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
    #endif
    
    #if 1
    /* Keep RPi at highest clock speed. */    
    system("sudo cp /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq "
           "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
    #endif     
           
    /* Allow cFS code to control CPU. */
    /* Avoid uncontrollable involuntary context switches. */
    system("sudo echo -1 >/proc/sys/kernel/sched_rt_runtime_us");    

    /* Initialize pigpio library (5us sampling rate). */
    if (gpioInitialise() < 0) {
        printf("%s: Error initializing pigpio library\n", APP_NAME);
        goto sch_error;
    }

    gpioWrite(PIN_LED, 0);
    gpioGlitchFilter(PIN_IRQ, SCH_SYNC_IRQ_FILTER);
    gpioSetAlertFunc(PIN_IRQ, SCH_SYNC_IrqHandler);
    
    /* Start any cFS libraries. */
    if (SCH_SYNC_StartLibs() != 0) {
        printf("%s: Error initializing cFS libraries\n", APP_NAME);
        goto sch_error;    
    }
    
    /* Install delete handler. */  
    if (OS_TaskInstallDeleteHandler(&SCH_SYNC_Cleanup) != OS_SUCCESS) {
        printf("%s: Error installing delete handler\n", APP_NAME);    
        goto sch_error;           
    }

    /* Initialize timing semaphore. */
    sem_init(&g_time_sem, 0, 0);
    
    /* Register app with Executive services. */
    CFE_ES_RegisterApp();
        
    printf("%s: App Initialized\n", APP_NAME);
    
    printf("**************************************\n"
           "*** Starting CFE CPU ID = %d ...  ***\n"
           "**************************************\n",
           CFE_PLATFORM_CPU_ID);
              
    /* Wait for apps to startup. */
    CFE_ES_WaitForStartupSync(SCH_SYNC_STARTUP_TIMEOUT);

    while (CFE_ES_RunLoop(&RunStatus) == TRUE)    
    {
        /* Wait for timing semaphore. */
        do {
            wait_again = 0;
            ret = sem_wait(&g_time_sem);
            if ( (ret == -1) && (errno == EINTR) ) { /* interrupted */
                wait_again = 1;
            }
        } while (wait_again);    
    
        SCH_SYNC_ProcessTableEntries();
        SCH_SYNC_UpdateTablePos();
        LOG_LogWrite(LOG_APP_SCH, LOG_ACT_IRQ_RECV);
    }

    CFE_ES_ExitApp(RunStatus);

    return;
    
sch_error:

    printf("%s: App exiting in error\n", APP_NAME);
    SCH_SYNC_Cleanup();
    CFE_ES_ExitApp(CFE_ES_APP_EXIT);

    return;
}
              
/* Cleanup app after shutdown. */
void SCH_SYNC_Cleanup(void)
{
    usleep(10000);

    SCH_SYNC_StopLibs();

    gpioWrite(PIN_LED, 0);
    gpioSetAlertFunc(PIN_IRQ, NULL);
    gpioTerminate();
    
    printf("%s: Cleaned up app\n", APP_NAME);  
    return;
}
              
/* Initialize select cFS libraries. */
int SCH_SYNC_StartLibs(void)
{
    uint16 msg[4] = { CFE_MAKE_BIG16(SCH_INIT_DONE_MID), CFE_MAKE_BIG16(0xC000),
                      CFE_MAKE_BIG16(0x0001), 0x00 };
    
    if (AFDX_LIB_Init() != 0) {
        printf("%s: Error initializing AFDX library\n", APP_NAME);    
        return -1;        
    }
    
    if (LOG_LibInit() != 0) {
        printf("%s: Error initializing logging library\n", APP_NAME);    
        return -1;
    }

    BCAST_LibInit();
    IO_LibInit();
    EXCHANGE_LibInit();
    STATE_LibInit();    

    if (CFE_SB_SendMsg((CFE_SB_Msg_t*) msg) != CFE_SUCCESS) {
        printf("%s: Error sending init done message\n", APP_NAME);
    }

    return 0;
}

/* Shutdown select cFS libraries. */         
void SCH_SYNC_StopLibs(void)
{
    STATE_LibCleanup();
    EXCHANGE_LibCleanup();
    IO_LibCleanup();
    BCAST_LibCleanup();
    LOG_LibCleanup();
    AFDX_LIB_Cleanup();
    return;
}

/* Update position in table. */
void SCH_SYNC_UpdateTablePos(void)
{
    g_minor_frame++;
    if (g_minor_frame == SCH_SYNC_TOTAL_SLOTS) {
        g_minor_frame = 0;
        g_major_frame++;
        printf("major = %zu\n", g_major_frame);
    }
   
    if ((g_minor_frame % SCH_SYNC_MINOR_BW_STATUS) == 0) {
        gpioWrite(PIN_LED, gpioRead(PIN_LED) ^ 1);
    }
           
    return;
}

/* Process entries in minor frame. */
void SCH_SYNC_ProcessTableEntries(void)
{
    uint32 index = g_minor_frame * SCH_SYNC_ENTRIES_PER_SLOT;
    int i = 0;
    
    /* Skip first major frame. */
    /* Allows interrupts to stabilize. */
    if (g_major_frame == 0) {
        return;
    }   

    /* Run entries in minor frame. */
    for (i = 0; i < SCH_SYNC_ENTRIES_PER_SLOT; i++) {
        SCH_SYNC_ProcessTableEntry(index + i);
    }

    return;
}

/* Process a single table entry. */
void SCH_SYNC_ProcessTableEntry(uint32 index)
{
    int32 Status = CFE_SUCCESS;
    uint16 mid = SCH_SYNC_Table[index];
    uint16 msg[4] = { 0x00, CFE_MAKE_BIG16(0xC000), CFE_MAKE_BIG16(0x0001), 0x00 };
    
    if (mid != 0) { /* entry is used */
        msg[0] = CFE_MAKE_BIG16(mid);
        Status = CFE_SB_SendMsg((CFE_SB_Msg_t*) msg);
        if (Status != CFE_SUCCESS) {
            printf("%s: Error sending wakeup message\n", APP_NAME);
        }
    }
    
    return;
}

