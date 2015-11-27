/* PAppClerk.c 
 *    
 *	The passport office application clerk.
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"



/**********************************
* ApplicationClerk
**********************************/

/*Utility for applicationClerk to gon on brak
* Assumptions: called with clerkLineLock*/
void applicationClerkcheckAndGoOnBreak(int myLine){
  /*TODO DONT GO ON BREAK FOR NOW...*/
  /*Only go on break if there is at least one other clerk*/
  int freeOrAvailable = 0;
  int i;
  int tempSate;
  for(i = 0; i < CLERKCOUNT; i++){
  	tempSate = Get(applicationClerkState, i);
    if(i != myLine && ( tempSate == AVAILABLE || tempSate == BUSY ) ){
      freeOrAvailable = 1;
      break;
    }
  }
  /*There is at least one clerk...go on a break.*/
  if(freeOrAvailable){

  	Set(applicationClerkState, myLine, ONBREAK);

    Acquire(printLock);
    PrintString("ApplicationClerk ", sizeof("ApplicationClerk ") );
    PrintInt(myLine);
    PrintString(" is going on break.\n", sizeof(" is going on break.\n") );
    Release(printLock);

    Wait(applicationClerkBreakCV, applicationClerkLineLock);
    Set(applicationClerkState, myLine, BUSY);

    Acquire(printLock);
    PrintString("ApplicationClerk ", sizeof("ApplicationClerk ") );
    PrintInt(myLine);
    PrintString(" is coming off break.\n", sizeof(" is coming off break.\n") );
    Release(printLock);

  }else{
    /*If everyone is on break...
    * applicationClerkState[myLine] = AVAILABLE;*/
    Release(applicationClerkLineLock);
    /*Should we go to sleep?*/
    Acquire(managerLock);
    if(Get(checkedOutCount, 0) == (CUSTOMERCOUNT + SENATORCOUNT)){Release(managerLock); passportDestroy(); Exit(0);}
    Release(managerLock);
    Yield();
    Acquire(applicationClerkLineLock);
  }
  /*applicationClerkState[myLine] = AVAILABLE;*/
}

int getMyApplicationLine(){
  int myLine;
  Acquire(ApplicationMyLineLock);
  myLine = Get(ApplicationMyLine, 0);
  Set(ApplicationMyLine, 0, (myLine + 1) );
  Release(ApplicationMyLineLock);
  return myLine;
}

