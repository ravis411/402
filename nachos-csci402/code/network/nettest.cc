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







void serverDoCreateLock(string name, int pktHdr, int mailHdr);  //forward declaration














/////////////
//  sendMail
// 
//  Sends the given mail message prints an error on failure.
///////////////////////////
void sendMail(char* msg, int pktHdr, int mailHdr){
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    outMailHdr.from = currentThread->getThreadID();

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










//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
///////////////////////////////////////////////////////
//Pending Requests....
/////////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
#define SERVERREQUEST 50
int pendingRequestCount = 0;

class PendingRequest{
public:
    int pktHdr;
    int mailHdr;
    int type;
    string name;
    int lockID;
    int CVID;
    int MVID;
    int MVIndex;
    int MVValue;
    int sentCount;
    int noCount;
    int requestID;

    PendingRequest(){
        pktHdr = -1;
        mailHdr = -1;
        type = -1;
        lockID = -1;
        CVID = -1;
        MVID = -1;
        MVIndex = -1;
        MVValue = 0;
        sentCount = -1;
        noCount = 0;
        requestID = pendingRequestCount++;
    }


    string toString(){
        stringstream s;
        s << "\n\tPkthdr: " << pktHdr;
        s << "\n\tMailHdr: "<< mailHdr;
        s<< "\n\tType: "<< type;
        s << "\n\tName: "<< name;
        s << "\n\tLockID: "<< lockID;
        s << "\n\tCVID: "<< CVID;
        s<< "\n\tMVID: "<< MVID;
        s << "\n\tMVIndex: "<< MVIndex;
        s << "\n\tMVValue: "<< MVValue;
        s << "\n\tsentCount: "<< sentCount;
        s << "\n\tnoCount: "<< noCount;
        return s.str();
    }
};

vector<PendingRequest*> pendingRequests;

void deletePendingRequest(int index){
    PendingRequest* p = pendingRequests[index];
    delete p;
    pendingRequests.erase(pendingRequests.begin() + index);
}

void deletePendingRequestPointer(PendingRequest* p){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        if(pendingRequests[i] == p){
            deletePendingRequest((int)i);
            return;
        }
    }
}


bool sendPendingRequest(PendingRequest* p){
    stringstream ss;
    PacketHeader outPktHdr;
    MailHeader outMailHdr;
    outMailHdr.from = currentThread->getThreadID();
    outMailHdr.to = 0;
    
    //type // request/Nreply // pkthdr // mailhdr
    ss << p->type << " " << true << " " << p->requestID << " " << p->pktHdr << " " << p->mailHdr << " ";


    if(p->type == SC_CreateLock){
        ss << p->name;
    }else if(p->type == SC_Acquire){
        ss << p->lockID;
    }else if(p->type == SC_Release){
        ss << p->lockID;
    }else if(p->type == SC_DestroyLock){
        ss << p->lockID;
    }else if(p->type == SC_CreateCondition){
        ss << p->name;
    }else if(p->type == SC_Wait){
        ss << p->CVID << " " << p->lockID;
    }else if(p->type == SC_Signal){
        ss << p->CVID << " " << p->lockID;
    }else if(p->type == SC_Broadcast){
        ss << p->CVID << " " << p->lockID;
    }else if(p->type == SC_DestroyCondition){
        ss << p->CVID;
    }else if(p->type == SC_CreateMV){
        ss << p->MVIndex << " " << p->name;
    }else if(p->type == SC_Set){
        ss << p->MVID << " " << p->MVIndex << " " << p->MVValue;
    } else if(p->type == SC_Get){
        ss << p->MVID << " " << p->MVIndex;
    }else if(p->type == SC_DestroyMV){
        ss << p->MVID;
    }

    string msg(ss.str());
    outMailHdr.length = strlen(msg.c_str()) + 1;
    p->sentCount = 0;
    for(int i = 0; i < 5; i++){
        
        if(i == postOffice->getNetworkAddress()){
            //Don't send the request to ourself...
            continue;
        }
        outPktHdr.to = i;

        if( postOffice->Send(outPktHdr, outMailHdr, (char*)msg.c_str()) ){
            (p->sentCount)++;
        }

    }

    if(p->sentCount == 0){
        //No Servers awake to handle the request...
        printf("NO SERVERS AWAKE TO HANDLE THE REQUEST!\n");
        return false;
    }else{
        printf("\t\tRequest sent to %i servers.\n", p->sentCount);
        if(false)printf("%s\n", p->toString().c_str());
        return true;
    }


}



