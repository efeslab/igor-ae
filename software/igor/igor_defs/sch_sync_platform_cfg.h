/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

#ifndef _sch_sync_platform_cfg_h_
#define _sch_sync_platform_cfg_h_

/* Number of minor frames in 1 sec major frame. */
#define SCH_SYNC_TOTAL_SLOTS (500)

/* Number of minor frames between toggling LED. */
#define SCH_SYNC_MINOR_BW_STATUS (50)

/* Number of entries per minor frame. */
#define SCH_SYNC_ENTRIES_PER_SLOT (10)

/* Total entries in schedule table. */ 
#define SCH_SYNC_TABLE_ENTRIES (SCH_SYNC_TOTAL_SLOTS * SCH_SYNC_ENTRIES_PER_SLOT)

/* Time to wait for apps to start (in ms). */
#define SCH_SYNC_STARTUP_TIMEOUT (50000)

/* Number of microseconds level must be stable. */
#define SCH_SYNC_IRQ_FILTER (10) 

#endif
