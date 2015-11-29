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
	char input[5];
	startPass();

	Read(input, 5, ConsoleInput);

	PrintString(input, 5);


	
    Exit(0);		/* and then we're done */
}
