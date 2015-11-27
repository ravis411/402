/*setup.h*/

/* Contains all code for the passport office setup. Includes locks, CVs, and MVs.*/

#include "syscall.h"



#define CLERKCOUNT  2
#define CUSTOMERCOUNT 2
#define SENATORCOUNT  0

#define MAXCLERKS 5
#define MAXCUSTOMERS 5/*50*/
#define MAXSENATORS 2 /*10*/


#define AVAILABLE 0
#define SIGNALEDCUSTOMER 1
#define BUSY 2
#define ONBREAK 3

char CUSTOMERTEXT[] = "Customer";
char SENATORTEXT[] = "Senator ";

#define true 1
#define false 0

/***********
* Locks
*********/
int applicationClerkLineLock;
int pictureClerkLineLock;
int passportClerkLineLock;
int cashierLineLock;
int managerLock;

int printLock;  /*For using the PrintSyscalls*/
int SSNLock;
int ApplicationMyLineLock;
int PictureMyLineLock;
int PassportMyLineLock;
int CashierMyLineLock;


int applicationClerkLock[MAXCLERKS];
int pictureClerkLock[MAXCLERKS];
int passportClerkLock[MAXCLERKS];
int cashierLock[MAXCLERKS];

/***************
* CVs
**********/
int passportOfficeOutsideLineCV; /* Outside line for when senators are present */
int senatorLineCV;

int applicationClerkLineCV[MAXCLERKS];
int applicationClerkBribeLineCV[MAXCLERKS];  /* //applicationClerk CVs */
int applicationClerkCV[MAXCLERKS];
int applicationClerkBreakCV; /*To keep track of clerks on break */

int pictureClerkLineCV[MAXCLERKS];
int pictureClerkBribeLineCV[MAXCLERKS];  /*pictureClerk CVs*/
int pictureClerkCV[MAXCLERKS];
int pictureClerkBreakCV;

int passportClerkLineCV[MAXCLERKS];
int passportClerkBribeLineCV[MAXCLERKS]; /*passportClerk CVs*/
int passportClerkCV[MAXCLERKS];
int passportClerkBreakCV;

int cashierLineCV[MAXCLERKS];
int cashierBribeLineCV[MAXCLERKS]; /* //passportClerk CVs */
int cashierCV[MAXCLERKS];
int cashierBreakCV;



/*******************************
*	Monitor Variables...
*
*********************************/

/************
* States
*********/
int applicationClerkState;/*[MAXCLERKS]; /* //applicationClerkState */
int pictureClerkState;/*[MAXCLERKS];  /* //applicationClerkState */
int passportClerkState;/*[MAXCLERKS];  /* //applicationClerkState */
int cashierState;/*[MAXCLERKS];  /* //applicationClerkState */


/***********
* Line Counts
************/
int applicationClerkLineCount;/*[MAXCLERKS];     /* //applicationClerkLineCount */
int applicationClerkBribeLineCount;/*[MAXCLERKS];    /* //applicationClerkBribeLineCount */
int pictureClerkLineCount;/*[MAXCLERKS];     /* //pictureClerkLineCount */
int pictureClerkBribeLineCount;/*[MAXCLERKS];    /* //pictureClerkBribeLineCount */
int passportClerkLineCount;/*[MAXCLERKS];      /* //passportClerkLineCount */
int passportClerkBribeLineCount;/*[MAXCLERKS];   /* //passportClerkBribeLineCount */
int cashierLineCount;/*[MAXCLERKS];      /* //cashierLineCount */
int cashierBribeLineCount;/*[MAXCLERKS];   /* //cashierBribeLineCount */

/*******************
* Shared Data
******************/
int applicationClerkSharedData;/*[MAXCLERKS];  /* //This can be used by the customer to pass SSN */
int pictureClerkSharedDataSSN;/*[MAXCLERKS]; /* //This can be used by the customer to pass SSN */
int pictureClerkSharedDataPicture;/*[MAXCLERKS]; /* // This can be used by the customer to pass acceptance of the picture */
int passportClerkSharedDataSSN;/*[MAXCLERKS]; /* //This can be used by the customer to pass SSN */

