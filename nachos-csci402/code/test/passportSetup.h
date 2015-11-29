/*setup.h*/

/* Contains all code for the passport office setup. Includes locks, CVs, and MVs.*/

#include "syscall.h"



/*#define CLERKCOUNT  1*/
#define CUSTOMERCOUNT 2
#define SENATORCOUNT  1

#define MAXCLERKS 5
#define MAXCUSTOMERS 100
#define MAXSENATORS 50


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
int STARTPASSPORT;
int STOPPASS;
int CLERKCOUNT;

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
	char temp[15];

	/*PrintString("Start passportSetup...\n", sizeof("Start passportSetup...\n"));*/


  applicationClerkLineLock = CreateLock("appLineLock", sizeof("appLineLock"));
  /*PrintString("Getting stuck here...?\n", sizeof("Getting stuck here...?\n"));*/
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

	applicationCompletion = CreateMV("appComp", sizeof("appComp"), (MAXCUSTOMERS + MAXSENATORS));
	pictureCompletion = CreateMV("picComp", sizeof("picComp"), (MAXCUSTOMERS + MAXSENATORS));
	passportCompletion = CreateMV("pasComp", sizeof("pasComp"), (MAXCUSTOMERS + MAXSENATORS));
	passportPunishment = CreateMV("pasPun", sizeof("pasPun"), (MAXCUSTOMERS + MAXSENATORS));
	cashierSharedDataSSN = CreateMV("casSDSSN", sizeof("casSDSSN"), (MAXCUSTOMERS + MAXSENATORS));
	cashierRejection = CreateMV("casReject", sizeof("casReject"), (MAXCUSTOMERS + MAXSENATORS));
	doneCompletely = CreateMV("doneComp", sizeof("doneComp"), (MAXCUSTOMERS + MAXSENATORS));

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
  STARTPASSPORT = CreateMV("STARTPASS", sizeof("STARTPASS"), 1 );
  STOPPASS = CreateMV("STOPPASS", sizeof("STOPPASS"), 1);
  if(Get(STARTPASSPORT, 0))
    CLERKCOUNT = 5;
  else 
    CLERKCOUNT = 1;
  /*Init clerkStates, lineCounts*/
	/*PrintString("Setup bout to create MAXCLERKS loop.\n", sizeof("Setup bout to create MAXCLERKS loop.\n"));*/
 /* for(i=0; i<MAXCLERKS; i++){

  	temp = "appLock";
  	StringConcatInt(temp, sizeof(temp), i);

    applicationClerkLock[i] = CreateLock(temp, sizeof(temp));
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
  }*/

   	applicationClerkLock[0] = CreateLock("appLock1", sizeof("appLock1"));
    applicationClerkLineCV[0] = CreateCondition("appLineCV1", sizeof("appLineCV1"));
    applicationClerkBribeLineCV[0] = CreateCondition("appBribeCV1", sizeof("appBribeCV1"));
    applicationClerkCV[0] = CreateCondition("appCV1", sizeof("appCV1"));

    applicationClerkLock[1] = CreateLock("appLock2", sizeof("appLock2"));
    applicationClerkLineCV[1] = CreateCondition("appLineCV2", sizeof("appLineCV2"));
    applicationClerkBribeLineCV[1] = CreateCondition("appBribeCV2", sizeof("appBribeCV2"));
    applicationClerkCV[1] = CreateCondition("appCV2", sizeof("appCV2"));

    applicationClerkLock[2] = CreateLock("appLock3", sizeof("appLock3"));
    applicationClerkLineCV[2] = CreateCondition("appLineCV3", sizeof("appLineCV3"));
    applicationClerkBribeLineCV[2] = CreateCondition("appBribeCV3", sizeof("appBribeCV3"));
    applicationClerkCV[2] = CreateCondition("appCV3", sizeof("appCV3"));

    applicationClerkLock[3] = CreateLock("appLock4", sizeof("appLock4"));
    applicationClerkLineCV[3] = CreateCondition("appLineCV4", sizeof("appLineCV4"));
    applicationClerkBribeLineCV[3] = CreateCondition("appBribeCV4", sizeof("appBribeCV4"));
    applicationClerkCV[3] = CreateCondition("appCV4", sizeof("appCV4"));

    applicationClerkLock[4] = CreateLock("appLock5", sizeof("appLock5"));
    applicationClerkLineCV[4] = CreateCondition("appLineCV5", sizeof("appLineCV5"));
    applicationClerkBribeLineCV[4] = CreateCondition("appBribeCV5", sizeof("appBribeCV5"));
    applicationClerkCV[4] = CreateCondition("appCV5", sizeof("appCV5"));


    pictureClerkLock[0] = CreateLock("picLock1", sizeof("picLock1"));
    pictureClerkLineCV[0] = CreateCondition("picLineCV1", sizeof("picLineCV1"));
    pictureClerkBribeLineCV[0] = CreateCondition("picBribeCV1", sizeof("picBribeCV1"));
    pictureClerkCV[0] = CreateCondition("picCV1", sizeof("picCV1"));

    pictureClerkLock[1] = CreateLock("picLock2", sizeof("picLock2"));
    pictureClerkLineCV[1] = CreateCondition("picLineCV2", sizeof("picLineCV2"));
    pictureClerkBribeLineCV[1] = CreateCondition("picBribeCV2", sizeof("picBribeCV2"));
    pictureClerkCV[1] = CreateCondition("picCV2", sizeof("picCV2"));

    pictureClerkLock[2] = CreateLock("picLock3", sizeof("picLock3"));
    pictureClerkLineCV[2] = CreateCondition("picLineCV3", sizeof("picLineCV3"));
    pictureClerkBribeLineCV[2] = CreateCondition("picBribeCV3", sizeof("picBribeCV3"));
    pictureClerkCV[2] = CreateCondition("picCV3", sizeof("picCV3"));

    pictureClerkLock[3] = CreateLock("picLock4", sizeof("picLock4"));
    pictureClerkLineCV[3] = CreateCondition("picLineCV4", sizeof("picLineCV4"));
    pictureClerkBribeLineCV[3] = CreateCondition("picBribeCV4", sizeof("picBribeCV4"));
    pictureClerkCV[3] = CreateCondition("picCV4", sizeof("picCV4"));

    pictureClerkLock[4] = CreateLock("picLock5", sizeof("picLock5"));
    pictureClerkLineCV[4] = CreateCondition("picLineCV5", sizeof("picLineCV5"));
    pictureClerkBribeLineCV[4] = CreateCondition("picBribeCV5", sizeof("picBribeCV5"));
    pictureClerkCV[4] = CreateCondition("picCV5", sizeof("picCV5"));


    passportClerkLock[0] = CreateLock("pasLock1", sizeof("pasLock1"));
    passportClerkLineCV[0] = CreateCondition("pasLineCV1", sizeof("pasLineCV1"));
    passportClerkBribeLineCV[0] = CreateCondition("pasBribeCV1", sizeof("pasBribeCV1"));
    passportClerkCV[0] = CreateCondition("pasCV1", sizeof("pasCV1"));

    passportClerkLock[1] = CreateLock("pasLock2", sizeof("pasLock2"));
    passportClerkLineCV[1] = CreateCondition("pasLineCV2", sizeof("pasLineCV2"));
    passportClerkBribeLineCV[1] = CreateCondition("pasBribeCV2", sizeof("pasBribeCV2"));
    passportClerkCV[1] = CreateCondition("pasCV2", sizeof("pasCV2"));

    passportClerkLock[2] = CreateLock("pasLock3", sizeof("pasLock3"));
    passportClerkLineCV[2] = CreateCondition("pasLineCV3", sizeof("pasLineCV3"));
    passportClerkBribeLineCV[2] = CreateCondition("pasBribeCV3", sizeof("pasBribeCV3"));
    passportClerkCV[2] = CreateCondition("pasCV3", sizeof("pasCV3"));

    passportClerkLock[3] = CreateLock("pasLock4", sizeof("pasLock4"));
    passportClerkLineCV[3] = CreateCondition("pasLineCV4", sizeof("pasLineCV4"));
    passportClerkBribeLineCV[3] = CreateCondition("pasBribeCV4", sizeof("pasBribeCV4"));
    passportClerkCV[3] = CreateCondition("pasCV4", sizeof("pasCV4"));

    passportClerkLock[4] = CreateLock("pasLock5", sizeof("pasLock5"));
    passportClerkLineCV[4] = CreateCondition("pasLineCV5", sizeof("pasLineCV5"));
    passportClerkBribeLineCV[4] = CreateCondition("pasBribeCV5", sizeof("pasBribeCV5"));
    passportClerkCV[4] = CreateCondition("pasCV5", sizeof("pasCV5"));


    cashierLock[0] = CreateLock("casLock1", sizeof("casLock1"));
    cashierLineCV[0] = CreateCondition("casLineCV1", sizeof("casLineCV1"));
    cashierBribeLineCV[0] = CreateCondition("casBribeCV1", sizeof("casBribeCV1"));
    cashierCV[0] = CreateCondition("casCV1", sizeof("casCV1"));

    cashierLock[1] = CreateLock("casLock2", sizeof("casLock2"));
    cashierLineCV[1] = CreateCondition("casLineCV2", sizeof("casLineCV2"));
    cashierBribeLineCV[1] = CreateCondition("casBribeCV2", sizeof("casBribeCV2"));
    cashierCV[1] = CreateCondition("casCV2", sizeof("casCV2"));

    cashierLock[2] = CreateLock("casLock3", sizeof("casLock3"));
    cashierLineCV[2] = CreateCondition("casLineCV3", sizeof("casLineCV3"));
    cashierBribeLineCV[2] = CreateCondition("casBribeCV3", sizeof("casBribeCV3"));
    cashierCV[2] = CreateCondition("casCV3", sizeof("casCV3"));

    cashierLock[3] = CreateLock("casLock4", sizeof("casLock4"));
    cashierLineCV[3] = CreateCondition("casLineCV4", sizeof("casLineCV4"));
    cashierBribeLineCV[3] = CreateCondition("casBribeCV4", sizeof("casBribeCV4"));
    cashierCV[3] = CreateCondition("casCV4", sizeof("casCV4"));

    cashierLock[4] = CreateLock("casLock5", sizeof("casLock5"));
    cashierLineCV[4] = CreateCondition("casLineCV5", sizeof("casLineCV5"));
    cashierBribeLineCV[4] = CreateCondition("casBribeCV5", sizeof("casBribeCV5"));
    cashierCV[4] = CreateCondition("casCV5", sizeof("casCV5"));

  /*PrintString("Setup done...\n", sizeof("Setup done...\n"));*/
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
  DestroyMV(STARTPASSPORT);
  DestroyMV(STOPPASS);

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
	int i;

  if(Get(STARTPASSPORT, 0) == 0){
    while(!Get(THEEND, 0)){
      for(i = 0; i < 900;i++){
      	Yield();
      }
    }
  }else{
    while(!Get(STOPPASS, 0)){
      for(i = 0; i < 900;i++){
        Yield();
      }
    }
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
  CLERKCOUNT = 1;
  Set(THEEND, 0, 0);
  Set(STARTPASSPORT,0, 0);
  Set(STOPPASS, 0 , 0);

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

  /*PrintString("Initial Setup Done. Execing entities...\n", sizeof("Initial Setup Done. Execing entities...\n"));*/

  for(i = 0; i < CUSTOMERCOUNT; i++){
    Exec("../test/PCustomer", sizeof("../test/PCustomer"));
  }

  
  for(i = 0; i < SENATORCOUNT; i++){
 	Exec("../test/PSenator", sizeof("../test/PSenator"));
  }


  for(i = 0; i < CLERKCOUNT; i++){
    Exec("../test/PAppClerk", sizeof("../test/PAppClerk"));
    Exec("../test/PPicClerk", sizeof("../test/PPicClerk"));
    Exec("../test/PPasClerk", sizeof("../test/PPasClerk"));
    Exec("../test/PCashier", sizeof("../test/PCashier"));
  }

  Exec("../test/PManager", sizeof("../test/PManager"));

}

