/* .c 
 *    
 *
 *
 *
 *
 */

#include "syscall.h"
#include "passportSetup.h"






 /********************************************************************************
 *********************************************************************************
 *********************************************************************************
 *
 *  Customer
 *
 *********************************************************************************
 *********************************************************************************/

/*Wait outside or something there's a Senator present*/
void customerSenatorPresentWaitOutside(int SSN){
  int temp;

  Acquire(printLock);
  PrintString("Customer ", sizeof("Customer ")); 
  PrintInt(SSN);
  PrintString(" is going outside the PassportOffice because there is a Senator present.\n", 
    sizeof(" is going outside the PassportOffice because there is a Senator present.\n"));
  Release(printLock);

  /*Go outside.*/
  temp = Get(customersPresentCount, 0);
  temp--;
  Set(customersPresentCount, 0, temp);

  temp = Get(passportOfficeOutsideLineCount, 0);
  temp++;
  Set(passportOfficeOutsideLineCount, 0, temp);
  
  Wait(passportOfficeOutsideLineCV, managerLock);
  /*Can go back inside now.*/


  temp = Get(customersPresentCount, 0);
  temp++;
  Set(customersPresentCount, 0, temp);

  temp = Get(passportOfficeOutsideLineCount, 0);
  temp--;
  Set(passportOfficeOutsideLineCount, 0, temp);
}

/* Checks if a senator is present. Then goes outside if there is.*/
int customerCheckSenator(int SSN){
  int present;
  Acquire(managerLock);
  present = Get(senatorPresentWaitOutSide, 0);

  if(present)
    customerSenatorPresentWaitOutside(SSN);

  Release(managerLock);
  return present;
}

int customerCheckIn(){
  int SSN;
  int temp;
  Acquire(managerLock);

  temp = Get(customersPresentCount, 0);
  temp++;
  Set(customersPresentCount, 0, temp);

  Release(managerLock);

  Acquire(SSNLock);
  SSN = Get(SSNCount, 0);
  Set(SSNCount, 0, (SSN + 1));
  Release(SSNLock);
  return SSN;
}

