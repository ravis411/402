/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
 #include "passportSetup.h"




/**********************
* Cashier
*************************/


void cashiercheckAndGoOnBreak(int myLine){
  int freeOrAvailable = false;
  int i;
  int tempState;

  if(Get(THEEND,0)){Set(cashierState, myLine, ONBREAK); Release(cashierLineLock); passportDestroy(); Exit(0);}

  for(i = 0; i < CLERKCOUNT; i++){
  	tempState = Get(cashierState, i);
    if(i != myLine && ( tempState == AVAILABLE || tempState == BUSY ) ){
      freeOrAvailable = true;
      break;
    }
  }

  if(freeOrAvailable && (Rand() % 100 ) < 25){
    Set(cashierState, myLine, ONBREAK);
    Acquire(printLock);
      PrintString("Cashier ", sizeof("Cashier ") );
      PrintInt(myLine);
      PrintString(" is going on break.\n", sizeof(" is going on break.\n") );
    Release(printLock);
    Wait(cashierBreakCV, cashierLineLock);
    if(Get(THEEND,0)){Set(cashierState, myLine, ONBREAK); Release(cashierLineLock); passportDestroy(); Exit(0);}
    Set(cashierState, myLine, BUSY);
    Acquire(printLock);
      PrintString("Cashier ", sizeof("Cashier ") );
      PrintInt(myLine);
      PrintString(" is coming off break.\n", sizeof(" is coming off break.\n") );
    Release(printLock);
  }
    
    /*Release(cashierLineLock);*/
    /*Acquire(managerLock);*/
    /*if(Get(checkedOutCount,0) == (CUSTOMERCOUNT + SENATORCOUNT)){Release(managerLock); passportDestroy(); Exit(0);}*/
    /*if(Get(THEEND,0)){Release(managerLock); passportDestroy(); Exit(0);}*/
  
    /*Release(managerLock);*/
    Yield();
    /*Acquire(cashierLineLock);*/

}

int CashierGetMyLine(){
  int myLine;
  Acquire(CashierMyLineLock);
  myLine = Get(CashierMyLine, 0);
  Set(CashierMyLine, 0, myLine + 1);
  Release(CashierMyLineLock);
  return myLine;
}

void Cashier(){
  int myLine;
  int money = 0;
  int customerFromLine;/*0 no line, 1 bribe line, 2 regular line*/
  int customerSSN;
  
  myLine = CashierGetMyLine();

  while (true){

    if(clerkCheckForSenator()) continue;

    Acquire(cashierLineLock);


    if (Get(cashierBribeLineCount, myLine) > 0){
      customerFromLine = 1;
      Signal(cashierBribeLineCV[myLine], cashierLineLock);
      Set(cashierState, myLine, SIGNALEDCUSTOMER);
    }
    else if (Get(cashierLineCount, myLine) > 0){
      customerFromLine = 2;
      Signal(cashierLineCV[myLine], cashierLineLock);
      Set(cashierState, myLine, SIGNALEDCUSTOMER);
    }
    else{
      customerFromLine = 0;
      cashiercheckAndGoOnBreak(myLine);
      Release(cashierLineLock);
    }


    if (customerFromLine != 0){

      Acquire(printLock);
          PrintString("Cashier ", sizeof("Cashier ") );
          PrintInt(myLine);
          PrintString(" has signalled a Customer to come to their counter.\n",
               sizeof(" has signalled a Customer to come to their counter.\n") );
      Release(printLock);


      Acquire(cashierLock[myLine]);
      Release(cashierLineLock);

      Wait(cashierCV[myLine], cashierLock[myLine]);

      customerSSN = Get(cashierSharedDataSSN, myLine);

      if(customerFromLine == 1){
        money += 500;
        Acquire(printLock);
            PrintString("Cashier ", sizeof("Cashier ") );
            PrintInt(myLine);
            PrintString(" has received $500 from Customer ", sizeof(" has received $500 from Customer ") );
            PrintInt(customerSSN);
            PrintString(".\n", 2);
        Release(printLock);
        Yield();
      }
      

      Acquire(printLock);
            PrintString("Cashier ", sizeof("Cashier ") );
            PrintInt(myLine);
            PrintString(" has received SSN ", sizeof(" has received SSN "));
            PrintInt(customerSSN);
            PrintString(" from Customer ", sizeof(" from Customer ") );
            PrintInt(customerSSN);
            PrintString(".\n", 2);
        Release(printLock);


      if (Get(passportCompletion, customerSSN) == 0) {
        
        Acquire(printLock);
            PrintString("Cashier ", sizeof("Cashier ") );
            PrintInt(myLine);
            PrintString(" has received the $100 from Customer ", sizeof(" has received the $100 from Customer "));
            PrintInt(customerSSN);
            PrintString(" before certification. They are to go to the back of my line.\n", 
                 sizeof(" before certification. They are to go to the back of my line.\n") );
        Release(printLock);
        Set(cashierRejection, customerSSN, 1);
      }
      else {
        Set(cashierRejection, customerSSN, 0);
         Acquire(printLock);
            PrintString("Cashier ", sizeof("Cashier ") );
            PrintInt(myLine);
            PrintString(" has verified that Customer ", sizeof(" has verified that Customer "));
            PrintInt(customerSSN);
            PrintString(" has been certified by a PassportClerk.\n", 
                 sizeof(" has been certified by a PassportClerk.\n") );
        Release(printLock);

        Signal(cashierCV[myLine], cashierLock[myLine]);
        Wait(cashierCV[myLine], cashierLock[myLine]);
       
        Acquire(printLock);
            PrintString("Cashier ", sizeof("Cashier ") );
            PrintInt(myLine);
            PrintString(" has received the $100 from Customer ", sizeof(" has received the $100 from Customer "));
            PrintInt(customerSSN);
            PrintString(" after certification.\n", sizeof(" after certification.\n") );
        Release(printLock);

        Set(doneCompletely, customerSSN, 1);
        Acquire(printLock);
            PrintString("Cashier ", sizeof("Cashier ") );
            PrintInt(myLine);
            PrintString(" has provided Customer ", sizeof(" has provided Customer "));
            PrintInt(customerSSN);
            PrintString(" their completed passport.\n", sizeof(" their completed passport.\n") );
        Release(printLock);
        Signal(cashierCV[myLine], cashierLock[myLine]);
    
      }
      Release(cashierLock[myLine]);
    }

  }


}/*End Cashier*/







int
main()
{

	passportSetup();
	Cashier();


	
    Exit(0);		/* and then we're done */
}