int applicationCompletion;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by passportCerkto verify that application has been completed */
int pictureCompletion;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by passportClerk to verify that picture has beeen completed */
int passportCompletion;/*[MAXCUSTOMERS + MAXSENATORS]; /* // Used by cashier to verify that the passport is complete */
int passportPunishment;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by passportClerk to punish bad people. */
int cashierSharedDataSSN;/*[MAXCUSTOMERS + MAXSENATORS]; /* //This can be used by the customer to pass SSN */
int cashierRejection;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by the cashier to reject customers. */
int doneCompletely;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by customer to tell when done. */

int SSNCount;
int ApplicationMyLine;	/*For clerks to know which line is theirs incremented upon clerk creation.*/
int PictureMyLine;
int PassportMyLine;
int CashierMyLine;

int customersPresentCount;/*For telling the manager we're in the office*/
int senatorPresentCount;
int checkedOutCount;  /*For the manager to put everyone to sleep when the customers have all finished*/
int senatorLineCount; /*For counting the sentors.//They wait in a private line for the manager while waiting for customers to leave.*/
int passportOfficeOutsideLineCount;
int senatorSafeToEnter; /*To tell senators when it is safe to enter*/
int senatorPresentWaitOutSide;/*Set by the manager to tell customers when a senator is present...*/

int THEEND;

/*********************
*
* End Variables
*
************************************************/



/*****************************
* Utility Functions...
***************************/

/*Used by customerInteractions to return customer/senator text...*/
char* MYTYPE(int VIP){
  if(VIP == 0){
    return CUSTOMERTEXT;
  }else if(VIP == 1){
    return SENATORTEXT;
  }else{
    return "NULL";
  }
}

/* Utility function to pick the shortest line
* Customer has already chosen type of line to get in just needs to pick which line
* Assumptions: The caller has a lock for the given MVs
*Parameters: 
  *lineCount: a vector of the lineCount
  *clerkState: a vector of the clerkState*/
int pickShortestLine(int pickShortestlineCount, int pickShortestclerkState){
  int myLine = -1;
  int lineSize = 1000;
  int i;
  int tempLineCount;
  int tempState;
  for(i=0; i < CLERKCOUNT; i++){
    tempLineCount = Get(pickShortestlineCount, i);
    tempState = Get(pickShortestclerkState, i);
    /*If lineCount < lineSize and clerk is not on break*/
    if(tempLineCount < lineSize && tempState != ONBREAK ){
      myLine = i;
      lineSize = tempLineCount;
    }
  }
  return myLine;  /*This is the shortest line*/
}/*End pickShortestLine*/


/*This may be necessary to check for race conditions while a senator is waiting outside
 * Before the customer leaves their line the clerk might think they are able to call them*/
int clerkCheckForSenator(){
  int i;
  int temp1;
  int temp2;
  /*DEBUG('s', "DEBUG: Clerk bout to check for senator.\n");*/
  Acquire(managerLock);
  temp1 = Get(senatorPresentWaitOutSide, 0);
  temp2 = Get(senatorSafeToEnter, 0);
  if(temp1 && !temp2){
    Release(managerLock);
    /*Lets just wait a bit...
    printf("DEBUG: Clerk yielding for senator.\n");*/
    for(i = 0; i < Rand()%780 + 20; i++) { Yield(); }
    return 1;
  }
  Release(managerLock);
  return 0;
}

/*******************
 End Utility Functions
********************/





