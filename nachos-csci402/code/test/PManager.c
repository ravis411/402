/* PManager.c 
 *    
 *	The distributed passport office Manager.
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"




/******************************************************
*******************************************************
*
* Manager
*
*******************************************************/



void managerBroadcastBreakLine(int managerBBLineLock, int managerBBLCV){
	Acquire(managerBBLineLock);
	Broadcast(managerBBLCV, managerBBLineLock);
	Release(managerBBLineLock);
}

void managerWakeUpAllClerks(){
	managerBroadcastBreakLine(applicationClerkLineLock, applicationClerkBreakCV);
	managerBroadcastBreakLine(pictureClerkLineLock, pictureClerkBreakCV);
	managerBroadcastBreakLine(passportClerkLineLock, passportClerkBreakCV);
	managerBroadcastBreakLine(cashierLineLock, cashierBreakCV);
}


/*This will put the clerks and the manager to sleep so everyone can Exit and nachos can clean up*/
void checkEndOfDay(){
  Acquire(managerLock);

  if (Get(checkedOutCount,0) == (CUSTOMERCOUNT + SENATORCOUNT)){
    /*DEBUG('s', "DEBUG: MANAGER: END OF DAY!\n");
    All the customers are gone
    Lets all EXIT!!!*/
    /*PrintString("Manager SETING END OF DAY!!", sizeof("Manager SETING END OF DAY!!") );*/
    Set(THEEND,0, 1);
    Release(managerLock);

    managerWakeUpAllClerks();

  /*currentThread->Finish();*/
    passportDestroy();
    Exit(0);
  }
  Release(managerLock);
}



/* managerCheckandWakeupCLERK
* checks if a line has more than 3 customers... 
* if so, signals a clerk on break
* Returns true if there was asleeping clerk and needed to wake one up*/
int managerCheckandWakeupCLERK(int managerCWCLineLock, int managerCWClineCount, int managerCWCState, int managerCWCBreakCV, int managerCWCount){
  int wakeUp = 0;/*should we wake up a clerk?*/
  int asleep = 0;/*is any clerk asleep?*/
  int i;
  Acquire(managerCWCLineLock);
  for(i = 0; i < managerCWCount; i++){
    if(Get(managerCWCState,i) == ONBREAK)
      asleep = 1;
    if(Get(managerCWClineCount, i) > 3)
      wakeUp = 1;
  }
  if(wakeUp && asleep){Signal(managerCWCBreakCV, managerCWCLineLock);}
  Release(managerCWCLineLock);
  return asleep && wakeUp;
}

/*managerCheckandWakupClerks()
* Checks all types of clerks for lines longer than 3 and wakes up a sleaping clerk if there is one*/
void managerCheckandWakupClerks(){
  /*Check Application Clerks*/
  if(managerCheckandWakeupCLERK(applicationClerkLineLock, applicationClerkLineCount, applicationClerkState, applicationClerkBreakCV, CLERKCOUNT)){
    Acquire(printLock);
    PrintString("Manager has woken up an ApplicationClerk.\n", sizeof("Manager has woken up an ApplicationClerk.\n"));
    Release(printLock);
  }

  /*Check Picture Clerks*/
  if(managerCheckandWakeupCLERK(pictureClerkLineLock, pictureClerkLineCount, pictureClerkState, pictureClerkBreakCV, CLERKCOUNT)){
    Acquire(printLock);
    PrintString("Manager has woken up a PictureClerk.\n", sizeof("Manager has woken up a PictureClerk.\n") );
    Release(printLock);
  }
  
  /*Check Passport Clerks*/
  if(managerCheckandWakeupCLERK(passportClerkLineLock, passportClerkLineCount, passportClerkState, passportClerkBreakCV, CLERKCOUNT)){
    Acquire(printLock);
    PrintString("Manager has woken up a PassportClerk.\n", sizeof("Manager has woken up a PassportClerk.\n") );
    Release(printLock);
  }

  /*Check Cashiers*/
  if(managerCheckandWakeupCLERK(cashierLineLock, cashierLineCount, cashierState, cashierBreakCV, CLERKCOUNT)){
    Acquire(printLock);
    PrintString("Manager has woken up a Cashier.\n", sizeof("Manager has woken up a Cashier.\n") );
    Release(printLock);
  }

}

