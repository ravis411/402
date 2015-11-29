/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"

char ughh[] = "Initialized Passport Office and 1 manager";

int
main()
{
	
	startPass();

	while(true){
		if(Get(STOPPASS, 0)){
			break;
		}
		PrintString(ughh, sizeof(ughh));
		Sleep(1);Yield();
		PrintString(".", 1);
		Sleep(1);Yield();
		PrintString(".", 1);
		Sleep(1);Yield();
		PrintString(".", 1);
		Sleep(1);Yield();
		PrintString("\b\b\b", 3);
		Yield();
		Yield();
	}



    Exit(0);		/* and then we're done */
}