/*Called for every clerk to Create the locks and whatnot...*/
void passportSetup(){
	int i;



  applicationClerkLineLock = CreateLock("appLineLock", sizeof("appLineLock"));
  pictureClerkLineLock = CreateLock("picLineLock", sizeof("picLineLock"));
  passportClerkLineLock = CreateLock("pasLineLock", sizeof("pasLineLock"));
  cashierLineLock = CreateLock("casLineLock", sizeof("casLineLock"));
  managerLock = CreateLock("manLock", sizeof("manLock"));
  printLock = CreateLock("printLock", sizeof("printLock"));
  SSNLock = CreateLock("SSNLock", sizeof("SSNLock"));
  ApplicationMyLineLock = CreateLock("appMyLock", sizeof("appMyLock"));
  PictureMyLineLock = CreateLock("picMyLock", sizeof("picMyLock"));
  PassportMyLineLock = CreateLock("pasMyLock", sizeof("pasMyLock"));
  CashierMyLineLock = CreateLock("casMyLock", sizeof("casMyLock"));

  applicationClerkBreakCV = CreateCondition("appBrkCV", sizeof("appBrkCV"));
  pictureClerkBreakCV = CreateCondition("picBrkCV", sizeof("picBrkCV"));
  passportClerkBreakCV = CreateCondition("pasBrkCV", sizeof("pasBrkCV"));
  cashierBreakCV = CreateCondition("casBrkCV", sizeof("casBrkCV"));
  passportOfficeOutsideLineCV = CreateCondition("pasOutCV", sizeof("pasOutCV"));
  senatorLineCV = CreateCondition("senLineCV", sizeof("senLineCV"));

  /*Monitor Variables...*/
	applicationClerkState = CreateMV("appState", sizeof("appState"), MAXCLERKS);
	pictureClerkState = CreateMV("picState", sizeof("picState"), MAXCLERKS);
	passportClerkState = CreateMV("pasState", sizeof("pasState"), MAXCLERKS);
	cashierState = CreateMV("casState", sizeof("casState"), MAXCLERKS);

	applicationClerkLineCount = CreateMV("appLC", sizeof("appLC"), MAXCLERKS);
	applicationClerkBribeLineCount = CreateMV("appBC", sizeof("appBC"), MAXCLERKS);
	pictureClerkLineCount = CreateMV("picLC", sizeof("picLC"), MAXCLERKS);
	pictureClerkBribeLineCount = CreateMV("picBC", sizeof("picBC"), MAXCLERKS);
	passportClerkLineCount = CreateMV("pasLC", sizeof("pasLC"), MAXCLERKS);
	passportClerkBribeLineCount = CreateMV("pasBC", sizeof("pasBC"), MAXCLERKS);
	cashierLineCount = CreateMV("casLC", sizeof("casLC"), MAXCLERKS);
	cashierBribeLineCount = CreateMV("casBC", sizeof("casBC"), MAXCLERKS);

	applicationClerkSharedData = CreateMV("appSD", sizeof("appSD"), MAXCLERKS);
	pictureClerkSharedDataSSN = CreateMV("picSSN", sizeof("picSSN"), MAXCLERKS);
	pictureClerkSharedDataPicture = CreateMV("picSDP", sizeof("picSDP"), MAXCLERKS);
	passportClerkSharedDataSSN = CreateMV("pasSDS", sizeof("pasSDS"), MAXCLERKS);

	applicationCompletion = CreateMV("appComp", sizeof("appComp"), MAXCLERKS + MAXSENATORS);
	pictureCompletion = CreateMV("picComp", sizeof("picComp"), MAXCLERKS + MAXSENATORS);
	passportCompletion = CreateMV("pasComp", sizeof("pasComp"), MAXCLERKS + MAXSENATORS);
	passportPunishment = CreateMV("pasPun", sizeof("pasPun"), MAXCLERKS + MAXSENATORS);
	cashierSharedDataSSN = CreateMV("casSDSSN", sizeof("casSDSSN"), MAXCLERKS + MAXSENATORS);
	cashierRejection = CreateMV("casReject", sizeof("casReject"), MAXCLERKS + MAXSENATORS);
	doneCompletely = CreateMV("doneComp", sizeof("doneComp"), MAXCLERKS + MAXSENATORS);

	SSNCount = CreateMV("SSNCount", sizeof("SSNCount"), 1);
	ApplicationMyLine = CreateMV("AppMyLine", sizeof("AppMyLine"), 1);
	PictureMyLine = CreateMV("PicMyLine", sizeof("PicMyLine"), 1);
	PassportMyLine = CreateMV("PasMyLine", sizeof("PasMyLine"), 1);
	CashierMyLine = CreateMV("CasMyLine", sizeof("CasMyLine"), 1);

	customersPresentCount = CreateMV("cusPresCount", sizeof("cusPresCount"), 1);
	senatorPresentCount = CreateMV("senPresCount", sizeof("senPresCount"), 1);
	checkedOutCount = CreateMV("checkOutC", sizeof("checkOutC"), 1);
	senatorLineCount = CreateMV("senLineC", sizeof("senLineC"), 1);
	passportOfficeOutsideLineCount = CreateMV("outsideLC", sizeof("outsideLC"), 1);
	senatorSafeToEnter = CreateMV("senSafeTE", sizeof("senSafeTE"), 1);
	senatorPresentWaitOutSide = CreateMV("senPresWOS", sizeof("senPresWOS"), 1);

	THEEND = CreateMV("THEEND", sizeof("THEEND"), 1);

  /*Init clerkStates, lineCounts*/
	PrintString("Setup bout to create MAXCLERKS loop.\n", sizeof("Setup bout to create MAXCLERKS loop.\n"));
  for(i=0; i<MAXCLERKS; i++){
    applicationClerkLock[i] = CreateLock("appLock" + i, sizeof("appLock" + i));
    applicationClerkLineCV[i] = CreateCondition("appLineCV", sizeof("appLineCV"));
    applicationClerkBribeLineCV[i] = CreateCondition("appBribeCV", sizeof("appBribeCV"));
    applicationClerkCV[i] = CreateCondition("appCV", sizeof("appCV"));

    pictureClerkLock[i] = CreateLock("picLock" + i, sizeof("picLock" + i));
    pictureClerkLineCV[i] = CreateCondition("picLineCV", sizeof("picLineCV"));
    pictureClerkBribeLineCV[i] = CreateCondition("picBribeCV", sizeof("picBribeCV"));
    pictureClerkCV[i] = CreateCondition("picCV", sizeof("picCV"));

    passportClerkLock[i] = CreateLock("pasLock" + i, sizeof("pasLock" + i));
    passportClerkLineCV[i] = CreateCondition("pasLineCV" + i, sizeof("pasLineCV" + i));
    passportClerkBribeLineCV[i] = CreateCondition("pasBribeCV" + i, sizeof("pasBribeCV" + i));
    passportClerkCV[i] = CreateCondition("pasCV" + i, sizeof("pasCV" + i));

    cashierLock[i] = CreateLock("casLock" + i, sizeof("casLock" + i));
    cashierLineCV[i] = CreateCondition("casLineCV" + i, sizeof("casLineCV" + i));
    cashierBribeLineCV[i] = CreateCondition("casBribeCV" + i, sizeof("casBribeCV" + i));
    cashierCV[i] = CreateCondition("casCV" + i, sizeof("casCV" + i));
  }

}

