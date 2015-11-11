// nettest.cc 
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads 
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include <sstream>
#include <string>
#include "syscall.h"    //For the SC_syscall defines
#include "list.h"
#include "bitmap.h"
using namespace std;

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

void
MailTest(int farAddr)
{
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Hello there!";
    char *ack = "Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = farAddr;		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    success = postOffice->Send(outPktHdr, outMailHdr, ack); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}











/////////////
//  sendMail
// 
//  Sends the given mail message prints an error on failure.
///////////////////////////
void sendMail(char* msg, int pktHdr, int mailHdr){
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    outMailHdr.from = 0;

    outMailHdr.to = mailHdr;
    outPktHdr.to = pktHdr;
    outMailHdr.length = strlen(msg) + 1;

    bool success = postOffice->Send(outPktHdr, outMailHdr, msg); 

    if ( !success ) {
      printf("Failed to send message to machine %i mailbox %i with message %s !\n", pktHdr, mailHdr, msg);
    }

    DEBUG('n', "Sent mail to %i:%i message:\"%s\".\n", pktHdr, mailHdr, msg);
}



class ServerReplyMsg{
public:
    int pktHdr;
    int mailHdr;
    char* msg;
};


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////LOCKS//////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////
#define lockTableSize 200
BitMap serverLockTableBitMap(lockTableSize);

enum SERVERLOCKSTATE {FREE, BUSY};

class ServerLock{
public:
    SERVERLOCKSTATE state;
    string name;
    int ownerMachineID;
    int ownerMailboxNumber;
    List *q;
    bool isToBeDestroyed;
    int createLockCount;
    ServerLock(){
        q = new List();
        state = FREE;
        isToBeDestroyed = FALSE;
        createLockCount = 1;
    }
    ~ServerLock(){
        delete q;
    }
    bool isOwner(int machineID, int mailbox){
        return (ownerMachineID == machineID) && (ownerMailboxNumber == mailbox) && state == BUSY;
    }
};

vector<ServerLock*> serverLocks; //The table of locks


bool checkIfLockIDExists(int lockID){
    //Check if lock exists.
    if(lockID >= (int)serverLocks.size() || lockID < 0){
        printf("\t\tLockID %i does not exist.\n", lockID);
        return FALSE;
    }else if(serverLocks[lockID] == NULL){
        printf("\t\tLockID %i no longer exists.\n", lockID);
        return FALSE;
    }
    return TRUE;
}

////////////
// Trys to find a lock with the given name
// Creates one if it doesn't exist.
int getLockNamed(string name){
    int index = -1;

    //See if lock exists.
    for(unsigned int i = 0; i < serverLocks.size(); i++){
        if(serverLocks[i] != NULL){
            if(serverLocks[i]->name == name){
                index = (int)i;
                break;
            }
        }
    }

    if(index == -1){
        //Lock doesn't exist yet...need to create it.
        ServerLock* l = new ServerLock();
        l->name = name;

        index = serverLockTableBitMap.Find();

        if(index == -1){
            printf("\t\tMax Number of locks created. lockTableSize: %i\n", lockTableSize);
            return index;
        }
        //Add lock to the table
        if(index == (int)serverLocks.size()){
            serverLocks.push_back(l);
        }else if(index < (int)serverLocks.size()){
            serverLocks[index] = l;
        }else{ASSERT(FALSE);}
        printf("\t\tCreated Lock.\n");
    }else{
        //ock does exist ... increment create count
        serverLocks[index]->createLockCount++;
        printf("\t\tLock Already Exists.\n");
    }
    return index;
}

void checkLockAndDestroy(int lockID){
    ServerLock* l;

    if(!checkIfLockIDExists(lockID)){return;}
    
    l = serverLocks[lockID];

    if(l->isToBeDestroyed && l->state == FREE && l->createLockCount == 0){
        //Destroy the lock
        serverLocks[lockID] = NULL;
        delete l;
        serverLockTableBitMap.Clear(lockID);
        printf("\t\tDestroyed LockID: %i.\n", lockID);
    }else{
        //printf("\t\tNot ready to destroy lockID: %i\n", lockID);
    }
}


void serverAcquireLock(int lockID, int pktHdr, int mailHdr){
    bool status;
    char* msg;
    ServerLock* l;
    stringstream rs;

    //Check if lock exists.
    status = checkIfLockIDExists(lockID);

    rs << status;
    msg = (char*) rs.str().c_str();

    if(!status){
        sendMail(msg, pktHdr, mailHdr);
        return;
    }

    l = serverLocks[lockID];

    if(l->isOwner(pktHdr, mailHdr)){
        //We already own the lock...
        sendMail(msg, pktHdr, mailHdr);
        printf("\t\tAlready owned.\n");
    }else if(l->state == BUSY){
        //Lock busy add request to q
        ServerReplyMsg* r = new ServerReplyMsg();
        r->pktHdr = pktHdr;
        r->mailHdr = mailHdr;
        r->msg = msg;
        l->q->Append((void*)r);
        printf("\t\tAdded to lock queue.\n");
    }else if(l->state == FREE){
        l->state = BUSY;
        l->ownerMachineID = pktHdr;
        l->ownerMailboxNumber = mailHdr;
        //Send reply
        sendMail(msg, pktHdr, mailHdr);
        printf("\t\tLock Free. Sent acquire message.\n");
    }
}



void serverReleaseLock(int lockID, int pktHdr, int mailHdr){
    ServerLock* l;

    //Check if lock exists.
   if(!checkIfLockIDExists(lockID)){return;}

    l = serverLocks[lockID];


    if(! (l->isOwner(pktHdr, mailHdr)) ){
        //Only the lock owner can release the lock
        printf("\t\tOnly the lock owner can release the lock.\n");
        return;
    }else if(! (l->q->IsEmpty()) ){//Someone else waiting to acquire the lock
        ServerReplyMsg* r = (ServerReplyMsg*)(l->q->Remove());
        l->ownerMachineID = r->pktHdr;
        l->ownerMailboxNumber = r->mailHdr;
        sendMail(r->msg, r->pktHdr, r->mailHdr);
        delete r;
        printf("\t\tSent acquire confirmation to sleeping thread.\n");
    }else{//The lock is now free
        l->state = FREE;
        printf("\t\tLock is now Free.\n");
        checkLockAndDestroy(lockID);
    }
}


void serverDestroyLock(int lockID){
     ServerLock* l;

    if(!checkIfLockIDExists(lockID)){return;}

    l = serverLocks[lockID];

    //mark for deletion.
    l->isToBeDestroyed = TRUE;
    l->createLockCount--;
    checkLockAndDestroy(lockID);
}













//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////   //////// //////////// ///////////////////////////////////
////////////////   /////////// ////////// ////////////////////////////////////
//////////////   ////////////// //////// /////////////////////////////////////
/////////////  ///////////////// ////// //////////////////////////////////////
//////////////   //////////////// //// ///////////////////////////////////////
////////////////  //////////////// // ////////////////////////////////////////
/////////////////   /////////////// //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#define CVTableSize 200
BitMap serverCVTableBitMap(CVTableSize);

class ServerCV{
public:
    string name;
    List *q;
    int waitingLock;
    int createCVCount;
    ServerCV(){
        q = new List();
        waitingLock = NULL;
        createCVCount = 1;
    }
    ~ServerCV(){
        delete q;
    }
};

vector<ServerCV*> serverCVs; //The table of locks


////////////////////
bool checkIfCVIDExists(int CVID){
    //Check if lock exists.
    if(CVID >= (int)serverCVs.size() || CVID < 0){
        printf("\t\tCVID %i does not exist.\n", CVID);
        return FALSE;
    }else if(serverCVs[CVID] == NULL){
        printf("\t\tCVID %i no longer exists.\n", CVID);
        return FALSE;
    }
    return TRUE;
}


////////////////
void serverCreateCV(string name, int pktHdr, int mailHdr){
    int index = -1;

    //See if CV name exists
    for(unsigned int i = 0; i < serverCVs.size(); i++){
        if(serverCVs[i] != NULL){
            if(serverCVs[i]->name == name){
                index = (int)i;
                break;
            }
        }
    }

    if(index == -1){
        //CV doesn't exist
        ServerCV* c = new ServerCV();
        c->name = name;

        index = serverCVTableBitMap.Find();

        if(index == -1){
            printf("\t\tMax Number of locks created. lockTableSize: %i\n", lockTableSize);
        }else{
            if(index == (int)serverCVs.size()){
                serverCVs.push_back(c);
            }else if(index < (int)serverCVs.size()){
                serverCVs[index] = c;
            }else{ASSERT(FALSE);}
            printf("\t\tCreated CV\n");
        }
    }else{
        //CV exists
        serverCVs[index]->createCVCount++;
        printf("\t\tCV already Exists.\n");
    }

    stringstream rs;
    rs << index;

    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
}



bool checkCVAndDestroy(int CVID){
    ServerCV* c;

    if(!checkIfCVIDExists(CVID)){return false;}
    
    c = serverCVs[CVID];

    if(c->q->IsEmpty() && c->createCVCount == 0){
        //Destroy the CV
        serverCVs[CVID] = NULL;
        delete c;
        serverCVTableBitMap.Clear(CVID);
        printf("\t\tDestroyed CVID: %i.\n", CVID);
        return true;
    }else{
        return false;
    }
}

/////////////////
void serverDestroyCV(int CVID){

    if(!checkIfCVIDExists(CVID)){return;}

    serverCVs[CVID]->createCVCount--;
    if(!checkCVAndDestroy(CVID)){
        printf("\t\tNot ready to destroy CVID %i.\n", CVID);
    }
}


///////////////////
void serverWait(int CVID, int lockID, int pktHdr, int mailHdr){
    ServerCV* c;
    ServerLock* l;
    stringstream rs;

    if(!checkIfCVIDExists(CVID)){//Return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }else if(!checkIfLockIDExists(lockID)){//return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    c = serverCVs[CVID];
    l = serverLocks[lockID];

    if(!l->isOwner(pktHdr, mailHdr)){
        printf("\t\tMust be lock owner to wait.\n");
        //Return error....
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    if(c->waitingLock == NULL){
        c->waitingLock = lockID;
    }

    if(c->waitingLock != lockID){
        printf("CV lockID %i does not match lockID %i passed to Wait.\n",c->waitingLock, lockID);
        //return error
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    rs << TRUE;

    ServerReplyMsg* r = new ServerReplyMsg();

    r->pktHdr = pktHdr;
    r->mailHdr = mailHdr;
    r->msg = (char*)rs.str().c_str();

    c->q->Append((void*)r);

    //Release waiting lock...
    serverReleaseLock(lockID, pktHdr, mailHdr);
}



//////////////////////
void serverSignal(int CVID, int lockID, int pktHdr, int mailHdr){
    ServerCV* c;
    ServerLock* l;
    stringstream rs;

    if(!checkIfCVIDExists(CVID)){//Return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }else if(!checkIfLockIDExists(lockID)){//return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    c = serverCVs[CVID];
    l = serverLocks[lockID];

    if(!l->isOwner(pktHdr, mailHdr)){
        printf("\t\tMust be lock owner to signal.\n");
        //Return error....
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }


    if(c->waitingLock == NULL){ //if no thread waiting
        printf("\t\tNo waiting threads.\n");
        rs << TRUE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }
    if(c->waitingLock != lockID){
        printf("\t\tCV lockID %i != signal lockID %i\n", c->waitingLock, lockID);
        //Return error....
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }


    //Wakeup 1 waiting thread
    ServerReplyMsg* r = (ServerReplyMsg *)c->q->Remove();

    if(c->q->IsEmpty()){
        c->waitingLock = NULL;
    }

    //We need to have the 'woken up' thread acquire the lock
    serverAcquireLock(lockID, r->pktHdr, r->mailHdr);

    printf("Signalled %i:%i.\n",r->pktHdr, r->mailHdr);
    delete r;
    
    rs << TRUE;
    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
    return;
}



//////////////////////
void serverBroadcast(){

}
















































//////////////////////////////////////////////
/// Server()
///
/// The nachos network server
///
/// Handles all network related syscalls
///
/// CreateLock, Acquire, Release, DestroyLock
/// CreateCV, Wait, Signal, Broadcast, DestroyCV
/// CreateMV, SetMV, GetMV, DestroyMV
////////////////////////////////////////////////
/*SC_CreateLock
SC_Acquire
SC_Release
SC_DestroyLock
SC_CreateCondition
SC_Wait
SC_Signal
SC_Broadcast
SC_DestroyCondition*/

void Server(){
    printf("Starting nachos network server.\n");
    char buffer[MaxMailSize];
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    bool success;

    outMailHdr.from = 0;

    while(TRUE){
        //Revieve a msg
        //Parse msg
        //Process msg
        //Reply(maybe)
        stringstream ss;

        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
        ss << buffer;
        printf("\nServer: Received Message from %d: %s.\n", inPktHdr.from, ss.str().c_str());
        fflush(stdout);


        int which;
        ss >> which;



        // SC_CreateLock
        if(which == SC_CreateLock){
            printf("\tCreateLock\n");
            string lockName;
            ss >> lockName;
            
            int lockID = getLockNamed(lockName);//Find or create the lock
            
            printf("\t\tCreateLock named %s lockID %i.\n", lockName.c_str(), lockID);
            
            stringstream rs;
            rs << (lockID != -1);//status
            rs << " ";
            rs << lockID;

            char *msg = (char*) rs.str().c_str();

            outPktHdr.to = inPktHdr.from;
            outMailHdr.to = inMailHdr.from;
            outMailHdr.length = strlen(msg) + 1;
            success = postOffice->Send(outPktHdr, outMailHdr, msg);
            if(!success){
                printf("Failed to reply to machine %d.\n", outPktHdr.to);
            }
        


        }//SC_Acquire
        else if(which == SC_Acquire){
            printf("\tAcquire:\n");
            int lockID;
            ss >> lockID;

            serverAcquireLock(lockID, inPktHdr.from, inMailHdr.from);
        

        }
        else if(which == SC_Release){
            printf("\tRelease:\n");
            int lockID;
            ss >> lockID;

            serverReleaseLock(lockID, inPktHdr.from, inMailHdr.from);
        

        }
        else if (which == SC_DestroyLock){
            printf("\tDestroyLock:\n");
            int lockID;
            ss >> lockID;

            serverDestroyLock(lockID);


        }



        printf("\n");
        continue;
        

        switch (which){
            
            case SC_Acquire:

            break;
            case SC_Release:

            break;
            case SC_DestroyLock:

            break;
            case SC_CreateCondition:

            break;
            case SC_Wait:

            break;
            default:
                printf("Unrecognized request.\n");
            break;
        }




        
        //Send reply
        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.length = 1;
        success = postOffice->Send(outPktHdr, outMailHdr, "");

        if(!success){
            printf("Failed to reply to machine %d\n", outPktHdr.to);
        }
    }//End whileLOOP
}//End Server()