void startPass(){
  int i;

  passportSetup();
  CLERKCOUNT = 5;
  Set(THEEND, 0, 0);
  Set(STARTPASSPORT,0, 1);
  Set(STOPPASS, 0 , 0);

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

  Exec("../test/PManager", sizeof("../test/PManager"));
  
  for(i = 0; i < 10000; i++){
    Yield();
  }
  passportDestroy();
}

void stopPass(){
  passportSetup();

  if(Get(STARTPASSPORT, 0) == 0){
    Acquire(printLock);
    PrintString("StopPass should only be called after startPass.\n", sizeof("StopPass should only be called after startPass.\n"));
    PrintString("\tThis is probably an error.\n", sizeof("\tThis is probably an error.\n"));
    Release(printLock);
  }

  Set(STOPPASS, 0, 1);

  Acquire(printLock);
  PrintString("Passport Office Closed.\n", sizeof("Passport Office Closed.\n"));
  Release(printLock);

  passportDestroy();
}




void startCustomers(int count){
  int i;
  for(i = 0; i < count; i++){
     Exec("../test/PCustomer", sizeof("../test/PCustomer"));
  }
}

void startSenators(int count){
  int i;
  for(i = 0; i < count; i++){
    Exec("../test/PSenator", sizeof("../test/PSenator"));
  }
}

void startAppClerks(int count){
  int i;
  for(i = 0; i < count; i++){
    Exec("../test/PAppClerk", sizeof("../test/PAppClerk"));
  }
}

void startPicClerks(int count){
  int i;
  for(i = 0; i < count; i++){
    Exec("../test/PPicClerk", sizeof("../test/PPicClerk"));
  }
}

void startPasClerks(int count){
  int i;
  for(i = 0; i < count; i++){
    Exec("../test/PPasClerk", sizeof("../test/PPasClerk"));
  }
}

void startCashiers(int count){
  int i;
  for(i = 0; i < count; i++){
    Exec("../test/PCashier", sizeof("../test/PCashier"));
  }
}

