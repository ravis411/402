/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"



int
main()
{
	char input[2];
	stopPass();

	Read(input, 2, ConsoleInput);

	PrintString(input, 2);


	
    Exit(0);		/* and then we're done */
}