/*Wake up customers in all lines*/
void managerBroacastLine(int* line, int* bribeLine, int lock, int count){
  int i;
  /*DEBUG('s', "DEBUG: MANAGER: BROADCAST acquiring lock %s.\n", lock->getName());*/
  Acquire(lock);
  /*DEBUG('s', "DEBUG: MANAGER: BROADCAST acquired lock %s.\n", lock->getName());*/
  for(i = 0; i < count; i++){
    Broadcast(line[i], lock);
    Broadcast(bribeLine[i], lock);
  }
  Release(lock);
  /*DEBUG('s', "DEBUG: MANAGER: BROADCAST released lock %s.\n", lock->getName());*/
}
void managerBroadcastCustomerLines(){
  /*Wake up all customers in line//So they can go outside*/

  /*App clerks*/
  managerBroacastLine(applicationClerkLineCV, applicationClerkBribeLineCV, applicationClerkLineLock, CLERKCOUNT);
  /*DEBUG('s', "DEBUG: MANAGER: FINISHED BROADCAST to applicaiton lines.\n");*/
  /*Picture clerks*/
  managerBroacastLine(pictureClerkLineCV, pictureClerkBribeLineCV, pictureClerkLineLock, CLERKCOUNT);
  /*DEBUG('s', "DEBUG: MANAGER: FINISHED BROADCAST to picture lines.\n");*/
  /*Passport Clerks*/
  managerBroacastLine(passportClerkLineCV, passportClerkBribeLineCV, passportClerkLineLock, CLERKCOUNT);
  /*DEBUG('s', "DEBUG: MANAGER: FINISHED BROADCAST to passport lines.\n");*/
  /*Cashiers*/
  managerBroacastLine(cashierLineCV, cashierBribeLineCV, cashierLineLock, CLERKCOUNT);
  /*DEBUG('s', "DEBUG: MANAGER: FINISHED BROADCAST to cashier lines.\n");*/
}

/*Checks if a sentor is present...does somehting*/
void managerSenatorCheck(){
  int senatorWaiting;
  int senatorsInside;
  int customersInside;
  int customersOutside;

  
  Acquire(managerLock);

  senatorWaiting = (Get(senatorLineCount,0) > 0);
  senatorsInside = (Get(senatorPresentCount,0) > 0);
  customersInside = (Get(customersPresentCount,0) > 0);
  customersOutside = (Get(passportOfficeOutsideLineCount,0) > 0);

  /*See if a senator is waiting in line...*/
  if(senatorWaiting){
    /*if(!senatorPresentWaitOutSide){ DEBUG('s', "DEBUG: MANAGER NOTICED A SENATOR!.\n"); }*/
    Set(senatorPresentWaitOutSide, 0, 1);

    /*Wake up customers in line so they go outside.*/
    if(customersInside){
      /*DEBUG('s', "DEBUG: MANAGER CUSTOMER PRESENT COUNT: %i.\n", customersPresentCount);*/
      Release(managerLock);
      managerBroadcastCustomerLines();
      /*DEBUG('s', "DEBUG: MANAGER: FINISHED BROADCAST to customers.\n");*/
      return;
    }
  }
  
  if(senatorWaiting && !customersInside){
    /*if(1 || !senatorSafeToEnter) DEBUG('s', "DEBUG: MANAGER: SENATORS SAFE TO ENTER.\n");*/
    Set(senatorSafeToEnter, 0, 1);
    Broadcast(senatorLineCV, managerLock);
    /*DEBUG('s', "DEBUG: MANAGER: FINISHED BROADCAST to senators.\n");*/
  }

  if(!senatorWaiting && !senatorsInside && Get(senatorSafeToEnter,0) ){
    /*if(senatorSafeToEnter){DEBUG('s', "DEBUG: SENATORS GONE CUSTOMERS COME BACK IN!.\n");}*/
    Set(senatorSafeToEnter, 0, 0);
    Set(senatorPresentWaitOutSide, 0, 0);
    Broadcast(passportOfficeOutsideLineCV, managerLock);
  }


  Release(managerLock);
}/*End managerSenatorCheck*/


void Manager(){
  int i;
  /*Untill End of Simulation*/
  while(1){
    for(i = 0; i < 1000; i++) { 
    

      /*SENATORS*/
      managerSenatorCheck();

      /*Check Lines Wake up Clerk if More than 3 in a line.*/
      managerCheckandWakupClerks();

      /*Check if all the customers are gone and let all the clerks go home*/
      checkEndOfDay();

      Yield(); 
      Yield(); 
      Yield(); 
    }/*Let someone else get the CPU*/
    
    /*Count and print money*/
    /*managerCountMoney();*/
  }

}/*End Manager*/















int
main()
{
	passportSetup();
	Manager();


	
    Exit(0);		/* and then we're done */
}
