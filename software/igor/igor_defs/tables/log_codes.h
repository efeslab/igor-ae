/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _log_codes_
#define _log_codes_

/* App codes for LOG_LogWrite(). */
typedef enum {
    LOG_APP_SCH            = 0,  /* the scheduler */
    LOG_APP_SIM            = 1,  /* the sim interface */
    LOG_APP_READ_SENSOR    = 2,  /* read data from sim */
    LOG_APP_SEND_ACTUATOR  = 3,  /* send results to actuator */
    LOG_APP_AGREEMENT      = 4,  /* agree on input */
    LOG_APP_SOURCE_SELECT  = 5,  /* perform source selection */
    LOG_APP_FILTER_SENSOR  = 6,  /* perform filtering on sensor data */    
    LOG_APP_COMPUTE_0      = 7,  /* computation 0 */
    LOG_APP_COMPUTE_1      = 8,  /* computation 1 */
    LOG_APP_COMPUTE_2      = 9,  /* computation 2 */
    LOG_APP_COMPUTE_3      = 10, /* computation 3 */
    LOG_APP_COMPUTE_4      = 11, /* computation 4 */    
    LOG_APP_COMPUTE_ALL    = 12, /* all computations */    
    LOG_APP_STATE_CONS     = 13, /* state consolidation */ 
    LOG_APP_STATE_DISPERSE = 14, /* state dispersal */    
    LOG_APP_FILTER_0       = 15, /* filtering instance 0 */
    LOG_APP_FILTER_1       = 16, /* filtering instance 1 */ 
    LOG_APP_FILTER_2       = 17, /* filtering instance 2 */
    LOG_APP_FILTER_3       = 18, /* filtering instance 3 */ 
    LOG_APP_FILTER_4       = 19, /* filtering instance 4 */
    LOG_APP_PARALLEL_START = 20, /* start of combined filtering/execution */
        
    LOG_APP_NUM
} LOG_AppCode_t;

/* Activity codes for LOG_LogWrite(). */
typedef enum {
    LOG_ACT_IRQ_RECV  = 0, /* received an interrupt */
    LOG_ACT_APP_START = 1, /* app started running or sent request */
    LOG_ACT_APP_STOP  = 2, /* app finished running or received result */
    LOG_ACT_NUM
} LOG_ActCode_t;

#endif
