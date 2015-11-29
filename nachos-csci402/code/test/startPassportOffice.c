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
	char input[3];
	startPass();

	Read(input, 3, ConsoleInput);

	PrintString(input, 3);


	
    Exit(0);		/* and then we're done */
}