/*ApplicationClerk - an application clerk accepts a completed Application. 
// A completed Application requires an "completed" application and a Customer "social security number".
// You can assume that the Customer enters the passport office with a completed passport application. 
// The "social security number" can be a random number, or a sequentially increasing number. 
// In any event, it must be a unique number for each Customer.
//
// PassportClerks "record" that a Customer has a completed application. 
// The Customer must pass the application to the PassportClerk. 
// This consists of giving the ApplicationClerk their Customer "social security number" - their personal number.
// The application is assumed to be passed, it is not explicitly provided in the shared data between the 2 threads.
// The ApplicationClerk then "records" that a Customer, with the provided social security number, has a completed application. 
  //This information is used by the PassportClerk. Customers are told when their application has been "filed".
// Any money received from Customers wanting to move up in line must be added to the ApplicationClerk received money amount.
// ApplicationClerks go on break if they have no Customers in their line.
// There is always a delay in an accepted application being "filed".
// This is determined by a random number of 'currentThread->Yield() calls - the number is to vary from 20 to 100.*/
void ApplicationClerk(){
  int myLine;
  int money = 0;
  int customerFromLine;/*0 no line, 1 bribe line, 2 regular line*/
  int customerSSN;
  int i;
  int tempState;

  myLine = getMyApplicationLine();


  while(1){

    if(clerkCheckForSenator()) continue; /*Waiting for senators to enter just continue.*/

    Acquire(applicationClerkLineLock);

    /*If there is someone in my bribe line*/
    if( Get(applicationClerkBribeLineCount, myLine) > 0){
      customerFromLine = 1;
      Signal(applicationClerkBribeLineCV[myLine], applicationClerkLineLock);
      Set(applicationClerkState, myLine, SIGNALEDCUSTOMER);
    }else if(Get(applicationClerkLineCount, myLine) > 0){/*if there is someone in my regular line*/
      customerFromLine = 2;
      Signal(applicationClerkLineCV[myLine], applicationClerkLineLock);
      Set(applicationClerkState, myLine, SIGNALEDCUSTOMER);
    }else{
      /*No Customers
      //Go on break if there is another clerk*/
      customerFromLine = 0;
      applicationClerkcheckAndGoOnBreak(myLine);
      Release(applicationClerkLineLock);
    }

    /*Should only do this when we have a customer...*/
    if(customerFromLine != 0){

      /*printf("ApplicationClerk %i has signalled a Customer to come to their counter.\n", myLine);*/
      Acquire(printLock);
      PrintString("ApplicationClerk ", sizeof("ApplicationClerk "));
      PrintInt(myLine);
      PrintString(" has signalled a Customer to come to their counter.\n",
         sizeof(" has signalled a Customer to come to their counter.\n"));
      Release(printLock);

      Acquire(applicationClerkLock[myLine]);
      Release(applicationClerkLineLock);
      /*wait for customer data*/
      Wait(applicationClerkCV[myLine], applicationClerkLock[myLine]);
      /*Customer Has given me their SSN?
      //And I have a lock*/
      customerSSN = Get(applicationClerkSharedData, myLine);
      
      /*Customer from bribe line? //maybe should be separate signalwait  ehh?*/
      if(customerFromLine == 1){
        money += 500;
        /*printf("ApplicationClerk %i has received $500 from Customer %i.\n", myLine, customerSSN);*/
       Acquire(printLock);
          PrintString("ApplicationClerk ", sizeof("ApplicationClerk "));
          PrintInt(myLine);
          PrintString(" has received $500 from Customer ", sizeof(" has received $500 from Customer "));
          PrintInt(customerSSN);
          PrintString(".\n", 2);
        Release(printLock);
        Yield();/*Just to change things up a bit.*/
      }
      

      /*printf("ApplicationClerk %i has received SSN %i from Customer %i.\n", myLine, customerSSN, customerSSN);*/
      Acquire(printLock);
          PrintString("ApplicationClerk ", sizeof("ApplicationClerk "));
          PrintInt(myLine);
          PrintString(" has received SSN ", sizeof(" has received SSN "));
          PrintInt(customerSSN);
          PrintString(" from Customer ", sizeof(" from Customer "));
          PrintInt(customerSSN);
          PrintString(".\n", 2);
        Release(printLock);
      
      /*Signal Customer that I'm Done.*/
      Signal(applicationClerkCV[myLine], applicationClerkLock[myLine]);
      Release(applicationClerkLock[myLine]);

      /*yield for filing time*/
      for(i = 0; i < Rand()%81 + 20; i++) { Yield(); }
      
      /*TODO: NEED TO ACQUIRE A LOCK FOR THIS!!*/
      Set(applicationCompletion, customerSSN, 1);
      /*printf("ApplicationClerk %i has recorded a completed application for Customer %i.\n", myLine, customerSSN);*/
      Acquire(printLock);
          PrintString("ApplicationClerk ", sizeof("ApplicationClerk "));
          PrintInt(myLine);
          PrintString(" has recorded a completed application for Customer ", 
               sizeof(" has recorded a completed application for Customer "));
          PrintInt(customerSSN);
          PrintString(".\n", 2);
        Release(printLock);
    }/*end if have customer*/

  }

}/*End ApplicationClerk*/











int
main()
{
	passportSetup();
	ApplicationClerk();
	
    Exit(0);		/* and then we're done */
}
