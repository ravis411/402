/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
 #include "passportSetup.h"




void passportClerkcheckAndGoOnBreak(int myLine){
	int tempState;
  int freeOrAvailable = false;
  int i;
  for(i = 0; i < CLERKCOUNT; i++){
  	tempState = Get(passportClerkState, i);
    if(i != myLine && ( tempState == AVAILABLE || tempState == BUSY ) ){
      freeOrAvailable = true;
      break;
    }
  }

  if(freeOrAvailable){
    Set(passportClerkState, myLine, ONBREAK);
    Acquire(printLock);
      PrintString("PassportClerk ", sizeof("PassportClerk ") );
      PrintInt(myLine);
      PrintString(" is going on break.\n", sizeof(" is going on break.\n") );
    Release(printLock);
    Wait(passportClerkBreakCV, passportClerkLineLock);
    Set(passportClerkState, myLine, BUSY);
    Acquire(printLock);
      PrintString("PassportClerk ", sizeof("PassportClerk ") );
      PrintInt(myLine);
      PrintString(" is coming off break.\n", sizeof(" is coming off break.\n") );
    Release(printLock);
  }

  Release(passportClerkLineLock);
  Acquire(managerLock);
  if(Get(checkedOutCount,0) == (CUSTOMERCOUNT + SENATORCOUNT)){Release(managerLock); passportDestroy(); Exit(0);}
  Release(managerLock);
  Yield();
  Acquire(passportClerkLineLock);

  /*passportClerkState[myLine] = AVAILABLE;*/
}

int PassportGetMyLine(){
  int myLine;
  Acquire(PassportMyLineLock);
  myLine = Get(PassportMyLine,0);
  Set(PassportMyLine, 0, myLine + 1);
  Release(PassportMyLineLock);
  return myLine;
}


void PassportClerk(){
  int myLine;
  int money = 0;
  int customerFromLine;/*0 no line, 1 bribe line, 2 regular line*/
  int i;
  int customerSSN;
  
  myLine = PassportGetMyLine();

  while(true){

    if(clerkCheckForSenator()) continue; 

    Acquire(passportClerkLineLock);

    if(Get(passportClerkBribeLineCount, myLine) > 0){
      customerFromLine = 1;
      Signal(passportClerkBribeLineCV[myLine], passportClerkLineLock);
      Set(passportClerkState, myLine, SIGNALEDCUSTOMER);
    }else if(Get(passportClerkLineCount, myLine) > 0){
      customerFromLine = 2;
      Signal(passportClerkLineCV[myLine], passportClerkLineLock);
      Set(passportClerkState, myLine, SIGNALEDCUSTOMER);
    }else{
      customerFromLine = 0;
      passportClerkcheckAndGoOnBreak(myLine);
      Release(passportClerkLineLock);
    }


    if(customerFromLine != 0){
      Acquire(printLock);
        PrintString("PassportClerk ", sizeof("PassportClerk ") );
        PrintInt(myLine);
        PrintString(" has signalled a Customer to come to their counter.\n",
             sizeof(" has signalled a Customer to come to their counter.\n") );
      Release(printLock);
      Acquire(passportClerkLock[myLine]);
      Release(passportClerkLineLock);

      Wait(passportClerkCV[myLine], passportClerkLock[myLine]);


      customerSSN = Get(passportClerkSharedDataSSN, myLine);

      if(customerFromLine == 1){
        money += 500;
        /*printf("PassportClerk %i has received $500 from Customer %i.\n", myLine, customerSSN);*/
        Acquire(printLock);
            PrintString("PassportClerk ", sizeof("PassportClerk ") );
            PrintInt(myLine);
            PrintString("has received $500 from Customer ",
                 sizeof("has received $500 from Customer ") );
            PrintInt(customerSSN);
            PrintString(".\n", 2);
        Release(printLock);
        Yield();
      }
      
       Acquire(printLock);
            PrintString("PassportClerk ", sizeof("PassportClerk ") );
            PrintInt(myLine);
            PrintString(" has received SSN ", sizeof(" has received SSN ") );
            PrintInt(customerSSN);
            PrintString(" from Customer ", sizeof(" from Customer ") );
            PrintInt(customerSSN);
            PrintString(".\n", 2);
        Release(printLock);
      

      if(!(Get(applicationCompletion, customerSSN) == 1 && Get(pictureCompletion, customerSSN) == 1)) {
        Set(passportPunishment, customerSSN, 1);
        Acquire(printLock);
            PrintString("PassportClerk ", sizeof("PassportClerk ") );
            PrintInt(myLine);
            PrintString(" has determined that Customer ", sizeof(" has determined that Customer ") );
            PrintInt(customerSSN);
            PrintString(" does not have both their application and picture completed.\n",
                 sizeof(" does not have both their application and picture completed.\n") );
        Release(printLock);


        Signal(passportClerkCV[myLine], passportClerkLock[myLine]);
      }
      else {
        Set(passportPunishment, customerSSN, 0);
       
        Acquire(printLock);
            PrintString("PassportClerk ", sizeof("PassportClerk ") );
            PrintInt(myLine);
            PrintString(" has determined that Customer ", sizeof(" has determined that Customer ") );
            PrintInt(customerSSN);
            PrintString(" has both their application and picture completed.\n",
                 sizeof(" has both their application and picture completed.\n") );
        Release(printLock);

        Set(passportCompletion, customerSSN, true);

        Signal(passportClerkCV[myLine], passportClerkLock[myLine]);
        for(i = 0; i < Rand()%81 + 20; i++) {Yield(); }
        Acquire(printLock);
            PrintString("PassportClerk ", sizeof("PassportClerk ") );
            PrintInt(myLine);
            PrintString(" has recorded Customer ", sizeof(" has recorded Customer ") );
            PrintInt(customerSSN);
            PrintString(" passport documentation.\n",
                 sizeof(" passport documentation.\n") );
        Release(printLock);
      }

      Release(passportClerkLock[myLine]);
    }

  }
  



}/*End PassportClerk*/








int
main()
{

	passportSetup();
	PassportClerk();


	
    Exit(0);		/* and then we're done */
}