/*Called by each entity to destroy their resources.*/
void passportDestroy(){
  int i;



  DestroyLock(applicationClerkLineLock);
  DestroyLock(pictureClerkLineLock);
  DestroyLock(passportClerkLineLock);
  DestroyLock(cashierLineLock);
  DestroyLock(managerLock);
  DestroyLock(printLock);
  DestroyLock(SSNLock);
  DestroyLock(ApplicationMyLineLock);
  DestroyLock(PictureMyLineLock);
  DestroyLock(PassportMyLineLock);
  DestroyLock(CashierMyLineLock);

  DestroyCondition(applicationClerkBreakCV);
  DestroyCondition(pictureClerkBreakCV);
  DestroyCondition(passportClerkBreakCV);
  DestroyCondition(cashierBreakCV);
  DestroyCondition(passportOfficeOutsideLineCV);
  DestroyCondition(senatorLineCV);

  /*Monitor Variables...*/
  DestroyMV(applicationClerkState);
  DestroyMV(pictureClerkState);
  DestroyMV(passportClerkState);
  DestroyMV(cashierState);

  DestroyMV(applicationClerkLineCount);
  DestroyMV(applicationClerkBribeLineCount);
  DestroyMV(pictureClerkLineCount);
  DestroyMV(pictureClerkBribeLineCount);
  DestroyMV(passportClerkLineCount);
  DestroyMV(passportClerkBribeLineCount);
  DestroyMV(cashierLineCount);
  DestroyMV(cashierBribeLineCount);

  DestroyMV(applicationClerkSharedData);
  DestroyMV(pictureClerkSharedDataSSN);
  DestroyMV(pictureClerkSharedDataPicture);
  DestroyMV(passportClerkSharedDataSSN);

  DestroyMV(applicationCompletion);
  DestroyMV(pictureCompletion);
  DestroyMV(passportCompletion);
  DestroyMV(passportPunishment);
  DestroyMV(cashierSharedDataSSN);
  DestroyMV(cashierRejection);
  DestroyMV(doneCompletely);

  DestroyMV(SSNCount);
  DestroyMV(ApplicationMyLine);
  DestroyMV(PictureMyLine);
  DestroyMV(PassportMyLine);
  DestroyMV(CashierMyLine);

  DestroyMV(customersPresentCount);
  DestroyMV(senatorPresentCount);
  DestroyMV(checkedOutCount);
  DestroyMV(senatorLineCount);
  DestroyMV(passportOfficeOutsideLineCount);
  DestroyMV(senatorSafeToEnter);
  DestroyMV(senatorPresentWaitOutSide);

  DestroyMV(THEEND);

  /*Init clerkStates, lineCounts*/
  for(i=0; i<MAXCLERKS; i++){
    DestroyLock(applicationClerkLock[i]);
    DestroyCondition(applicationClerkLineCV[i]);
    DestroyCondition(applicationClerkBribeLineCV[i]);
    DestroyCondition(applicationClerkCV[i]);

    DestroyLock(pictureClerkLock[i]);
    DestroyCondition(pictureClerkLineCV[i]);
    DestroyCondition(pictureClerkBribeLineCV[i]);
    DestroyCondition(pictureClerkCV[i]);

    DestroyLock(passportClerkLock[i]);
    DestroyCondition(passportClerkLineCV[i]);
    DestroyCondition(passportClerkBribeLineCV[i]);
    DestroyCondition(passportClerkCV[i]);

    DestroyLock(cashierLock[i]);
    DestroyCondition(cashierLineCV[i]);
    DestroyCondition(cashierBribeLineCV[i]);
    DestroyCondition(cashierCV[i]);
  }
}

