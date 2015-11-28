/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"









/*The Customer's Interaction with the applicationClerk
*    Get their application accepted by the ApplicationClerk*/
int customerApplicationClerkInteraction(int SSN, int *money, int VIP){
  int myLine = -1;
  char* myType = MYTYPE(VIP);
  int bribe = (*money > 500) && (Rand()%2) && !VIP;/*VIPS dont bribe...*/
  int tempState;
  int temp;
  /*I have decided to go to the applicationClerk*/

  /*I should acquire the line lock*/
  Acquire(applicationClerkLineLock);
  /*lock acquired*/

  /*Can I go to counter, or have to wait? Should i bribe?*/
  /*Pick shortest line with clerk not on break*/
  /*Should i get in the regular line else i should bribe?*/
  if(!bribe){ /*Get in regular line*/
    myLine = pickShortestLine(applicationClerkLineCount, applicationClerkState);
  }else{ /*get in bribe line*/
    myLine = pickShortestLine(applicationClerkBribeLineCount, applicationClerkState);
  }
  
  /*I must wait in line*/
  tempState = Get(applicationClerkState, myLine);
  if(tempState != AVAILABLE){
    if(!bribe){
      temp = Get(applicationClerkLineCount, myLine);
      temp++;
      Set(applicationClerkLineCount, myLine, temp);
      /*printf("%s %i has gotten in regular line for ApplicationClerk %i.\n", myType, SSN, myLine);*/

      Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has gotten in regular line for ApplicationClerk ", 
          sizeof(" has gotten in regular line for ApplicationClerk ") );
      PrintInt(myLine);
      PrintString(".\n", 2);
      Release(printLock);

      Wait(applicationClerkLineCV[myLine], applicationClerkLineLock);
      temp = Get(applicationClerkLineCount, myLine);
      temp--;
      Set(applicationClerkLineCount, myLine, temp);
      /*See if the clerk for my line signalled me, otherwise check if a senator is here and go outside.*/
      tempState = Get(applicationClerkState, myLine);
      if(tempState != SIGNALEDCUSTOMER){
        Release(applicationClerkLineLock);
        if(customerCheckSenator(SSN))
          return 0;
      }
    }else{
      temp = Get(applicationClerkBribeLineCount, myLine);
      temp++;
      Set(applicationClerkBribeLineCount, myLine, temp);

      /*printf("%s %i has gotten in bribe line for ApplicationClerk %i.\n", myType, SSN, myLine);*/
      Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has gotten in bribe line for ApplicationClerk ", 
          sizeof(" has gotten in bribe line for ApplicationClerk ") );
      PrintInt(myLine);
      PrintString(".\n", 2);
      Release(printLock);
      
      Wait(applicationClerkBribeLineCV[myLine], applicationClerkLineLock);
      temp = Get(applicationClerkBribeLineCount, myLine);
      temp--;
      Set(applicationClerkBribeLineCount, myLine, temp);
      /*See if the clerk for my line signalled me, otherwise check if a senator is here and go outside.*/
      tempState = Get(applicationClerkState, myLine);
      if(tempState != SIGNALEDCUSTOMER){
        Release(applicationClerkLineLock);
        if(customerCheckSenator(SSN))
          return 0;
      }
      *money -= 500;
    }
  }
  /*Clerk is AVAILABLE*/
  Set(applicationClerkState, myLine, BUSY);
  Release(applicationClerkLineLock);
  /*Lets talk to clerk*/
  Acquire(applicationClerkLock[myLine]);
  /*Give my data to my clerk*/
  /*We already have a lock so put my SSN in applicationClerkSharedData*/
  Set(applicationClerkSharedData, myLine, SSN);
  /*printf("%s %i has given SSN %i to ApplicationClerk %i.\n", myType, SSN, SSN, myLine);*/
  Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has given SSN ", sizeof(" has given SSN "));
      PrintInt(SSN);
      PrintString(" to ApplicationClerk ", sizeof(" to ApplicationClerk "));
      PrintInt(myLine);
      PrintString(".\n", 2);
  Release(printLock);
  Signal(applicationClerkCV[myLine], applicationClerkLock[myLine]);
  /*Wait for clerk to do their job*/
  Wait(applicationClerkCV[myLine], applicationClerkLock[myLine]);
  
  /*Done*/
  Release(applicationClerkLock[myLine]);
  return 1;
}/*End customerApplicationClerkInteraction*/






