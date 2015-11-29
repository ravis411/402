/* PPicClerk.c 
 *    
 *
 *	The distributed passport office picture clerk.
 *
 *
 */

#include "syscall.h"
 #include "passportSetup.h"




/*****************************************
* PictureClerk
*******************************************/


/*Utility for applicationClerk to gon on brak
// Assumptions: called with clerkLineLock*/
void pictureClerkcheckAndGoOnBreak(int myLine){
  int i;
  int tempState;
  int freeOrAvailable = false;

  if(Get(THEEND,0)){Set(pictureClerkState, myLine, ONBREAK); Release(pictureClerkLineLock); passportDestroy(); Exit(0);}

  for(i = 0; i < CLERKCOUNT; i++){
  	tempState = Get(pictureClerkState, i);
    if(i != myLine && ( tempState == AVAILABLE || tempState == BUSY ) ){
      freeOrAvailable = true;
      break;
    }
  }

  if(freeOrAvailable && (Rand() % 10 ) < 3){
    Set(pictureClerkState, myLine, ONBREAK);
    Acquire(printLock);
      PrintString("PictureClerk ", sizeof("PictureClerk ") );
      PrintInt(myLine);
      PrintString(" is going on break.\n", sizeof(" is going on break.\n") );
    Release(printLock);
    Wait(pictureClerkBreakCV, pictureClerkLineLock);
    if(Get(THEEND,0)){Set(pictureClerkState, myLine, ONBREAK); Release(pictureClerkLineLock); passportDestroy(); Exit(0);}
    Set(pictureClerkState, myLine, BUSY);
    Acquire(printLock);
      PrintString("PictureClerk ", sizeof("PictureClerk ") );
      PrintInt(myLine);
      PrintString(" is coming off break.\n", sizeof(" is coming off break.\n") );
    Release(printLock);
  }

  /*Check if the day is over...*/
  /*Release(pictureClerkLineLock);*/
  /*Acquire(managerLock);*/
  /*if(Get(checkedOutCount, 0) == (CUSTOMERCOUNT + SENATORCOUNT)){Release(managerLock); passportDestroy(); Exit(0);}*/
  /*if(Get(THEEND,0)){Release(managerLock); passportDestroy(); Exit(0);}*/
  /*if(Get(THEEND,0)){Set(passportClerkState, myLine, ONBREAK); Release(passportClerkLineLock); passportDestroy(); Exit(0);}*/
  /*Release(managerLock);*/
  Yield();
  /*Acquire(pictureClerkLineLock);*/

}

int PictureGetMyLine(){
  int myLine;
  Acquire(PictureMyLineLock);
  myLine = Get(PictureMyLine, 0);
  Set(PictureMyLine, 0, myLine + 1);
  Release(PictureMyLineLock);
  return myLine;
}