/*Should be called once to close the passport office.*/
void initialPassportDestroy(){

  while(!Get(THEEND, 0)){
    Yield();
  }


  Acquire(printLock);
  PrintString("Passport Office Closed.\n", sizeof("Passport Office Closed.\n"));
  Release(printLock);

  passportDestroy();
}

/*hould be called only once to initialize all MVs*/
void initialPassportSetup(){
  int i;

  passportSetup();

  Set(THEEND, 0, 0);
  Set(senatorPresentWaitOutSide, 0, 0);
  Set(senatorSafeToEnter, 0, 0);
  Set(SSNCount, 0, 0);
  Set(ApplicationMyLine, 0, 0);
  Set(PictureMyLine, 0, 0);
  Set(PassportMyLine, 0, 0);
  Set(CashierMyLine, 0, 0);

  for(i = 0; i < MAXCUSTOMERS + MAXSENATORS; i++){
    Set(applicationCompletion, i, 0);
    Set(pictureCompletion, i, 0);
    Set(passportCompletion, i, 0);
    Set(passportPunishment, i, 0);
    Set(cashierSharedDataSSN, i, 0);
    Set(cashierRejection, i, 0);
    Set(doneCompletely, i, 0);
  }

  for(i=0; i<MAXCLERKS; i++){
    Set(applicationClerkState, i, BUSY);
    Set(pictureClerkState, i, BUSY);
    Set(passportClerkState, i, BUSY);
    Set(cashierState, i, BUSY);

    Set(applicationClerkLineCount, i, 0);     
    Set(applicationClerkBribeLineCount, i, 0);    
    Set(pictureClerkLineCount, i, 0);     
    Set(pictureClerkBribeLineCount, i, 0);    
    Set(passportClerkLineCount, i, 0);      
    Set(passportClerkBribeLineCount, i, 0);   
    Set(cashierLineCount, i, 0);      
    Set(cashierBribeLineCount, i, 0);

    Set(applicationClerkSharedData, i, 0);
    Set(pictureClerkSharedDataSSN, i, 0);
    Set(pictureClerkSharedDataPicture, i, 0);
    Set(passportClerkSharedDataSSN, i, 0);
  }

  for(i = 0; i < CUSTOMERCOUNT; i++){
    Exec("../test/PCustomer", sizeof("../test/PCustomer"));
  }

  
  for(i = 0; i < SENATORCOUNT; i++){
    /*Exec("../test/PSenator", sizeof("../test/PSenator"));*/
  }


  for(i = 0; i < CLERKCOUNT; i++){
    /*Exec("../test/PAppClerk", sizeof("../test/PAppClerk"));*/
    /*Exec("../test/PPicClerk", sizeof("../test/PPicClerk"));
    Exec("../test/PPasClerk", sizeof("../test/PPasClerk"));
    Exec("../test/PCashier", sizeof("../test/PCashier"));*/
  }

  /*Exec("../test/PManager", sizeof("../test/PManager"));*/

}