/*The Customer's Interaction with the pictureClerk
//Get their picture accepted by the pictureClerk*/
int customerPictureClerkInteraction(int SSN, int *money, int VIP){
  int myLine = -1;
  char* myType = MYTYPE(VIP);
  int bribe = (*money > 500) && (Rand()%2) && !VIP;

  Acquire(pictureClerkLineLock);

  if(!bribe){
    myLine = pickShortestLine(pictureClerkLineCount, pictureClerkState);
  }else{
    myLine = pickShortestLine(pictureClerkBribeLineCount, pictureClerkState);
  }
  
  if(Get(pictureClerkState, myLine) != AVAILABLE){
    if(!bribe){
      Set(pictureClerkLineCount, myLine, Get(pictureClerkLineCount, myLine) + 1);
      /*printf("%s %i has gotten in regular line for PictureClerk %i.\n", myType, SSN, myLine);*/
       Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in regular line for PictureClerk ", 
              sizeof(" has gotten in regular line for PictureClerk ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(pictureClerkLineCV[myLine], pictureClerkLineLock);
      Set(pictureClerkLineCount, myLine, Get(pictureClerkLineCount, myLine) - 1 );
      if(Get(pictureClerkState, myLine) != SIGNALEDCUSTOMER){
        Release(pictureClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
    }else{
      Set(pictureClerkBribeLineCount, myLine, Get(pictureClerkBribeLineCount, myLine) + 1);
      /*printf("%s %i has gotten in bribe line for PictureClerk %i.\n", myType, SSN, myLine);*/
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in bribe line for PictureClerk ", 
              sizeof(" has gotten in bribe line for PictureClerk ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);

      Wait(pictureClerkBribeLineCV[myLine], pictureClerkLineLock);
      Set(pictureClerkBribeLineCount, myLine, Get(pictureClerkBribeLineCount, myLine) - 1);
      if(Get(pictureClerkState, myLine) != SIGNALEDCUSTOMER){
        Release(pictureClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
      *money -= 500;
    }
  }

  Set(pictureClerkState, myLine, BUSY);
  Release(pictureClerkLineLock);

  Acquire(pictureClerkLock[myLine]);
 
  Set(pictureClerkSharedDataSSN, myLine, SSN);
  Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has given SSN ", sizeof(" has given SSN "));
      PrintInt(SSN);
      PrintString(" to PictureClerk ", sizeof(" to PictureClerk ") );
      PrintInt(myLine);
      PrintString(".\n", 2);
  Release(printLock);


  Signal(pictureClerkCV[myLine], pictureClerkLock[myLine]);
  Wait(pictureClerkCV[myLine], pictureClerkLock[myLine]);


  while(Get(pictureClerkSharedDataPicture, myLine) == 0) {
    if(Rand()%10 > 7) {
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" does not like their picture from PictureClerk ",
               sizeof(" does not like their picture from PictureClerk "));
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Set(pictureClerkSharedDataPicture, myLine, 0);
      Signal(pictureClerkCV[myLine], pictureClerkLock[myLine]);

      Wait(pictureClerkCV[myLine], pictureClerkLock[myLine]);
    }
    else {
      /*printf("%s %i does like their picture from PictureClerk %i.\n", myType, SSN, myLine);*/
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" does like their picture from PictureClerk ",
               sizeof(" does like their picture from PictureClerk "));
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Set(pictureClerkSharedDataPicture, myLine, 1);
      Signal(pictureClerkCV[myLine], pictureClerkLock[myLine]);

      Wait(pictureClerkCV[myLine], pictureClerkLock[myLine]);
    }
  }
  Release(pictureClerkLock[myLine]);

  return true;
}/*End customerPictureClerkInteraction*/




int customerPassportClerkInteraction(int SSN, int *money, int VIP){
  int myLine = -1;
  char* myType = MYTYPE(VIP);
  int bribe = (*money > 500) && (Rand()%2) && !VIP;

  Acquire(passportClerkLineLock);


  if(!bribe){
    myLine = pickShortestLine(passportClerkLineCount, passportClerkState);
  }else{ 
    myLine = pickShortestLine(passportClerkBribeLineCount, passportClerkState);
  }

  if(Get(passportClerkState, myLine) != AVAILABLE){
    if(!bribe){
      Set(passportClerkLineCount,myLine, ( Get(passportClerkLineCount, myLine) + 1 ) );
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in regular line for PassportClerk ", 
              sizeof(" has gotten in regular line for PassportClerk ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(passportClerkLineCV[myLine], passportClerkLineLock);
      Set(passportClerkLineCount, myLine, Get(passportClerkLineCount, myLine) - 1);
      if(Get(passportClerkState, myLine) != SIGNALEDCUSTOMER){
        Release(passportClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
    }else{
      Set(passportClerkBribeLineCount, myLine, Get(passportClerkBribeLineCount, myLine) + 1);
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in bribe line for PassportClerk ", 
              sizeof(" has gotten in bribe line for PassportClerk ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(passportClerkBribeLineCV[myLine], passportClerkLineLock);
      Set(passportClerkBribeLineCount, myLine, Get(passportClerkBribeLineCount, myLine) - 1 );
      if(Get(passportClerkState, myLine) != SIGNALEDCUSTOMER){
        Release(passportClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
      *money -= 500;
    }
  }


  Set(passportClerkState, myLine, BUSY);
  Release(passportClerkLineLock);

  Acquire(passportClerkLock[myLine]);


  Set(passportClerkSharedDataSSN, myLine, SSN);
  Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has given SSN ", 
          sizeof(" has given SSN ") );
      PrintInt(SSN);
      PrintString(" to PassportClerk ", sizeof(" to PassportClerk "));
      PrintInt(myLine);
      PrintString(".\n", 2);
  Release(printLock);
  Signal(passportClerkCV[myLine], passportClerkLock[myLine]);


  Wait(passportClerkCV[myLine], passportClerkLock[myLine]);
  if(Get(passportPunishment, SSN) == 1) {
    Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has gone to PassportClerk ", 
          sizeof(" has gone to PassportClerk ") );
      PrintInt(myLine);
      PrintString(" too soon. They are going to the back of the line.\n", 
           sizeof(" too soon. They are going to the back of the line.\n"));
  Release(printLock);
    Release(passportClerkLock[myLine]);
    return false;
  }

  Release(passportClerkLock[myLine]);

  return true;

}/*//End of customerPassportClerkInteraction*/





int customerCashierInteraction(int SSN, int *money, int VIP){
  int myLine = -1;
  char* myType = MYTYPE(VIP);
  int bribe = (*money > 500) && (Rand()%2) && !VIP;


  Acquire(cashierLineLock);

  if(!bribe){
    myLine = pickShortestLine(cashierLineCount, cashierState);
  }else{ 
    myLine = pickShortestLine(cashierBribeLineCount, cashierState);
  }
  
  if(Get(cashierState, myLine) != AVAILABLE){
    if(!bribe){
      Set(cashierLineCount, myLine, Get(cashierLineCount, myLine) + 1);
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in regular line for Cashier ", sizeof(" has gotten in regular line for Cashier ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(cashierLineCV[myLine], cashierLineLock);
      Set(cashierLineCount, myLine, Get(cashierLineCount, myLine) - 1);
      if(Get(cashierState, myLine) != SIGNALEDCUSTOMER){
        Release(cashierLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
    }else{
      Set(cashierBribeLineCount, myLine, Get(cashierBribeLineCount, myLine) + 1);
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in bribe line for Cashier ", sizeof(" has gotten in bribe line for Cashier ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(cashierBribeLineCV[myLine], cashierLineLock);
      Set(cashierBribeLineCount, myLine, Get(cashierBribeLineCount, myLine) - 1);
      if(Get(cashierState, myLine) != SIGNALEDCUSTOMER){
        Release(cashierLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
      *money -= 500;
      
    }
  }

  Set(cashierState, myLine, BUSY);
  Release(cashierLineLock);


  Acquire(cashierLock[myLine]);


  Set(cashierSharedDataSSN, myLine, SSN);
   Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has given SSN ", sizeof(" has given SSN ") );
      PrintInt(SSN);
      PrintString(" to Cashier ", sizeof(" to Cashier "));
      PrintInt(myLine);
      PrintString(".\n", 2);
  Release(printLock);
  Signal(cashierCV[myLine], cashierLock[myLine]);

  Wait(cashierCV[myLine], cashierLock[myLine]);

  if (Get(cashierRejection, SSN) == 1) {
    Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has gone to Cashier ", 
          sizeof(" has gone to Cashier ") );
      PrintInt(myLine);
      PrintString(" too soon. They are going to the back of the line.\n", 
           sizeof(" too soon. They are going to the back of the line.\n"));
    Release(printLock);
    Release(cashierLock[myLine]);
    return false;
  }
  else {
    *money -= 100;
    Acquire(printLock);
      PrintString(myType, 8);
      PrintString(" ", 1);
      PrintInt(SSN);
      PrintString(" has given Cashier ", 
          sizeof(" has given Cashier ") );
      PrintInt(myLine);
      PrintString(" $100.\n", sizeof(" $100.\n"));
    Release(printLock);
    Signal(cashierCV[myLine], cashierLock[myLine]);

    Wait(cashierCV[myLine], cashierLock[myLine]);
  }

  Release(cashierLock[myLine]);

  return true;



}/*End of customerCashierInteraction*/











/**************************************
**
**  Senator
**
******************************************/
int senatorArriveAtPassportOffice(){
  int SSN;

  Acquire(SSNLock);
    SSN = Get(SSNCount, 0);
    Set(SSNCount, 0, SSN + 1);
  Release(SSNLock);

  Acquire(managerLock);
  while(!Get(senatorSafeToEnter, 0) ){
    Set(senatorLineCount, 0, Get(senatorLineCount, 0) + 1);
    Wait(senatorLineCV, managerLock);
    Set(senatorLineCount,0, Get(senatorLineCount, 0) - 1 );
  }
  Set(senatorPresentCount, 0, Get(senatorPresentCount, 0) + 1 );
  Release(managerLock);

  return SSN;
}

void senatorLeavePassportOffice(int SSN){
  Acquire(managerLock);
  Set(senatorPresentCount, 0, Get(senatorPresentCount, 0) - 1 );
  Set(checkedOutCount, 0, Get(checkedOutCount, 0) + 1 );
  Release(managerLock);

  Acquire(printLock);
      PrintString("Senator ", sizeof("Senator "));
      PrintInt(SSN);
      PrintString(" is leaving the Passport Office.\n", sizeof(" is leaving the Passport Office.\n") );
  Release(printLock);
  passportDestroy();
  Exit(0);
}
void Senator(){
  int SSN;
  int i;
  int money = (Rand()%4)*500 + 100;
  int appClerkDone = false;
  int pictureClerkDone = false;
  int passportClerkDone = false;
  int cashierDone = false; 
  int appClerkFirst = Rand() % 2;



  /*Check in*/
  SSN = senatorArriveAtPassportOffice();

  /*Safe to do 'normal' interactions now....*/

  while(!cashierDone){

    if( !(appClerkDone) && (appClerkFirst || pictureClerkDone) ){
      appClerkDone = customerApplicationClerkInteraction(SSN, &money, 1);
    }
    else if( !pictureClerkDone ){
      pictureClerkDone = customerPictureClerkInteraction(SSN, &money, 1);
    }
    else if(!passportClerkDone){
      passportClerkDone = customerPassportClerkInteraction(SSN, &money, 1);
      if (!passportClerkDone) { for (i = 0; i < Rand() % 901 + 100; i++) { Yield(); } }
    }
    else if(!cashierDone){
      cashierDone = customerCashierInteraction(SSN, &money, 1);
      if (!cashierDone) { for (i = 0; i < Rand() % 901 + 100; i++) { Yield(); } }
    }
  }

  /*Done lets leave*/
  senatorLeavePassportOffice(SSN);


} /*End Senator*/

int
main()
{

	passportSetup();
	Senator();



    Exit(0);
}