void PictureClerk(){
    int myLine;
    int money = 0;
    int customerFromLine;/*0 no line, 1 bribe line, 2 regular line*/
    int i;
    int customerSSN;

    myLine = PictureGetMyLine();
    /*Acquire(printLock);
    PrintString("PicClerk MyLine: ", sizeof("PicClerk MyLine: "));
    PrintInt(myLine);
    PrintString("\n", 1);
    Release(printLock);*/
    while(1){
  
      if(clerkCheckForSenator()) continue; /*Waiting for senators to enter just continue.*/

      Acquire(pictureClerkLineLock);
      /*DEBUG('p', "DEBUG: PictureClerk %i pICTURECLERKLINELOCK acquired top of while\n", myLine);*/

      /*If there is someone in my bribe line*/
      if(Get(pictureClerkBribeLineCount, myLine) > 0){
        customerFromLine = 1;
        Signal(pictureClerkBribeLineCV[myLine], pictureClerkLineLock);
        Set(pictureClerkState, myLine, SIGNALEDCUSTOMER);
      }else if(Get(pictureClerkLineCount, myLine) > 0){/*if there is someone in my regular line*/
        customerFromLine = 2;
        Signal(pictureClerkLineCV[myLine], pictureClerkLineLock);
        Set(pictureClerkState, myLine, SIGNALEDCUSTOMER);
      }else{
        /*Go on a break!*/
        customerFromLine = 0;
        pictureClerkcheckAndGoOnBreak(myLine);
        Release(pictureClerkLineLock);
        /*DEBUG('p', "DEBUG: PictureClerk %i pICTURECLERKLINELOCK released after checkbreak.\n", myLine);*/
      }

      /*Should only do this when we are BUSY? We have a customer...*/
      if(customerFromLine != 0){
        Acquire(printLock);
          PrintString("PictureClerk ", sizeof("PictureClerk "));
          PrintInt(myLine);
          PrintString(" has signalled a Customer to come to their counter.\n" ,
               sizeof(" has signalled a Customer to come to their counter.\n"));
        Release(printLock);

        Acquire(pictureClerkLock[myLine]);
      /*  pictureClerkSharedDataPicture[myLine] = 0;*/
        Release(pictureClerkLineLock);
        /*DEBUG('p', "DEBUG: PictureClerk %i pICTURECLERKLINELOCK released after signalling customer.\n", myLine);*/
        /*wait for customer data*/
        Wait(pictureClerkCV[myLine], pictureClerkLock[myLine]);
        /*Customer Has given me their SSN?
        //And I have a lock*/
        customerSSN = Get(pictureClerkSharedDataSSN, myLine);
        /*Customer from bribe line? //maybe should be separate signalwait  ehh?*/
      if(customerFromLine == 1){
        money += 500;
        /*printf("PictureClerk %i has received $500 from Customer %i.\n", myLine, customerSSN);*/
        Acquire(printLock);
          PrintString("PictureClerk ", sizeof("PictureClerk "));
          PrintInt(myLine);
          PrintString(" has received $500 from Customer " ,
               sizeof(" has received $500 from Customer "));
          PrintInt(customerSSN);
          PrintString(".\n", 2);
        Release(printLock);

        Yield();/*Just to change things up a bit.*/
      }
      
        /*printf("PictureClerk %i has received SSN %i from Customer %i.\n", myLine, customerSSN, customerSSN);*/
        Acquire(printLock);
          PrintString("PictureClerk ", sizeof("PictureClerk "));
          PrintInt(myLine);
          PrintString("  has received SSN ", sizeof("  has received SSN ") );
          PrintInt(customerSSN);
          PrintString(" from Customer " , sizeof(" from Customer "));
          PrintInt(customerSSN);
          PrintString(".\n", 2);
        Release(printLock);

        Set(pictureClerkSharedDataPicture, myLine, 0);
        while(Get(pictureClerkSharedDataPicture, myLine) == 0) {
          /*Taking picture*/
          /*printf("PictureClerk %i has taken a picture of Customer %i.\n", myLine, customerSSN);*/
          Acquire(printLock);
              PrintString("PictureClerk ", sizeof("PictureClerk "));
              PrintInt(myLine);
              PrintString(" has taken a picture of Customer ", sizeof(" has taken a picture of Customer ") );
              PrintInt(customerSSN);
              PrintString(".\n", 2);
          Release(printLock);


          /*Signal Customer that I'm Done and show them the picture. Then wait for response.*/
          Signal(pictureClerkCV[myLine], pictureClerkLock[myLine]);
          Wait(pictureClerkCV[myLine], pictureClerkLock[myLine]);
          if(Get(pictureClerkSharedDataPicture, myLine) == 0)  {
            /*printf("PictureClerk %i has has been told that Customer %i does not like their picture.\n", myLine, customerSSN);*/
            Acquire(printLock);
              PrintString("PictureClerk ", sizeof("PictureClerk "));
              PrintInt(myLine);
              PrintString(" has taken been told that Customer ", sizeof(" has taken been told that Customer ") );
              PrintInt(customerSSN);
              PrintString(" does not like their picture.\n", sizeof(" does not like their picture.\n"));
            Release(printLock);
          }

        }
        /*printf("PictureClerk %i has has been told that Customer %i does like their picture.\n", myLine, customerSSN);*/
        Acquire(printLock);
          PrintString("PictureClerk ", sizeof("PictureClerk "));
          PrintInt(myLine);
          PrintString(" has taken been told that Customer ", sizeof(" has taken been told that Customer ") );
          PrintInt(customerSSN);
          PrintString(" does like their picture.\n", sizeof(" does like their picture.\n"));
        Release(printLock);
        /*Yield before submitting.*/
        /*Signal Customer that I'm Done.*/
        Signal(pictureClerkCV[myLine], pictureClerkLock[myLine]);
        for(i = 0; i < Rand()%81 + 20; i++) { Yield(); }
        /*printf("PictureClerk %i has recorded a completed picture for Customer %i.\n", myLine, customerSSN);*/
          Acquire(printLock);
              PrintString("PictureClerk ", sizeof("PictureClerk "));
              PrintInt(myLine);
              PrintString(" has recorded a completed picture for Customer ", sizeof(" has recorded a completed picture for Customer ") );
              PrintInt(customerSSN);
              PrintString(".\n", sizeof(".\n"));
            Release(printLock);
        Set(pictureCompletion, customerSSN, 1);


        Release(pictureClerkLock[myLine]);
      }
  
    }

}/*End PictureClerk*/



















int
main()
{

	passportSetup();
	PictureClerk();

	
    Exit(0);		/* and then we're done */
}
