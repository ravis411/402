/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"

char ughh[] = "Initialized Passport Office and 1 manager.\n";

int
main()
{
	
	startPass();
	PrintString(ughh, sizeof(ughh));

    Exit(0);		/* and then we're done */
}