/*To tell the manager they did a great job and let him know we're done.*/
void customerCheckOut(int SSN){
  Acquire(managerLock);

  temp = Get(customersPresentCount, 0);
  temp--;
  Set(customersPresentCount, 0, temp);

  temp = Get(checkedOutCount, 0);
  temp++;
  Set(checkedOutCount, 0, temp);

  Release(managerLock);
  Acquire(printLock);
  PrintString("Customer ", sizeof("Customer ") );
  PrintInt(SSN);
  PrintString(" is leaving the Passport Office.\n", sizeof(" is leaving the Passport Office.\n"));
  Release(printLock);
  destroy();
  Exit(0);
}







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
  
  if(pictureClerkState[myLine] != AVAILABLE){
    if(!bribe){
      pictureClerkLineCount[myLine]++;
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
      pictureClerkLineCount[myLine]--;
      if(pictureClerkState[myLine] != SIGNALEDCUSTOMER){
        Release(pictureClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
    }else{
      pictureClerkBribeLineCount[myLine]++;
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
      pictureClerkBribeLineCount[myLine]--;
      if(pictureClerkState[myLine] != SIGNALEDCUSTOMER){
        Release(pictureClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
      *money -= 500;
    }
  }

  pictureClerkState[myLine] = BUSY;
  Release(pictureClerkLineLock);

  Acquire(pictureClerkLock[myLine]);
 
  pictureClerkSharedDataSSN[myLine] = SSN;
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


  while(pictureClerkSharedDataPicture[myLine] == 0) {
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
      pictureClerkSharedDataPicture[myLine] = 0;
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
      pictureClerkSharedDataPicture[myLine] = 1;
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

  if(passportClerkState[myLine] != AVAILABLE){
    if(!bribe){
      passportClerkLineCount[myLine]++;
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
      passportClerkLineCount[myLine]--;
      if(passportClerkState[myLine] != SIGNALEDCUSTOMER){
        Release(passportClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
    }else{
      passportClerkBribeLineCount[myLine]++;
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
      passportClerkBribeLineCount[myLine]--;
      if(passportClerkState[myLine] != SIGNALEDCUSTOMER){
        Release(passportClerkLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
      *money -= 500;
    }
  }


  passportClerkState[myLine] = BUSY;
  Release(passportClerkLineLock);

  Acquire(passportClerkLock[myLine]);


  passportClerkSharedDataSSN[myLine] = SSN;
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
  if(passportPunishment[SSN] == 1) {
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
  
  if(cashierState[myLine] != AVAILABLE){
    if(!bribe){
      cashierLineCount[myLine]++;
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in regular line for Cashier ", sizeof(" has gotten in regular line for Cashier ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(cashierLineCV[myLine], cashierLineLock);
      cashierLineCount[myLine]--;
      if(cashierState[myLine] != SIGNALEDCUSTOMER){
        Release(cashierLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
    }else{
      cashierBribeLineCount[myLine]++;
      Acquire(printLock);
          PrintString(myType, 8);
          PrintString(" ", 1);
          PrintInt(SSN);
          PrintString(" has gotten in bribe line for Cashier ", sizeof(" has gotten in bribe line for Cashier ") );
          PrintInt(myLine);
          PrintString(".\n", 2);
      Release(printLock);
      Wait(cashierBribeLineCV[myLine], cashierLineLock);
      cashierBribeLineCount[myLine]--;
      if(cashierState[myLine] != SIGNALEDCUSTOMER){
        Release(cashierLineLock);
        if(customerCheckSenator(SSN))
          return false;
      }
      *money -= 500;
      
    }
  }

  cashierState[myLine] = BUSY;
  Release(cashierLineLock);


  Acquire(cashierLock[myLine]);


  cashierSharedDataSSN[myLine] = SSN;
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

  if (cashierRejection[SSN] == 1) {
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





















/**********************
* Customer
**********************/
void Customer(){
  int appClerkDone = 0;
  int pictureClerkDone = 0;
  int passportClerkDone = 0;
  int cashierDone = 0;
  int SSN = -1;
  int money = (Rand()%4)*500 + 100;
  int appClerkFirst = true || Rand() % 2;
  int i;

  SSN = customerCheckIn();


  while(1){

    /*Check if a senator is present and wait outside if there is.*/
    customerCheckSenator(SSN);

    if( !(appClerkDone) && (appClerkFirst || pictureClerkDone) ){ /*Go to applicationClerk*/
      appClerkDone = customerApplicationClerkInteraction(SSN, &money, 0);
      Exit();/*Temp test only app clerk...*/
    }
    else if( !pictureClerkDone ){
      /*Go to the picture clerk*/
      pictureClerkDone = customerPictureClerkInteraction(SSN, &money, 0);
    }
    else if(!passportClerkDone){
      passportClerkDone = customerPassportClerkInteraction(SSN, &money, 0);
      if (!passportClerkDone) { for (i = 0; i < Rand() % 901 + 100; i++) { Yield(); } }
    }
    else if(!cashierDone){
      cashierDone = customerCashierInteraction(SSN, &money, 0);
      if (!cashierDone) { for (i = 0; i < Rand() % 901 + 100; i++) { Yield(); } }
    }
    else{
      /*This terminates the customer should go at end.*/
      Acquire(printLock);
      PrintString("Customer ", sizeof("Customer "));
      PrintInt(SSN);
      PrintString(" money: ", sizeof(" money: "));
      PrintInt(money);
      PrintString("\n", 1);
      Release(printLock);
      customerCheckOut(SSN);
    }
  }

  Exit(0);
}/*End Customer*/






int
main()
{
  setup();
	Customer();

  Exit(0);
}
