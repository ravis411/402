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
	char ughh[] = "Initialized Passport Office and 1 manager";
	startPass();

	while(true){
		if(Get(STOPPASS, 0)){
			break;
		}
		PrintString(ughh, sizeof(ughh));
		Sleep(1);
		PrintString(".", 1);
		Sleep(1);
		PrintString(".", 1);
		Sleep(1);
		PrintString(".", 1);
		Sleep(1);
		PrintString("\r", 1);
	}



    Exit(0);		/* and then we're done */
}