//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingCreateLockRequest(int pkthdr, int mailHdr, int requestID, string name){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_CreateLock){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->name == name){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingCreateLockRequestNamed(string name){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->type == SC_CreateLock){
            if(p->name == name){
                return (int)i;
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingAcquireLockRequest(int pkthdr, int mailHdr, int requestID, int lockID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Acquire){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->lockID == lockID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingReleaseLockRequest(int pkthdr, int mailHdr, int requestID, int lockID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Release){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->lockID == lockID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingDestroyLockRequest(int pkthdr, int mailHdr, int requestID, int lockID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_DestroyLock){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->lockID == lockID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingCreateMVRequest(int pkthdr, int mailHdr, int requestID, string name){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_CreateMV){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->name == name){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingCreateMVRequestNamed(string name){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->type == SC_CreateMV){
            if(p->name == name){
                return (int)i;
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingSetMVRequest(int pkthdr, int mailHdr, int requestID, int MVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Set){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->MVID == MVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingGetMVRequest(int pkthdr, int mailHdr, int requestID, int MVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Get){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->MVID == MVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingDestroyMVRequest(int pkthdr, int mailHdr, int requestID, int MVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_DestroyMV){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->MVID == MVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingCreateCVRequest(int pkthdr, int mailHdr, int requestID, string name){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_CreateCondition){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->name == name){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingCreateCVRequestNamed(string name){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->type == SC_CreateCondition){
            if(p->name == name){
                return (int)i;
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingWaitRequest(int pkthdr, int mailHdr, int requestID, int CVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Wait){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->CVID == CVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingSignalRequest(int pkthdr, int mailHdr, int requestID, int CVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Signal){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->CVID == CVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingBroadcastRequest(int pkthdr, int mailHdr, int requestID, int CVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_Broadcast){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->CVID == CVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}

//Retuns the index into the pendingRequests table of the matching entry or -1 if not found.
int findPendingDestroyCVRequest(int pkthdr, int mailHdr, int requestID, int CVID){
    for(unsigned int i = 0; i < pendingRequests.size(); i++){
        PendingRequest* p = pendingRequests[i];
        if(p->requestID == requestID){
            if(p->type == SC_DestroyCondition){
                if(p->pktHdr == pkthdr && p->mailHdr == mailHdr){
                    if(p->CVID == CVID){
                        return (int)i;
                    }
                }
            }
        }
    }
    return -1;
}







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
#define lockTableSize 100
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


int checkIfLockIsMineAndGetMyIndex(int lockID){
    //lockID = postOffice->getNetworkAddress() * lockTableSize + index;
    
    bool print = false;
    int index;
    int machineID;

    

    index = lockID % lockTableSize;
    machineID = (lockID - index) / lockTableSize;


    if(print)printf("CheckIFLockIsMine: lockID: %i \n\tindex: %i \n\t machineID: %i \n\t", lockID, index, machineID);

    if(machineID == postOffice->getNetworkAddress() ){
        if(print)printf("Lock IS mine.\n");
        return index;
    }else{
        if(print)printf("Lock is NOT mine.\n");
        return -1;
    }
}

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
int findLockNamed(string name){
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

    
    return index;
}

int createLockNamed(string name){
    int index = findLockNamed(name);

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


void serverDoCreateLock(string name, int pktHdr, int mailHdr){
    int lockID = createLockNamed(name);//Find or create the lock

    lockID = postOffice->getNetworkAddress() * lockTableSize + lockID;
        
    printf("\t\tCreateLock named %s lockID %i.\n", name.c_str(), lockID);
        
    stringstream rs;
    rs << (lockID != -1);//status
    rs << " ";
    rs << lockID;

    char *msg = (char*) rs.str().c_str();

    sendMail(msg, pktHdr, mailHdr);
}

void serverCreateLock(string name, int pktHdr, int mailHdr){

    if(findLockNamed(name) != -1){//We have the lock and can handle the create....
        serverDoCreateLock(name, pktHdr, mailHdr);
    }else{
        printf("\t\tThis lock isn't ours...checking with other servers.\n");
        //This isn't our lock and need to check with the other servers...
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_CreateLock;
        p->name = name;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoCreateLock(name, pktHdr, mailHdr);
        }
    }
}






bool checkLockAndDestroy(int lockID){
    ServerLock* l;
    //lockID = checkIfLockIsMineAndGetMyIndex(lockID);
    if(!checkIfLockIDExists(lockID)){return FALSE;}
    
    l = serverLocks[lockID];

    if(l->isToBeDestroyed && l->state == FREE && l->createLockCount == 0){
        //Destroy the lock
        serverLocks[lockID] = NULL;
        delete l;
        serverLockTableBitMap.Clear(lockID);
        printf("\t\tDestroyed LockID: %i.\n", lockID);
        return TRUE;
    }else{
        //printf("\t\tNot ready to destroy lockID: %i\n", lockID);
        return FALSE;
    }
}


void serverDoAcquireLock(int lockID, int pktHdr, int mailHdr){
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
    printf("\t\tLock: %s\n", l->name.c_str());

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


void serverAcquireLock(int lockID, int pktHdr, int mailHdr){
    int myIndex = checkIfLockIsMineAndGetMyIndex(lockID);

    if(myIndex == -1){
        printf("\t\tNot my lock...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Acquire;
        p->lockID = lockID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoAcquireLock(myIndex, pktHdr, mailHdr);
        }
    }else{
        //This is my lock
        serverDoAcquireLock(myIndex, pktHdr, mailHdr);
    }
}

void serverDoReleaseLock(int lockID, int pktHdr, int mailHdr){
    ServerLock* l;
    //Check if lock exists.
    if(!checkIfLockIDExists(lockID)){return;}

    l = serverLocks[lockID];
    printf("\t\tLock: %s\n", l->name.c_str());

    if(! (l->isOwner(pktHdr, mailHdr)) ){
        //Only the lock owner can release the lock
        printf("\t\tOnly the lock owner can release the lock.\n");
        return;
    }else if(! (l->q->IsEmpty()) ){//Someone else waiting to acquire the lock
        ServerReplyMsg* r = (ServerReplyMsg*)(l->q->Remove());
        l->ownerMachineID = r->pktHdr;
        l->ownerMailboxNumber = r->mailHdr;
        sendMail(r->msg, r->pktHdr, r->mailHdr);
        printf("\t\tSent acquire confirmation to sleeping thread. MID: %i MB: %i\n", r->pktHdr, r->mailHdr);
        delete r;
    }else{//The lock is now free
        l->state = FREE;
        printf("\t\tLock is now Free.\n");
        checkLockAndDestroy(lockID);
    }
}

void serverReleaseLock(int lockID, int pktHdr, int mailHdr){

    int myIndex = checkIfLockIsMineAndGetMyIndex(lockID);
    if(myIndex == -1){
        printf("\t\tNot my lock...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Release;
        p->lockID = lockID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoReleaseLock(myIndex, pktHdr, mailHdr);
        }
    }else{
        //This is my lock
        serverDoReleaseLock(myIndex, pktHdr, mailHdr);
    }
   
}

void serverDoDestroyLock(int lockID){
    ServerLock* l;

     
    if(!checkIfLockIDExists(lockID)){return;}

    l = serverLocks[lockID];
    printf("\t\tLock: %s\n", l->name.c_str());

    //mark for deletion.
    l->isToBeDestroyed = TRUE;
    l->createLockCount--;
    if(!checkLockAndDestroy(lockID)){
        printf("\t\tNot ready to destroy lock.\n");
    }
}

void serverDestroyLock(int lockID, int pktHdr, int mailHdr){
     int myIndex = checkIfLockIsMineAndGetMyIndex(lockID);

     if(myIndex == -1){
        printf("\t\tNot my lock...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_DestroyLock;
        p->lockID = lockID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoDestroyLock(myIndex);
        }
    }else{
        //This is my lock
        serverDoDestroyLock(myIndex);
    }

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


#define CVTableSize 100
BitMap serverCVTableBitMap(CVTableSize);

class ServerCV{
public:
    string name;
    List *q;
    int waitingLock;
    int createCVCount;
    ServerCV(){
        q = new List();
        waitingLock = -1;
        createCVCount = 1;
    }
    ~ServerCV(){
        delete q;
    }
};

vector<ServerCV*> serverCVs; //The table of CVs



////////////////////
int checkIfCVIsMineAndGetMyIndex(int CVID){
    //lockID = postOffice->getNetworkAddress() * lockTableSize + index;
    
    bool print = false;
    int index;
    int machineID;

    

    index = CVID % CVTableSize;
    machineID = (CVID - index) / CVTableSize;


    if(print)printf("CheckIFCVIsMine: CVID: %i \n\tindex: %i \n\t machineID: %i \n\t", CVID, index, machineID);

    if(machineID == postOffice->getNetworkAddress() ){
        if(print)printf("CVID IS mine.\n");
        return index;
    }else{
        if(print)printf("CVID is NOT mine.\n");
        return -1;
    }
}

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


///////////////////
int findCVNamed(string name){
    int CVID = -1;
    for(unsigned int i = 0; i < serverCVs.size(); i++){
        if(serverCVs[i] != NULL){
            if(serverCVs[i]->name == name){
                CVID = (int)i;
                break;
            }
        }
    }
    return CVID;
}

////////////////////
int createCVNamed(string name){
    int CVID = findCVNamed(name);

    if(CVID == -1){
        //CV doesn't exist
        ServerCV* c = new ServerCV();
        c->name = name;

        CVID = serverCVTableBitMap.Find();

        if(CVID == -1){
            printf("\t\tMax Number of CVs created. CVTableSize: %i\n", CVTableSize);
            return -1;
        }else{
            if(CVID == (int)serverCVs.size()){
                serverCVs.push_back(c);
            }else if(CVID < (int)serverCVs.size()){
                serverCVs[CVID] = c;
            }else{ASSERT(FALSE);}
            printf("\t\tCreated CV named %s.\n", name.c_str());
        }
    }else{
        //CV exists
        serverCVs[CVID]->createCVCount++;
        printf("\t\tCV %s already Exists.\n", name.c_str());
    }

    return CVID;
}

/////////////////////
void serverDoCreateCV(string name, int pktHdr, int mailHdr){
    int CVID = -1;
    bool status = TRUE;

    CVID = createCVNamed(name);

    if(CVID == -1){
        status = FALSE;
    }else{
        CVID = postOffice->getNetworkAddress() * CVTableSize + CVID;
    }

    stringstream rs;
    rs << status;
    rs << " ";
    rs << CVID;

    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
}


/////////////////////
void serverCreateCV(string name, int pktHdr, int mailHdr){
    if(findCVNamed(name) != -1){//We have the MV and can handle the create....
        printf("\t\tThis CV belongs to me. Handling the create.\n");
        serverDoCreateCV(name, pktHdr, mailHdr);
    }else{
        printf("\t\tThis CV isn't ours...checking with other servers.\n");
        //This isn't our lock and need to check with the other servers...
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_CreateCondition;
        p->name = name;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoCreateCV(name, pktHdr, mailHdr);
        }
    }
}



/////////////
bool checkCVAndDestroy(int CVID){
    ServerCV* c;

    if(!checkIfCVIDExists(CVID)){return FALSE;}
    
    c = serverCVs[CVID];

    if(c->q->IsEmpty() && c->createCVCount == 0){
        //Destroy the CV
        serverCVs[CVID] = NULL;
        delete c;
        serverCVTableBitMap.Clear(CVID);
        printf("\t\tDestroyed CVID: %i.\n", CVID);
        return TRUE;
    }else{
        return FALSE;
    }
}



void serverDoDestroyCV(int CVID){
    if(!checkIfCVIDExists(CVID)){
        printf("\t\tUnable to destroy.\n");
        return;
    }

    serverCVs[CVID]->createCVCount--;
    if(!checkCVAndDestroy(CVID)){
        printf("\t\tNot ready to destroy CVID %i.\n", CVID);
    }
}

/////////////////
void serverDestroyCV(int CVID, int pktHdr, int mailHdr){
    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

    if(myIndex == -1){
        printf("\t\tNot my CV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_DestroyCondition;
        p->CVID = CVID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoDestroyCV(myIndex);
        }
    }else{
        //This is my MV
        serverDoDestroyCV(myIndex);
    }
}


void serverDoWait(int CVID, int lockID, int pktHdr, int mailHdr){
    ServerCV* c;
    stringstream rs;

    if(!checkIfCVIDExists(CVID)){//Return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    c = serverCVs[CVID];
    printf("\t\tCV: %s\n", c->name.c_str());

    if(c->waitingLock == -1){
        printf("\t\tNo threads waiting. Setting CV lockID %i.\n", lockID);
        c->waitingLock = lockID;
    }

    if(c->waitingLock != lockID){
        printf("\t\tCV lockID %i does not match lockID %i passed to Wait.\n",c->waitingLock, lockID);
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
    printf("\t\tMID: %i MB: %i added to waitQ.\n", pktHdr, mailHdr );
    //Release waiting lock...
    serverReleaseLock(lockID, pktHdr, mailHdr);

}

void serverWait(int CVID, int lockID, int pktHdr, int mailHdr){
    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

    if(myIndex == -1){
        printf("\t\tNot my CV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Wait;
        p->CVID = CVID;
        p->lockID = lockID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoWait(myIndex, lockID, pktHdr, mailHdr);
        }
    }else{
        //This is my MV
        serverDoWait(myIndex, lockID, pktHdr, mailHdr);
    }
}


/*/////////////////// OLD PRE-MULTIPLE SERVER CODE
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
    printf("\t\tLock: %s", l->name.c_str());
    printf("\t\tCV: %s\n", c->name.c_str());
    if(!l->isOwner(pktHdr, mailHdr)){
        printf("\t\tMust be lock owner to wait.\n");
        //Return error....
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    if(c->waitingLock == -1){
        printf("\t\tNo threads waiting. Setting CV lockID %i.\n", lockID);
        c->waitingLock = lockID;
        if(c->waitingLock == -1){
            printf("\t\tWTF!!!\n");
        }
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
    printf("\t\tMID: %i MB: %i added to waitQ.\n", pktHdr, mailHdr );
    //Release waiting lock...
    serverReleaseLock(lockID, pktHdr, mailHdr);
}*/

/////////////////////////
void serverDoSignal(int CVID, int lockID, int pktHdr, int mailHdr){
    ServerCV* c;
    stringstream rs;

    if(!checkIfCVIDExists(CVID)){//Return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }


    c = serverCVs[CVID];
    printf("\t\tCV: %s\n", c->name.c_str());

    if(c->waitingLock == -1){ //if no thread waiting
        if(!c->q->IsEmpty()){
            printf("\t\tERROR Q is not empty but waitingLock is NULL\n");
        }
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
        c->waitingLock = -1;
    }

    //We need to have the 'woken up' thread acquire the lock
    serverAcquireLock(lockID, r->pktHdr, r->mailHdr);

    printf("\t\tSignalled %i:%i.\n",r->pktHdr, r->mailHdr);
    delete r;
    
    rs << TRUE;
    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
    return;
}



//////////////////
void serverSignal(int CVID, int lockID, int pktHdr, int mailHdr){
    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

    if(myIndex == -1){
        printf("\t\tNot my CV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Signal;
        p->CVID = CVID;
        p->lockID = lockID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoSignal(myIndex, lockID, pktHdr, mailHdr);
        }
    }else{
        //This is my MV
        serverDoSignal(myIndex, lockID, pktHdr, mailHdr);
    }
}



/*///////////////////// OLD serverSignal PREMultiptleServers
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

    printf("\t\tLock: %s", l->name.c_str());
    printf("\t\tCV: %s\n", c->name.c_str());

    if(!l->isOwner(pktHdr, mailHdr)){
        printf("\t\tMust be lock owner to signal.\n");
        //Return error....
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }


    if(c->waitingLock == -1){ //if no thread waiting
        if(!c->q->IsEmpty()){
            printf("\t\tERROR Q is not empty but waitingLock is NULL\n");
        }
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
        c->waitingLock = -1;
    }

    //We need to have the 'woken up' thread acquire the lock
    serverAcquireLock(lockID, r->pktHdr, r->mailHdr);

    printf("\t\tSignalled %i:%i.\n",r->pktHdr, r->mailHdr);
    delete r;
    
    rs << TRUE;
    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
    return;
}*/


void serverDoBroadcast(int CVID, int lockID, int pktHdr, int mailHdr){
    ServerCV* c;
    stringstream rs;

    if(!checkIfCVIDExists(CVID)){//Return error...
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    c = serverCVs[CVID];

    printf("\t\tCV: %s\n", c->name.c_str());

    while( !( c->q->IsEmpty() ) ){
        //Wakeup 1 waiting thread
        ServerReplyMsg* r = (ServerReplyMsg *)c->q->Remove();

        if(c->q->IsEmpty()){
            c->waitingLock = -1;
        }

        //We need to have the 'woken up' thread acquire the lock
        serverAcquireLock(lockID, r->pktHdr, r->mailHdr);
    }

    rs << TRUE;
    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
    return;
}



//////////////////
void serverBroadcast(int CVID, int lockID, int pktHdr, int mailHdr){
    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

    if(myIndex == -1){
        printf("\t\tNot my CV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Broadcast;
        p->CVID = CVID;
        p->lockID = lockID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoBroadcast(myIndex, lockID, pktHdr, mailHdr);
        }
    }else{
        //This is my MV
        serverDoBroadcast(myIndex, lockID, pktHdr, mailHdr);
    }
}


/*///////////////////// PREMultiServers
void serverBroadcast(int CVID, int lockID, int pktHdr, int mailHdr){

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

    printf("\t\tLock: %s", l->name.c_str());
    printf("\t\tCV: %s\n", c->name.c_str());

    if(!l->isOwner(pktHdr, mailHdr)){
        printf("\t\tMust be lock owner to Broadcast.\n");
        //Return error....
        rs << FALSE;
        sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
        return;
    }

    while( !( c->q->IsEmpty() ) ){
        //Wakeup 1 waiting thread
        ServerReplyMsg* r = (ServerReplyMsg *)c->q->Remove();

        if(c->q->IsEmpty()){
            c->waitingLock = -1;
        }

        //We need to have the 'woken up' thread acquire the lock
        serverAcquireLock(lockID, r->pktHdr, r->mailHdr);
    }

    rs << TRUE;
    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
    return;
}*/







/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////  ////////////////  ////  /////////////////////  ///////////
/////////////  /  //////////  /  /////  //////////////////  /////////////
/////////////  //  ////////  //  ///////  //////////////  ///////////////
/////////////  ///  //////  ///  /////////  //////////  /////////////////
/////////////  ////  ////  ////  ///////////  //////  ///////////////////
/////////////  /////  //  /////  /////////////  //  /////////////////////
/////////////  //////  ////////  ///////////////  ////////////S//////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////



#define MVTableSize 100
BitMap serverMVTableBitMap(MVTableSize);

class ServerMV{
public:
    string name;
    int size;
    int createMVCount;
    vector<int> v;
    ServerMV(string nam, int mvSize){
        name = nam;
        size = mvSize;
        createMVCount = 1;
        v.resize(size, 0);
    }
    ~ServerMV(){
        v.clear();
    }
};

vector<ServerMV*> serverMVs; //The table of MVs



int checkIfMVIsMineAndGetMyIndex(int MVID){
    //lockID = postOffice->getNetworkAddress() * lockTableSize + index;
    
    bool print = false;
    int index;
    int machineID;

    

    index = MVID % MVTableSize;
    machineID = (MVID - index) / MVTableSize;


    if(print)printf("CheckIFMVIsMine: MVID: %i \n\tindex: %i \n\t machineID: %i \n\t", MVID, index, machineID);

    if(machineID == postOffice->getNetworkAddress() ){
        if(print)printf("MVID IS mine.\n");
        return index;
    }else{
        if(print)printf("MVID is NOT mine.\n");
        return -1;
    }
}


////////////////////
bool checkIfMVIDExists(int MVID){
    //Check if MV exists.
    if(MVID >= (int)serverMVs.size() || MVID < 0){
        printf("\t\tMVID %i does not exist.\n", MVID);
        return FALSE;
    }else if(serverMVs[MVID] == NULL){
        printf("\t\tMVID %i no longer exists.\n", MVID);
        return FALSE;
    }
    return TRUE;
}


int findMVNamed(string name){
    int MVID = -1;
    for(unsigned int i = 0; i < serverMVs.size(); i++){
        if(serverMVs[i] != NULL){
            if(serverMVs[i]->name == name){
                MVID = (int)i;
                break;
            }
        }
    }
    return MVID;
}


int createMV(string name, int size){
    
    int MVID = findMVNamed(name);

    if(MVID == -1){
        //MV doesn't exist
        ServerMV* m;

        MVID = serverMVTableBitMap.Find();

        if(MVID == -1){
            printf("\t\tMax Number of MVs created. MVTableSize: %i\n", MVTableSize);
            return -1;
        }else{
            m = new ServerMV(name, size);
            if(MVID == (int)serverMVs.size()){
                serverMVs.push_back(m);
            }else if(MVID < (int)serverMVs.size()){
                serverMVs[MVID] = m;
            }else{ASSERT(FALSE);}
            printf("\t\tCreated MV \"%s\" size %i.\n", m->name.c_str(), m->size);
        }
    }else{
        //MV name exists
        if(serverMVs[MVID]->size != size){
            printf("\t\tMV with this name already exists...but with a different size! Returning an error.\n");
            return -1;
        }else{
            serverMVs[MVID]->createMVCount++;
            printf("\t\tMV already Exists.\n");
        }
    }

    return MVID;
}

void serverDoCreateMV(string name, int size, int pktHdr, int mailHdr){
    int MVID = -1;
    bool status = TRUE;

    MVID = createMV(name, size);

    if(MVID == -1){
        status = FALSE;
    }else{
        MVID = postOffice->getNetworkAddress() * MVTableSize + MVID;
    }

    stringstream rs;
    rs << status;
    rs << " ";
    rs << MVID;

    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
}


///////////////////////////
void serverCreateMV(string name, int size, int pktHdr, int mailHdr){
    
    if(findMVNamed(name) != -1){//We have the MV and can handle the create....
        serverDoCreateMV(name, size, pktHdr, mailHdr);
    }else{
        printf("\t\tThis MV isn't ours...checking with other servers.\n");
        //This isn't our lock and need to check with the other servers...
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_CreateMV;
        p->name = name;
        p->MVIndex = size;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoCreateMV(name, size, pktHdr, mailHdr);
        }
    }
}

////////////////////////////
void serverDoDestroyMV(int MVID){
    ServerMV* m;

    if(!checkIfMVIDExists(MVID)){
        printf("MVID not found. Unable to destroy.\n");
        return;
    }

    m = serverMVs[MVID];

    m->createMVCount--;

    if( m->createMVCount == 0 ){
        delete m;
        serverMVs[MVID] = NULL;
        serverMVTableBitMap.Clear(MVID);
        printf("\t\tDestroyed MVID: %i.\n", MVID);
    }else{
        printf("\t\tNot ready to destroy MVID %i.\n", MVID);
    }
}


////////////////////////////
void serverDestroyMV(int MVID, int pktHdr, int mailHdr){
    int myIndex = checkIfMVIsMineAndGetMyIndex(MVID);

     if(myIndex == -1){
        printf("\t\tNot my MV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_DestroyMV;
        p->MVID = MVID;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoDestroyMV(myIndex);
        }
    }else{
        //This is my MV
        serverDoDestroyMV(myIndex);
    }
}


///////////////////////////////
void serverDoSet(int MVID, int index, int value, int pktHdr, int mailHdr){
    ServerMV* m;
    bool status = TRUE;

    if(!checkIfMVIDExists(MVID)){
        status = FALSE;
    }else{

        m = serverMVs[MVID];
        printf("\t\tMV: %s\n", m->name.c_str());

        //Bounds Check for index
        if(index < 0 || index >= m->size){
            printf("\t\tInvalid index %i passed to Set.\n", index);
            status = FALSE;
        }

        if(status){
            m->v[index] = value;
        }
    }

    stringstream rs;
    rs << status;

    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);

    if(status)
        printf("\t\tSet MVID(%i)[%i] to %i.\n", MVID, index, value);
    else
        printf("\t\tSet error.\n");
}


//////////////////////////////////
void serverSet(int MVID, int index, int value, int pktHdr, int mailHdr){
    int myIndex = checkIfMVIsMineAndGetMyIndex(MVID);

     if(myIndex == -1){
        printf("\t\tNot my MV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Set;
        p->MVID = MVID;
        p->MVIndex = index;
        p->MVValue = value;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoSet(myIndex, index, value, pktHdr, mailHdr);
        }
    }else{
        //This is my MV
        serverDoSet(myIndex, index, value, pktHdr, mailHdr);
    }
}

//////////////////////////////////
void serverDoGet(int MVID, int index, int pktHdr, int mailHdr){
    ServerMV* m;
    bool status = TRUE;
    int value = 0;

    if(!checkIfMVIDExists(MVID)){
        status = FALSE;
    }else{

        m = serverMVs[MVID];
        printf("\t\tMV: %s\n", m->name.c_str());

        //Bounds Check for index
        if(index < 0 || index >= m->size){
            printf("\t\tInvalid index %i passed to Get.\n", index);
            status = FALSE;
        }

        if(status){
            value = m->v[index];
            printf("\t\tReturning MVID(%i)[%i]: %i.\n", MVID, index, value);
        }
    }

    stringstream rs;
    rs << status;
    rs << " ";
    rs << value;

    sendMail((char*)rs.str().c_str(), pktHdr, mailHdr);
}

//////////////////////////////////
void serverGet(int MVID, int index, int pktHdr, int mailHdr){
    int myIndex = checkIfMVIsMineAndGetMyIndex(MVID);

     if(myIndex == -1){
        printf("\t\tNot my MV...checking with other servers.\n");
        PendingRequest* p = new PendingRequest();
        p->pktHdr = pktHdr;
        p->mailHdr = mailHdr;
        p->type = SC_Get;
        p->MVID = MVID;
        p->MVIndex = index;
        pendingRequests.push_back(p);
        if(!sendPendingRequest(p)){
            deletePendingRequestPointer(p);
            serverDoGet(myIndex, index, pktHdr, mailHdr);
        }
    }else{
        //This is my MV
        serverDoGet(myIndex, index, pktHdr, mailHdr);
    }
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
    int mailBoxNumber = currentThread->getThreadID();
    char buffer[MaxMailSize];
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    bool success;
    outMailHdr.from = mailBoxNumber;

    printf("Starting nachos network server MID: %d, MailBox: %d.\n", postOffice->getNetworkAddress(), mailBoxNumber);

    while(TRUE){
        //Revieve a msg
        //Parse msg
        //Process msg
        //Reply(maybe)
        stringstream ss;
        bool fromServer = false;
        int requestID;
        bool serverRequest = false; // If this message is a request
        bool serverReply = false;   // or a reply to a request

        postOffice->Receive(mailBoxNumber, &inPktHdr, &inMailHdr, buffer);
        ss << buffer;
        printf("\nServer: Received Message from machine %d, mailbox %d:\"%s\".\n", inPktHdr.from, inMailHdr.from, ss.str().c_str());
        fflush(stdout);


        int which;
        ss >> which;


        fromServer = ( inPktHdr.from < 5 );
        if( fromServer ){
            int request;
            ss >> request;
            if(request){
                printf("\tServer Request.\n");
                serverRequest = true;

            }else{
                printf("\tServer Reply\n");
                serverReply = true;
            }

            ss >> requstID;
        }




        // SC_CreateLock
        if(which == SC_CreateLock){
            printf("\tCreateLock\n");

            string lockName;

            if(!fromServer){
                
                ss >> lockName;

                serverCreateLock(lockName, inPktHdr.from, inMailHdr.from);
            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> lockName;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingCreateLockRequest(reqPktHdr, reqMailHdr, requestID, lockName);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    //printf("%s\n", p->toString().c_str());
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                        //We need to delete all pending requests for this name... 
                        while(true){
                            pendingRequestIndex = findPendingCreateLockRequestNamed(lockName);
                            if(pendingRequestIndex == -1){
                                break;
                            }else{
                                deletePendingRequest(pendingRequestIndex);
                            }
                        }
                        
                        //and send out a pending request for each other request with same name//No we don't

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoCreateLock(p->name, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                            //handle all requests with same name...
                            while(true){
                                pendingRequestIndex = findPendingCreateLockRequestNamed(lockName);
                                if(pendingRequestIndex == -1){
                                    break;
                                }else{
                                    printf("\tFound another pending request with same name. Handling the request.\n");
                                    p = pendingRequests[pendingRequestIndex];
                                    serverDoCreateLock(p->name, p->pktHdr, p->mailHdr);
                                    deletePendingRequest(pendingRequestIndex);
                                }
                            }
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    
                    if(findLockNamed(lockName) != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis lock is ours...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_CreateLock << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << lockName << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoCreateLock(lockName, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO(or YES)
                        

                        //WHAT IF THERE IS ALREADY A PENDING CREATE FOR THIS LOCK!?
                        if(findPendingCreateLockRequestNamed(lockName) != -1){
                            //if my machine id lower reply YES
                            if(postOffice->getNetworkAddress() < inPktHdr.from){
                                printf("\t\tPending request found for this lock, and our machineID is lower...reply YES.\n");
                                stringstream rs;
                                rs << SC_CreateLock << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockName << " " << true;
                                sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                                
                                //and add another pending request for this lock...
                                PendingRequest* p = new PendingRequest();
                                p->pktHdr = reqPktHdr;
                                p->mailHdr = reqMailHdr;
                                p->type = SC_CreateLock;
                                p->name = lockName;
                                pendingRequests.push_back(p);
                            }else{
                                printf("\t\tPending request for this lock found but they have a lower machine ID...reply NO.\n");
                                // if my machine id higher reply NO
                                stringstream rs;
                                rs << SC_CreateLock << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockName << " " << false;
                                sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                            }
                        }else{
                            printf("\t\tThis lock is not ours and no pending requests...reply NO.\n");
                            stringstream rs;
                            rs << SC_CreateLock << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockName << " " << false;
                            sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                        }

                    }
                }

            }
           

        }//SC_Acquire
        else if(which == SC_Acquire){
            printf("\tAcquire:\n");
            int lockID;

            if(!fromServer){
                
                ss >> lockID;
                serverAcquireLock(lockID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> lockID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingAcquireLockRequest(reqPktHdr, reqMailHdr, requestID, lockID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoAcquireLock(lockID, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfLockIsMineAndGetMyIndex(lockID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis lock is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Acquire << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoAcquireLock(myIndex, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis lock is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Acquire << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }

        }
        else if(which == SC_Release){
            printf("\tRelease:\n");
            int lockID;

            if(!fromServer){
                
                ss >> lockID;
                serverReleaseLock(lockID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> lockID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingReleaseLockRequest(reqPktHdr, reqMailHdr, requestID, lockID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoReleaseLock(lockID, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfLockIsMineAndGetMyIndex(lockID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis lock is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Release << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << lockID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoReleaseLock(myIndex, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis lock is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Release << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }
        

        }
        else if (which == SC_DestroyLock){
            printf("\tDestroyLock:\n");
            int lockID;
            

            if(!fromServer){
                
                ss >> lockID;
                serverDestroyLock(lockID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> lockID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingDestroyLockRequest(reqPktHdr, reqMailHdr, requestID, lockID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoDestroyLock(lockID);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfLockIsMineAndGetMyIndex(lockID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis lock is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_DestroyLock << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << lockID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoDestroyLock(myIndex);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis lock is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_DestroyLock << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << lockID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }






            //serverDestroyLock(lockID);


        }
        else if( which == SC_CreateCondition){
            printf("\tCreateCondition:\n");
            string name;

            if(!fromServer){
                
                ss >> name;
                serverCreateCV(name, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> name;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingCreateCVRequest(reqPktHdr, reqMailHdr, requestID, name);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    //printf("%s\n", p->toString().c_str());
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                        //We need to delete all pending requests for this name... 
                        while(true){
                            pendingRequestIndex = findPendingCreateCVRequestNamed(name);
                            if(pendingRequestIndex == -1){
                                break;
                            }else{
                                deletePendingRequest(pendingRequestIndex);
                            }
                        }
                        
                        //and send out a pending request for each other request with same name//No we don't

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoCreateCV(p->name, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                            //handle all requests with same name...
                            while(true){
                                pendingRequestIndex = findPendingCreateCVRequestNamed(name);
                                if(pendingRequestIndex == -1){
                                    break;
                                }else{
                                    printf("\tFound another pending request with same name. Handling the request.\n");
                                    p = pendingRequests[pendingRequestIndex];
                                    serverDoCreateCV(p->name, p->pktHdr, p->mailHdr);
                                    deletePendingRequest(pendingRequestIndex);
                                }
                            }
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    
                    if(findCVNamed(name) != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis CV is ours...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_CreateCondition << " " << false << requestID << " " << " " << reqPktHdr << " " << reqMailHdr << " " << name << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoCreateCV(name, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO(or YES)
                        

                        //WHAT IF THERE IS ALREADY A PENDING CREATE FOR THIS LOCK!?
                        if(findPendingCreateCVRequestNamed(name) != -1){
                            //if my machine id lower reply YES
                            if(postOffice->getNetworkAddress() < inPktHdr.from){
                                printf("\t\tPending request found for this CV, and our machineID is lower...reply YES.\n");
                                stringstream rs;
                                rs << SC_CreateCondition << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << name << " " << true;
                                sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                                
                                //and add another pending request for this lock...
                                PendingRequest* p = new PendingRequest();
                                p->pktHdr = reqPktHdr;
                                p->mailHdr = reqMailHdr;
                                p->type = SC_CreateCondition;
                                p->name = name;
                                pendingRequests.push_back(p);
                            }else{
                                printf("\t\tPending request for this CV found but they have a lower machine ID...reply NO.\n");
                                // if my machine id higher reply NO
                                stringstream rs;
                                rs << SC_CreateCondition << " " << requestID << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << name << " " << false;
                                sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                            }
                        }else{
                            printf("\t\tThis CV is not ours and no pending requests...reply NO.\n");
                            stringstream rs;
                            rs << SC_CreateCondition << " " << requestID << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << name << " " << false;
                            sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                        }

                    }
                }

            }

        }
        else if(which == SC_Wait){
            printf("\tWait:\n");

            int CVID;
            int lockID;
            
            if(!fromServer){
                
                ss >> CVID;
                ss >> lockID;
                serverWait(CVID, lockID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> CVID;
                ss >> lockID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingWaitRequest(reqPktHdr, reqMailHdr, requestID, CVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoWait(CVID, lockID, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis CV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Wait << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << lockID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoWait(myIndex, lockID, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis CV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Wait << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << lockID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }

        }
        else if(which == SC_Signal){
            printf("\tSignal:\n");

            int CVID;
            int lockID;

            if(!fromServer){
                
                ss >> CVID;
                ss >> lockID;
                serverSignal(CVID, lockID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> CVID;
                ss >> lockID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingSignalRequest(reqPktHdr, reqMailHdr, requestID, CVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoSignal(CVID, lockID, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis CV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Signal << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << lockID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoSignal(myIndex, lockID, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis CV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Signal << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << lockID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }

        }
        else if(which == SC_Broadcast){
            printf("\tBroadcast:\n");

            int CVID;
            int lockID;

            if(!fromServer){
                
                ss >> CVID;
                ss >> lockID;
                serverBroadcast(CVID, lockID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> CVID;
                ss >> lockID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingBroadcastRequest(reqPktHdr, reqMailHdr, requestID, CVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoBroadcast(CVID, lockID, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis CV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Broadcast << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << lockID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoBroadcast(myIndex, lockID, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis CV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Broadcast << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << lockID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }

        }
        else if(which == SC_DestroyCondition){
            printf("\tDestroyCondition:\n");

            int CVID;

            if(!fromServer){
                
                ss >> CVID;
                serverDestroyCV(CVID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> CVID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingDestroyCVRequest(reqPktHdr, reqMailHdr, requestID, CVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoDestroyCV(CVID);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfCVIsMineAndGetMyIndex(CVID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis CV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_DestroyCondition << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoDestroyCV(myIndex);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis CV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_DestroyCondition << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << CVID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }

        }
        else if(which == SC_CreateMV){
            printf("\tCreateMV:\n");

            string name;
            int size;

            //ss >> size >> name;
            //printf("\n\n\t\tSIZE: %i NAME:%s.\n\n", size, name.c_str());
            //serverCreateMV(name, size, inPktHdr.from, inMailHdr.from);

            if(!fromServer){
                
                ss >> size >> name;

                serverCreateMV(name, size, inPktHdr.from, inMailHdr.from);
            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> size;
                ss >> name;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingCreateMVRequest(reqPktHdr, reqMailHdr, requestID, name);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. It was propbably already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                        //We need to delete all pending requests for this name... 
                        while(true){
                            pendingRequestIndex = findPendingCreateMVRequestNamed(name);
                            if(pendingRequestIndex == -1){
                                break;
                            }else{
                                deletePendingRequest(pendingRequestIndex);
                            }
                        }
                        
                        //and send out a pending request for each other request with same name//No we don't

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoCreateMV(p->name, p->MVIndex, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                            //handle all requests with same name...
                            while(true){
                                pendingRequestIndex = findPendingCreateMVRequestNamed(name);
                                if(pendingRequestIndex == -1){
                                    break;
                                }else{
                                    printf("\tFound another pending request with same name. Handling the request.\n");
                                    p = pendingRequests[pendingRequestIndex];
                                    serverDoCreateMV(p->name, p->MVIndex, p->pktHdr, p->mailHdr);
                                    deletePendingRequest(pendingRequestIndex);
                                }
                            }
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    
                    if(findMVNamed(name) != -1){
                        //If this MV is ours reply YES and handle the request.
                        printf("\t\tThis MV is ours...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_CreateMV << " " << false << " " << reqPktHdr << " " << reqMailHdr << " " << size << " " << name << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoCreateMV(name, size, reqPktHdr, reqMailHdr);
                    }else{
                        //If this lock does not belong to us...reply NO(or YES)
                        

                        //WHAT IF THERE IS ALREADY A PENDING CREATE FOR THIS LOCK!?
                        if(findPendingCreateMVRequestNamed(name) != -1){
                            //if my machine id lower reply YES
                            if(postOffice->getNetworkAddress() < inPktHdr.from){
                                printf("\t\tPending request found for this MV, and our machineID is lower...reply YES.\n");
                                stringstream rs;
                                rs << SC_CreateMV << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << size << " " << name << " " << true;
                                sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                                
                                //and add another pending request for this lock...
                                PendingRequest* p = new PendingRequest();
                                p->pktHdr = reqPktHdr;
                                p->mailHdr = reqMailHdr;
                                p->type = SC_CreateMV;
                                p->name = name;
                                p->MVIndex = size;
                                pendingRequests.push_back(p);
                            }else{
                                printf("\t\tPending request for this MV found but they have a lower machine ID...reply NO.\n");
                                // if my machine id higher reply NO
                                stringstream rs;
                                rs << SC_CreateMV << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << size << " " << name << " " << false;
                                sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                            }
                        }else{
                            printf("\t\tThis MV is not ours and no pending requests...reply NO.\n");
                            stringstream rs;
                            rs << SC_CreateMV << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << size << " " << name << " " << false;
                            sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                        }

                    }
                }

            }

        }
        else if(which == SC_DestroyMV){
            printf("\tDestroyMV:\n");

            int MVID;


            if(!fromServer){
                ss >> MVID;
                serverDestroyMV(MVID, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> MVID;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingDestroyMVRequest(reqPktHdr, reqMailHdr, requestID, MVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. It was most likely already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoDestroyMV(MVID);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfMVIsMineAndGetMyIndex(MVID);

                    if(myIndex != -1){
                        //If this lock is ours reply YES and handle the request.
                        printf("\t\tThis MV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_DestroyMV << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << MVID << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoDestroyMV(myIndex);
                    }else{
                        //If this lock does not belong to us...reply NO
                        printf("\t\tThis MV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_DestroyMV << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << MVID << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }

        }
        else if(which == SC_Set){
            printf("\tSet:\n");

            int MVID, index, value;

            if(!fromServer){
                
                ss >> MVID >> index >> value;
                serverSet(MVID, index, value, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> MVID;
                ss >> index;
                ss >> value;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingSetMVRequest(reqPktHdr, reqMailHdr, requestID, MVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoSet(MVID, index, value, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfMVIsMineAndGetMyIndex(MVID);

                    if(myIndex != -1){
                        //If this MV is ours reply YES and handle the request.
                        printf("\t\tThis MV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Set << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << MVID << " " << index << " " << value << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoSet(myIndex, index, value, reqPktHdr, reqMailHdr);
                    }else{
                        //If this MV does not belong to us...reply NO
                        printf("\t\tThis MV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Set << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << MVID << " " << index << " " << value << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }


        }
        else if(which == SC_Get){
            printf("\tGet:\n");

            int MVID, index;


            if(!fromServer){
                
                ss >> MVID >> index;
                serverGet(MVID, index, inPktHdr.from, inMailHdr.from);

            }else{
                //This message came from a server...
                int reqPktHdr;
                int reqMailHdr;
                ss >> reqPktHdr;
                ss >> reqMailHdr;
                ss >> MVID;
                ss >> index;

                if(serverReply){
                    bool response;
                    ss >> response;
                    PendingRequest* p;
                    int pendingRequestIndex = findPendingGetMVRequest(reqPktHdr, reqMailHdr, requestID, MVID);
                    if(pendingRequestIndex == -1){printf("\t\tThis request was not found. Hopefully it was already handled.\n"); continue;}
                    p = pendingRequests[pendingRequestIndex];
                    if(response){
                        //Was a YES
                        printf("\t\tGot a YES.\n");
                        //We can delete the request...The other server will handle it.
                        deletePendingRequest(pendingRequestIndex);

                    }else{
                        //Was a NO
                        printf("\t\tGot a NO.\n");
                        p->noCount++;
                        if(p->noCount == p->sentCount){
                            //All servers replied NO...we need to handle the request.
                            printf("\t\tAll servers have responded. Handling the request.\n");
                            serverDoGet(MVID, index, p->pktHdr, p->mailHdr);
                            deletePendingRequest(pendingRequestIndex);
                        }
                    }
                }else{
                    //Server request message
                    //Need to send a reply of some kind...possibly handle the request...
                    int myIndex = checkIfMVIsMineAndGetMyIndex(MVID);

                    if(myIndex != -1){
                        //If this MV is ours reply YES and handle the request.
                        printf("\t\tThis MV is ours...reply YES...I'll handle the request.\n");
                        stringstream rs;
                        rs << SC_Get << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << MVID << " " << index  << " " << true;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);

                        serverDoGet(myIndex, index, reqPktHdr, reqMailHdr);
                    }else{
                        //If this MV does not belong to us...reply NO
                        printf("\t\tThis MV is not ours...reply NO.\n");
                        stringstream rs;
                        rs << SC_Get << " " << false << " " << requestID << " " << reqPktHdr << " " << reqMailHdr << " " << MVID << " " << index  << " " << false;
                        sendMail((char*)rs.str().c_str(), inPktHdr.from, inMailHdr.from);
                    }
                }

            }









        }
        else{
            printf("\tERROR: Unknown request.\n");
        }



        printf("\n");
    }//End whileLOOP
}//End Server()
