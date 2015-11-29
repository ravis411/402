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
	char input[15];
	startPass();


	PrintString("How many Customers: ", sizeof("How many Customers: "));
	Read(input, sizeof(input), ConsoleInput);

	PrintString("Input: ", sizeof("Input: "));
	PrintString(input, sizeof(input));
	PrintString("\n", 1);


	
    Exit(0);		/* and then we're done */
}