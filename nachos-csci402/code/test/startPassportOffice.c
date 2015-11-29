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
	char *input = "  ";
	startPass();

	Read(input, sizeof(input), ConsoleInput);

	PrintString(input, sizeof(input));


	
    Exit(0);		/* and then we're done */
}