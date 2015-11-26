/*setup.h*/

/* Contains all code for the passport office setup. Includes locks, CVs, and MVs.*/

#include "syscall.h"



#define CLERKCOUNT  3
#define CUSTOMERCOUNT 10
#define SENATORCOUNT  3

#define MAXCLERKS 5
#define MAXCUSTOMERS 50
#define MAXSENATORS 10


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

/* Utility function to pick the shortest line
* Customer has already chosen type of line to get in just needs to pick which line
* Assumptions: The caller has a lock for the given MVs
*Parameters: 
  *lineCount: a vector of the lineCount
  *clerkState: a vector of the clerkState*/
int pickShortestLine(int* pickShortestlineCount, int* pickShortestclerkState){
  int myLine = -1;
  int lineSize = 1000;
  int i;
  for(i=0; i < CLERKCOUNT; i++){
    /*If lineCount < lineSize and clerk is not on break*/
    if(pickShortestlineCount[i] < lineSize && pickShortestclerkState[i] != ONBREAK ){
      myLine = i;
      lineSize = pickShortestlineCount[i];
    }
  }
  return myLine;  /*This is the shortest line*/
}/*End pickShortestLine*/

/*******************
 End Utility Functions
********************/





//Should be called only once to initialize all MVs
void initialSetup(){
	Set(THEEND, 0, 0);
  	Set(senatorPresentWaitOutSide, 0, 0);
  	Set(senatorSafeToEnter, 0, 0);
  	Set(SSNCount, 0, 0);
  	Set(ApplicationMyLine, 0, 0);
  	Set(PictureMyLine, 0, 0);
  	Set(PassportMyLine, 0, 0);
  	Set(CashierMyLine, 0, 0);
}





/*Called for every clerk to Create the locks and whatnot...*/
void setup(){
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

	applicationCompletion = CreateMV("appComp", sizeof("appComp"), MAXCLERKS + MAXSENATORS);/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by passportCerkto verify that application has been completed */
	pictureCompletion = CreateMV("picComp", sizeof("picComp"), MAXCLERKS + MAXSENATORS);;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by passportClerk to verify that picture has beeen completed */
	passportCompletion = CreateMV("pasComp", sizeof("pasComp"), MAXCLERKS + MAXSENATORS);;/*[MAXCUSTOMERS + MAXSENATORS]; /* // Used by cashier to verify that the passport is complete */
	passportPunishment = CreateMV("pasPun", sizeof("pasPun"), MAXCLERKS + MAXSENATORS);;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by passportClerk to punish bad people. */
	cashierSharedDataSSN = CreateMV("casSDSSN", sizeof("casSDSSN"), MAXCLERKS + MAXSENATORS);;/*[MAXCUSTOMERS + MAXSENATORS]; /* //This can be used by the customer to pass SSN */
	cashierRejection = CreateMV("casReject", sizeof("casReject"), MAXCLERKS + MAXSENATORS);;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by the cashier to reject customers. */
	doneCompletely = CreateMV("doneComp", sizeof("doneComp"), MAXCLERKS + MAXSENATORS);;/*[MAXCUSTOMERS + MAXSENATORS]; /* //Used by customer to tell when done. */

	SSNCount = CreateMV("SSNCount", sizeof("SSNCount"), 1);
	ApplicationMyLine = CreateMV("AppMyLine", sizeof("AppMyLine"), 1);
	PictureMyLine = CreateMV("PicMyLine", sizeof("PicMyLine"), 1);
	PassportMyLine = CreateMV("PasMyLine", sizeof("PasMyLine"), 1);
	CashierMyLine = CreateMV("CasMyLine", sizeof("CasMyLine"), 1);

	customersPresentCount = CreateMV("CasMyLine", sizeof("CasMyLine"), 1);
	senatorPresentCount = CreateMV("senPresCount", sizeof("senPresCount"), 1);
	checkedOutCount = CreateMV("checkOutC", sizeof("checkOutC"), 1);
	senatorLineCount = CreateMV("senLineC", sizeof("senLineC"), 1);
	passportOfficeOutsideLineCount = CreateMV("outsideLC", sizeof("outsideLC"), 1);
	senatorSafeToEnter = CreateMV("senSafeTE", sizeof("senSafeTE"), 1);
	senatorPresentWaitOutSide = CreateMV("senPresWOS", sizeof("senPresWOS"), 1);

	THEEND = CreateMV("THEEND", sizeof("THEEND"), 1);




  /*Init clerkStates, lineCounts*/
  for(i=0; i<MAXCLERKS; i++){
    applicationClerkState[i] = BUSY;
    pictureClerkState[i] = BUSY;
    passportClerkState[i] = BUSY;
    cashierState[i] = BUSY;

    applicationClerkLineCount[i] = 0;     
    applicationClerkBribeLineCount[i] = 0;    
    pictureClerkLineCount[i] = 0;     
    pictureClerkBribeLineCount[i] = 0;    
    passportClerkLineCount[i] = 0;      
    passportClerkBribeLineCount[i] = 0;   
    cashierLineCount[i] = 0;      
    cashierBribeLineCount[i] = 0;

    applicationClerkSharedData[i] = 0;
    pictureClerkSharedDataSSN[i] = 0;
    pictureClerkSharedDataPicture[i] = 0;
    passportClerkSharedDataSSN[i] = 0;

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

  for(i = 0; i < MAXCUSTOMERS + MAXSENATORS; i++){
    applicationCompletion[i] = 0;
    pictureCompletion[i] = 0;
    passportCompletion[i] = 0;
    passportPunishment[i] = 0;
    cashierSharedDataSSN[i] = 0;
    cashierRejection[i] = 0;
    doneCompletely[i] = 0;
  }


  for(i = 0; i < CUSTOMERCOUNT; i++){
    Fork(Customer);
  }

  
  for(i = 0; i < SENATORCOUNT; i++){
    Fork(Senator);
  }


  for(i = 0; i < CLERKCOUNT; i++){
    Fork(ApplicationClerk);
    Fork(PictureClerk);
    Fork(PassportClerk);
    Fork(Cashier);
  }

  Fork(Manager);

  while(!THEEND){
    Yield();
  }


  Acquire(printLock);
  PrintString("Passport Office Closed.\n", sizeof("Passport Office Closed.\n"));
  Release(printLock);

  Exit(0);
}