/*
** Andrew Loveless <loveless@umich.edu>
** License details in LICENSE.txt
*/

/*
** General tests of various Igor library functions.
*/

#include "cfe.h"
#include "cfe_platform_cfg.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include "afdx_api.h"
//#include "filter_lib.h"
#include "vote_lib.h"
#include "bcast_lib.h"
#include "comp_lib.h"
#include "exchange_lib.h"
#include "select_lib.h"

/*
** Function Declarations
*/

/*
** Constants
*/

/*
** Structure Declarations
*/

/*
** Global Variables
*/

/*
** Internal Function Definitions
*/

/*
** Public Function Definitions
*/

int main(void)
{
    //AFDX_Test();
    //VOTE_Test();
    //BCAST_Test();
    //COMP_Test();
    SELECT_Test();
    //EXCHANGE_Test();
                
    return 0;
}

