// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "synch.h"
#include "bitmap.h"
#ifdef NETWORK
	#include "network.h"
	#include "post.h"
	#include <sstream>
#endif

using namespace std;

int copyin(unsigned int vaddr, int len, char *buf) {
		// Copy len bytes from the current thread's virtual address vaddr.
		// Return the number of bytes so read, or -1 if an error occors.
		// Errors can generally mean a bad virtual address was passed in.
		bool result;
		int n=0;			// The number of bytes copied in
		int *paddr = new int;

		while ( n >= 0 && n < len) {
			result = machine->ReadMem( vaddr, 1, paddr );
			while(!result) // FALL 09 CHANGES
		{
				result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
		}	
			
			buf[n++] = *paddr;
		 
			if ( !result ) {
	//translation failed
	return -1;
			}

			vaddr++;
		}

		delete paddr;
		return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
		// Copy len bytes to the current thread's virtual address vaddr.
		// Return the number of bytes so written, or -1 if an error
		// occors.  Errors can generally mean a bad virtual address was
		// passed in.
		bool result;
		int n=0;			// The number of bytes copied in

		while ( n >= 0 && n < len) {
			// Note that we check every byte's address
			result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

			if ( !result ) {
	//translation failed
	return -1;
			}

			vaddr++;
		}

		return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
		// Create the file with the name in the user buffer pointed to by
		// vaddr.  The file name is at most MAXFILENAME chars long.  No
		// way to return errors, though...
		char *buf = new char[len+1];	// Kernel buffer to put the name in

		if (!buf) return;

		if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
		}

		buf[len]='\0';

		fileSystem->Create(buf,0);
		delete[] buf;
		return;
}

int Open_Syscall(unsigned int vaddr, int len) {
		// Open the file with the name in the user buffer pointed to by
		// vaddr.  The file name is at most MAXFILENAME chars long.  If
		// the file is opened successfully, it is put in the address
		// space's file table and an id returned that can find the file
		// later.  If there are any errors, -1 is returned.
		char *buf = new char[len+1];	// Kernel buffer to put the name in
		OpenFile *f;			// The new open file
		int id;				// The openfile id

		if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
		}

		if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
		}

		buf[len]='\0';

		f = fileSystem->Open(buf);
		delete[] buf;

		if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
			delete f;
	return id;
		}
		else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
		// Write the buffer to the given disk file.  If ConsoleOutput is
		// the fileID, data goes to the synchronized console instead.  If
		// a Write arrives for the synchronized Console, and no such
		// console exists, create one. For disk files, the file is looked
		// up in the current address space's open file table and used as
		// the target of the write.
		
		char *buf;		// Kernel buffer for output
		OpenFile *f;	// Open file for output

		if ( id == ConsoleInput) return;
		
		if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
		} else {
				if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to write: data not written\n");
			delete[] buf;
			return;
	}
		}

		if ( id == ConsoleOutput) {
			for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
			}

		} else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
			f->Write(buf, len);
	} else {
			printf("%s","Bad OpenFileId passed to Write\n");
			len = -1;
	}
		}

		delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
		// Write the buffer to the given disk file.  If ConsoleOutput is
		// the fileID, data goes to the synchronized console instead.  If
		// a Write arrives for the synchronized Console, and no such
		// console exists, create one.    We reuse len as the number of bytes
		// read, which is an unnessecary savings of space.
		char *buf;		// Kernel buffer for input
		OpenFile *f;	// Open file for output

		if ( id == ConsoleOutput) return -1;
		
		if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
		}

		if ( id == ConsoleInput) {
			//Reading from the keyboard
			scanf("%s", buf);

			if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
			}
		} else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
			len = f->Read(buf, len);
			if ( len > 0 ) {
					//Read something from the file. Put into user's address space
						if ( copyout(vaddr, len, buf) == -1 ) {
				printf("%s","Bad pointer passed to Read: data not copied\n");
		}
			}
	} else {
			printf("%s","Bad OpenFileId passed to Read\n");
			len = -1;
	}
		}

		delete[] buf;
		return len;
}

void Close_Syscall(int fd) {
		// Close the file associated with id fd.  No error reporting.
		OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

		if ( f ) {
			delete f;
		} else {
			printf("%s","Tried to close an unopen file\n");
		}
}

////////////////////////////////////////////////////////
/*******************************************************
	Our implementations for proj2 above code was provided
	*****************************************************/

Lock kernel_threadLock("Kernel Thread Lock");
//int forkCalled = 0;

void kernel_thread(int vaddr){
	DEBUG('f', "IN kernel_thread.\n");
 	
	currentThread->space->Fork(vaddr);//add stack space to pagetable and init registers...

	//(ProcessTable->getProcessEntry(currentThread->space))->addThread();
	//kernel_threadLock.Acquire();
	//forkCalled--;
  //kernel_threadLock.Release();
	DEBUG('f', "End kernel_thread.\n");
	machine->Run();
	ASSERT(FALSE);
}

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 */
void Fork_Syscall(int funct){
	kernel_threadLock.Acquire();
	(ProcessTable->getProcessEntry(currentThread->space))->addThread();
  kernel_threadLock.Release();
	Thread* t;
	DEBUG('f', "In fork syscall. funct = %i\n", funct);
	t = new Thread("Forked thread.");
	t->space = currentThread->space;
 // DEBUG('f', "CurrentSpace: %i  TSpace: %i\n", currentThread->space, t->space);
	t->Fork((VoidFunctionPtr)kernel_thread, funct); //kernel_thread??
	currentThread->Yield();//It should not be necessary to yield here
	DEBUG('f', "End of Fork Syscall.\n");
}//end Fork_Syscall


Lock execLock("ExecLock");
int execCalled = 0;

/************************************************************************
* Run the executable, stored in the Nachos file "name", and return the  *
* address space identifier                                              *
***********************************************************************/
//Lock* kernel_exec_lock = new Lock("Kernel Exec lock for filename...");
//char *kernel_execBUF = null;
void kernel_exec(int intName){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	char* name = (char*)intName;
	DEBUG('e', "Kernel_exec system call: FileName: %s \n\n", name);

	/*OpenFile *executable = fileSystem->Open(name);
	

	if (executable == NULL) {
		printf("Unable to open file %s\n", name);
		return;
	}*/
	AddrSpace *space; 
		space = new AddrSpace(name);

		currentThread->space = space;
		//processTable.insert(space, (new ProcessTableEntry(space)));
		ProcessTable->addProcess(space);
		

		space->InitRegisters();   // set the initial register values
		space->RestoreState();    // load page table register

		execLock.Acquire();
		execCalled--;
		execLock.Release();
		(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
		machine->Run();     // jump to the user progam
		ASSERT(FALSE);      // machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"*/
}

SpaceId Exec_Syscall(unsigned int vaddr, int len){
		execLock.Acquire();
		execCalled++;
		execLock.Release();
		DEBUG('e', "In exec syscall. vaddr: %i, len: %i\n", vaddr, len);

		char *buf;   // Kernel buffer
		
		if ( !(buf = new char[len]) ) {
			printf("%s","Error allocating kernel buffer for write!\n");
			return -1;
		} else {
				if ( copyin(vaddr,len,buf) == -1 ) {
					printf("%s","Bad pointer passed to to Exec: Exec aborted.\n");
					delete[] buf;
					return -1;
				}
		}

		//string name(buf);

		DEBUG('e' ,"The filename: %s\n", buf);
		//DEBUG('e' ,"Or as a string: %s\n", name.c_str());

		Thread* t;
		t = new Thread("Execed Thread.");
		t->Fork((VoidFunctionPtr)kernel_exec, (int)buf);

	return -1;
}



/*************************************************************************
* Exit()
*************************************************************************/
void Exit_Syscall(int status){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	DEBUG('e', "Exit: currentThread->space: %i\n", currentThread->space);
	ProcessTableEntry* p = ProcessTable->getProcessEntry(currentThread->space);
	execLock.Acquire();
	kernel_threadLock.Acquire();//TODO: RACE CONDITION THIS WONT WORK!!!?

	//Case 1
		//Not last thread in process
		//reclaim 8 pages of stack
		//vpn,ppn,valid
		//memoryBitMap->Clear(ppn)
		//valid = false
	if(p->getNumThreads() > 1){
		p->removeThread();
		DEBUG('E', "Not the last thread. Left: %i \n", p->getNumThreads());
		currentThread->space->Exit();
	}
	//Case 2
		//Last executing thread in last process
		//interupt->Halt();//shut downs nachos
	else if(p->getNumThreads() == 1 && ProcessTable->getNumProcesses() == 1 && !execCalled){
		DEBUG('E', "LAST THREAD LAST PROCESS\n");
		#ifdef USE_TLB
			printf("\nExit status: %i\n\n", status);
		#endif
		interrupt->Halt();
	}
	//Case 3
		//Last executing thread in a process - not last process
		//reclaim all unreclaimed memory
		//for(pageTable)
			//if valid clear
		//locks/cvs match addspace* w/ process table

	//Minumum this must have
	else{
		DEBUG('E', "Last thread in process.\n");
		ProcessTable->deleteProcess(currentThread->space);
		delete currentThread->space;
	}
	execLock.Release();
	kernel_threadLock.Release();

	#ifdef USE_TLB
	if(status != 0){
		#ifdef NETWORK
			printf("\nExit ThreadID: %i status: %i\n\n", currentThread->getThreadID(), status);
		#endif
		#ifndef NETWORK
			printf("\nExit status: %i\n\n", status);
		#endif
	}
	#endif
	(void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
	currentThread->Finish();
}


/*************************************************************
//Prints and int
**************************************************************/
void PrintInt_Syscall(int wat){
	printf("%i", wat);
	fflush(stdout);
}

/*************************************************************
//Prints a String
**************************************************************/
void PrintString_Syscall(unsigned int vaddr, int len){
	char *buf;		// Kernel buffer for output
		
		
	if ( !(buf = new char[len]) ) {
		printf("%s","Error allocating kernel buffer for write!\n");
		return;
	} else {
		if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to PrintString: data not pinted.\n");
			delete[] buf;
			return;
		}
	}

	for (int ii=0; ii<len; ii++) {
		printf("%c",buf[ii]);
	}
	fflush(stdout);
	delete[] buf;
}//End PrintStringSyscall



int Rand_Syscall(){
	return rand();
}


void Sleep_Syscall(int sec){
	Delay(sec);
}

//len is the length of the vaddr string curently in use...the char* had better be large enough to hold the integer as well...
void StringConcatInt_Syscall(unsigned int vaddr, int len, int concat) {
	char *buf;		// Kernel buffer for input
	string temp;
		
	if ( !(buf = new char[len + 2]) ) {
		printf("%s","Error allocating kernel buffer in StringConcatInt\n");
		return;
	} else {
		if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed in StringConcatInt: data not pinted.\n");
			delete[] buf;
			return;
		}
	}

	for(int ii = 0; ii < len; ii++){
		temp += buf[ii];
	}
	
	temp += concat;
	len = sizeof(temp);

	if ( copyout(vaddr, len, buf) == -1 ) {
		printf("%s","Bad pointer passed in StringConcatInt: data not copied\n");
	}
	delete[] buf;
} 
















#ifndef NETWORK

///////////////////////////////////////////////////
////////////////////////////////////////////////////
//NO NETWORKING

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//	Lock Syscalls
///////////////////////////////////////////////////////////////////////////////////////////
class LockTableEntry{
public:
	Lock* lock;
	AddrSpace* space;
	bool isToBeDeleted;
};
#define lockTableSize 200
BitMap lockTableBitMap(lockTableSize);
LockTableEntry* lockTable[lockTableSize];


bool Lock_Syscall_InputValidation(int lock){
	if(lock < 0 || lock >= lockTableSize){
		printf("Invalid Lock Identifier: %i ", lock);
		return FALSE;
	}

	LockTableEntry* lockEntry = lockTable[lock];

	if(lockEntry == NULL){
		printf("Lock %i does not exist. ", lock);
		return FALSE;
	}
	if(lockEntry->space != currentThread->space){
		printf("Lock %i does not belong to this process. ", lock);
		return FALSE;
	}
	return TRUE;
}


///////////////////////////////
// Creates the Lock
///////////////////////////////
int CreateLock_Syscall(int a, int b){
	DEBUG('L', "In CreateLock_Syscall\n");

	int lockTableIndex = lockTableBitMap.Find();
	if(lockTableIndex == -1){
		printf("Max Number of Locks created. Unable to CreateLock\n");
		return -1;
	}

	LockTableEntry* te = new LockTableEntry();
	te->lock = new Lock("Lock " + lockTableIndex);
	te->space = currentThread->space;
	te->isToBeDeleted = FALSE;

	lockTable[lockTableIndex] = te;

	return lockTableIndex;
}


/***********************
*	Acquire the lock
*/
void Acquire_Syscall(int lock){
	DEBUG('L', "In Acquire_Syscall\n");

	if(!Lock_Syscall_InputValidation(lock)){
	 printf("Unable to Acquire.\n");
	 return;
	}

	DEBUG('L', "Acquiring lock.\n");
	lockTable[lock]->lock->Acquire();

}

/*****************
* 	Release the lock
*/
void Release_Syscall(int lock){
	DEBUG('L', "In Release_Syscall\n");

	if(!Lock_Syscall_InputValidation(lock)){
	 printf("Unable to Release.\n");
	 return;
	}
	
	LockTableEntry* le = lockTable[lock];

	DEBUG('L', "Releasing lock.\n");
	le->lock->Release();
	
	if(le->isToBeDeleted && !(le->lock->isBusy()) ){
		DEBUG('L', "Lock %i no longer busy. Deleting.\n", lock);
		delete le->lock;
		le->lock = NULL;
		delete le;
		lockTable[lock] = NULL;
		lockTableBitMap.Clear(lock);
	}

}

void DestroyLock_Syscall(int lock){
	DEBUG('L', "In DestroyLock_Syscall\n");

	if(!Lock_Syscall_InputValidation(lock)){
	 printf("Unable to DestroyLock.\n");
	 return;
	}

	LockTableEntry* le = lockTable[lock];

	if((le->lock->isBusy()) ){
		le->isToBeDeleted = TRUE;
		DEBUG('L', "Lock %i BUSY marking for deletion.\n", lock);
	}else{
		delete le->lock;
		le->lock = NULL;
		delete le;
		lockTable[lock] = NULL;
		lockTableBitMap.Clear(lock);
		DEBUG('L', "Lock %i deleted.\n", lock);
	}


}



//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//	Condition Syscalls
///////////////////////////////////////////////////////////////////////////////////////////

class ConditionTableEntry{
public:
	Condition* condition;
	AddrSpace* space;
	bool isToBeDeleted;
};

#define ConditionTableSize 200
BitMap ConditionTableBitMap(ConditionTableSize);
ConditionTableEntry* ConditionTable[ConditionTableSize];


bool Condition_Syscall_InputValidation(int cond, int lock){

	if(!Lock_Syscall_InputValidation(lock)){
	 return FALSE;
	}

	if(cond < 0 || cond >= ConditionTableSize){
		printf("Invalid Condition Identifier: %i ", cond);
		return FALSE;
	}

	ConditionTableEntry* condEntry = ConditionTable[cond];
	if(condEntry == NULL){
		printf("Condition %i does not exist. ", cond);
		return FALSE;
	}
	if(condEntry->space != currentThread->space){
		printf("Lock %i does not belong to this process. ", cond);
		return FALSE;
	}
	return TRUE;
}



int CreateCondition_Syscall(int a, int b){
	DEBUG('C', "In CreateCondition_Syscall\n");
	
	int conID = ConditionTableBitMap.Find();
	if(conID == -1){
		printf("Max Number of Conditions created. Unable to CreateCondition\n");
		return -1;
	}

	ConditionTableEntry* ce = new ConditionTableEntry();

	ce->condition = new Condition("Condition " + conID);
	ce->space = currentThread->space;
	ce->isToBeDeleted = FALSE;

	ConditionTable[conID]	= ce;

	return conID;
}

void Wait_Syscall(int condition, int lock){
	DEBUG('C', "In Wait_Syscall\n");

	if(!Condition_Syscall_InputValidation(condition, lock)){
		printf("Unable to Wait.\n");
		return;
	}

	ConditionTableEntry* ce = ConditionTable[condition];
	LockTableEntry* le = lockTable[lock];

	ce->condition->Wait(le->lock);
}

void Signal_Syscall(int condition, int lock){
	DEBUG('C', "In Signal_Syscall\n");

	if(!Condition_Syscall_InputValidation(condition, lock)){
		printf("Unable to Signal.\n");
		return;
	}

	ConditionTableEntry* ce = ConditionTable[condition];
	LockTableEntry* le = lockTable[lock];

	ce->condition->Signal(le->lock);

	if(ce->isToBeDeleted && !ce->condition->isBusy()){
		DEBUG('C', "Condition %i no longer BUSY. Deleting.", condition);
		ConditionTable[condition] = NULL;
		delete ce->condition;
		ce->condition = NULL;
		delete ce;
		ConditionTableBitMap.Clear(condition);
	}

}

void Broadcast_Syscall(int condition, int lock){
	DEBUG('C', "In Broadcast_Syscall\n");

	if(!Condition_Syscall_InputValidation(condition, lock)){
		printf("Unable to Broadcast.\n");
		return;
	}

	ConditionTableEntry* ce = ConditionTable[condition];
	LockTableEntry* le = lockTable[lock];

	ce->condition->Broadcast(le->lock);


	if(ce->isToBeDeleted && !ce->condition->isBusy()){
		DEBUG('C', "Condition %i no longer BUSY. Deleting.", condition);
		ConditionTable[condition] = NULL;
		delete ce->condition;
		ce->condition = NULL;
		delete ce;
		ConditionTableBitMap.Clear(condition);
	}
}

void DestroyCondition_Syscall(int condition){
	DEBUG('C', "In DestroyCondition_Syscall\n");

	ConditionTableEntry* ce = ConditionTable[condition];

	if((ce->condition->isBusy()) ){
		ce->isToBeDeleted = TRUE;
		DEBUG('C', "Condition %i BUSY marking for deletion.\n", condition);
	}else{
		ConditionTable[condition] = NULL;
		delete ce->condition;
		ce->condition = NULL;
		delete ce;
		ConditionTableBitMap.Clear(condition);
		DEBUG('C', "Condition %i deleted.\n", condition);
	}

}



//MVs require networking...just some emtpty stubs then...
int CreateMV_Syscall(unsigned int vaddr, int len, int size){
	return -1;
}
void DestroyMV_Syscall(int MVID){
	return;
}
void Set_Syscall(int MVID, int index, int value){
	return;
}
int Get_Syscall(int MVID, int index){
	DEBUG('V', "\n\nGET SYSCALL: MVID: %i INDEX: %i\n\n", MVID, index);
	return 0;
}



#endif
//End for lock CV without networking.




































































//These are the syscalls used for NETWORK
#ifdef NETWORK


//Utility function to send a message to the server.
void clientSendMail(char* msg){
	PacketHeader outPktHdr;
    MailHeader outMailHdr;
    outMailHdr.from = currentThread->getThreadID();
    DEBUG('N', "ThreadID: %i, outMailHdr.from: %i\n",currentThread->getThreadID(), outMailHdr.from);
    
	outMailHdr.to = 0;
	outMailHdr.length = strlen(msg) + 1;
	bool success;

	int trys = 0;
	do{
		trys++;
		outPktHdr.to = rand() % 1;
 		success = postOffice->Send(outPktHdr, outMailHdr, msg);
	}while(!success && trys < 500);


	if(!success){
		printf("ERROR: Failed to send message to the server.\n");
		interrupt->Halt();
		ASSERT(FALSE);
	}
}



//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//	Lock Syscalls
///////////////////////////////////////////////////////////////////////////////////////////
#define MaxNameLen 20

///////////////////////////////
// Creates the Lock
///////////////////////////////
int CreateLock_Syscall(unsigned int vaddr, int len){
	DEBUG('L', "In CreateLock_Syscall\n");
	char *buf;		// Kernel buffer for input
	char buffer[MaxMailSize];
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    outMailHdr.from = currentThread->getThreadID();

    if(len > MaxNameLen){
    	printf("LockName greater than MaxNameLen %i. Use a shorter name.\n", MaxNameLen);
    	return -1;
    }
	
	if ( !(buf = new char[len]) ) {
		printf("%s","Error allocating kernel buffer!\n");
		return -1;
	} else {
		if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to CreateLock.\n");
			delete[] buf;
			return -1;
		}
	}

	string name = "";
	for(int i = 0; i < len; i++){
		if(buf[i] == ' '){
			printf("Invalid name passed to CreateLock. Aborting...\n");
			ASSERT(FALSE);
		}
		name += buf[i];
	}
	delete[] buf;

	stringstream ss;

	ss << SC_CreateLock;
	ss << " ";
	ss << name;

	char *msg = (char*) ss.str().c_str();
	outPktHdr.to = 0;
	outMailHdr.to = 0;
	outMailHdr.length = strlen(msg) + 1;
	bool success = postOffice->Send(outPktHdr, outMailHdr, msg);
	if(!success){
		printf("Failed to send message!!?!!\n");
		interrupt->Halt();
	}

	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;

	rs >> success;
	int lockID;
	if(success){
		rs >> lockID;
	}else{
		printf("CreateLock failure.?\n");
		return -1;
	}


	return lockID;
}


/***********************
*	Acquire the lock
*/
void Acquire_Syscall(int lock){
	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

    if(lock < 0){
    	printf("Invalid LockID %i. Unable to acquire.\n", lock);
    	return;
    }

	stringstream ss;

	ss << SC_Acquire;
	ss << " ";
	ss << lock;

	char *msg = (char*) ss.str().c_str();
	
	clientSendMail(msg);

	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;

	bool success;
	rs >> success;
	if(!success){
		printf("Acquire Error: Unable to Acquire Lock.\n");
	}

	return;
}

/*****************
* 	Release the lock
*/
void Release_Syscall(int lock){
	DEBUG('L', "In Release_Syscall\n");

	if(lock < 0){
    	printf("Invalid LockID %i. Unable to release.\n", lock);
    	return;
    }

	stringstream ss;
	ss << SC_Release;
	ss << " ";
	ss << lock;
	
	clientSendMail( (char*)ss.str().c_str() );

}

void DestroyLock_Syscall(int lock){
	DEBUG('L', "In DestroyLock_Syscall\n");

	if(lock < 0){
    	printf("Invalid LockID %i. Unable to destroy.\n", lock);
    	return;
    }

	stringstream ss;
	ss << SC_DestroyLock;
	ss << " ";
	ss << lock;
	
	clientSendMail( (char*)ss.str().c_str() );
}



//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//	Condition Syscalls
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
class ConditionTableEntry{
public:
	Condition* condition;
	AddrSpace* space;
	bool isToBeDeleted;
};

#define ConditionTableSize 200
BitMap ConditionTableBitMap(ConditionTableSize);
ConditionTableEntry* ConditionTable[ConditionTableSize];


bool Condition_Syscall_InputValidation(int cond, int lock){

	//if(!Lock_Syscall_InputValidation(lock)){
	// return FALSE;
	//}

	if(cond < 0 || cond >= ConditionTableSize){
		printf("Invalid Condition Identifier: %i ", cond);
		return FALSE;
	}

	ConditionTableEntry* condEntry = ConditionTable[cond];
	if(condEntry == NULL){
		printf("Condition %i does not exist. ", cond);
		return FALSE;
	}
	if(condEntry->space != currentThread->space){
		printf("Lock %i does not belong to this process. ", cond);
		return FALSE;
	}
	return TRUE;
}



int CreateCondition_Syscall(int vaddr, int len){
	DEBUG('C', "In CreateCondition_Syscall\n");
	
	char *buf;		// Kernel buffer for input
	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

    if(len > MaxNameLen){
    	printf("CV Name greater than MaxNameLen %i. Use a shorter name.\n", MaxNameLen);
    	return -1;
    }
	
	if ( !(buf = new char[len]) ) {
		printf("%s","Error allocating kernel buffer!\n");
		return -1;
	} else {
		if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to CreateCondition.\n");
			delete[] buf;
			return -1;
		}
	}

	string name = "";
	for(int i = 0; i < len; i++){
		if(buf[i] == ' '){
			printf("Invalid name passed to CreateCondition. Aborting...\n");
			ASSERT(FALSE);
		}
		name += buf[i];
	}
	delete[] buf;

	stringstream ss;

	ss << SC_CreateCondition;
	ss << " ";
	ss << name;

	char *msg = (char*) ss.str().c_str();
	
	clientSendMail(msg);

	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;

	bool success;
	rs >> success;
	int CVID;
	if(success){
		rs >> CVID;
	}else{
		return -1;
	}


	return CVID;
}

void Wait_Syscall(int condition, int lock){
	DEBUG('C', "In Wait_Syscall\n");

	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

	if(condition < 0 || lock < 0){
		printf("Bad condition or lock IDs. Unable to Wait.\n");
		return;
	}


	stringstream ss;
	ss << SC_Wait;
	ss << " ";
	ss << condition;
	ss << " ";
	ss << lock;

	clientSendMail((char*)ss.str().c_str());


	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;
	
	bool status;
	rs >> status;
	if(!status){
		printf("Error during Wait_Syscall.\n");
	}

	return;
}

void Signal_Syscall(int condition, int lock){
	DEBUG('C', "In Signal_Syscall\n");

	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

	if(condition < 0 || lock < 0){
		printf("Bad condition or lock IDs. Unable to Signal.\n");
		return;
	}


	stringstream ss;
	ss << SC_Signal;
	ss << " ";
	ss << condition;
	ss << " ";
	ss << lock;

	clientSendMail((char*)ss.str().c_str());


	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;
	
	bool status;
	rs >> status;
	if(!status){
		printf("Error during Signal_Syscall.\n");
	}
	
	return;
}

void Broadcast_Syscall(int condition, int lock){
	DEBUG('C', "In Broadcast_Syscall\n");

	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

	if(condition < 0 || lock < 0){
		printf("Bad condition or lock IDs. Unable to Broadcast.\n");
		return;
	}


	stringstream ss;
	ss << SC_Broadcast;
	ss << " ";
	ss << condition;
	ss << " ";
	ss << lock;

	clientSendMail((char*)ss.str().c_str());


	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;
	
	bool status;
	rs >> status;
	if(!status){
		printf("Error during Broadcast_Syscall.\n");
	}
	
	return;
}

void DestroyCondition_Syscall(int condition){
	DEBUG('C', "In DestroyCondition_Syscall\n");

	if(condition < 0){
		printf("Bad condition ID. Unable to Destroy.\n");
		return;
	}


	stringstream ss;
	ss << SC_DestroyCondition;
	ss << " ";
	ss << condition;

	clientSendMail((char*)ss.str().c_str());
	return;
}




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


int CreateMV_Syscall(unsigned int vaddr, int len, int size){
	char *buf;		// Kernel buffer for input
	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

    if(len > MaxNameLen){
    	printf("MV Name greater than MaxNameLen %i. Use a shorter name.\n", MaxNameLen);
    	return -1;
    }
	
	if ( !(buf = new char[len]) ) {
		printf("%s","Error allocating kernel buffer!\n");
		return -1;
	} else {
		if ( copyin(vaddr,len,buf) == -1 ) {
			printf("%s","Bad pointer passed to to CreateMV.\n");
			delete[] buf;
			return -1;
		}
	}

	string name = "";
	for(int i = 0; i < len; i++){
		if(buf[i] == ' '){
			printf("Invalid name passed to CreateMV. Aborting...\n");
			ASSERT(FALSE);
		}
		name += buf[i];
	}
	delete[] buf;

	if(size <= 0){
		printf("Invalid size passed to CreateMV. Size must be greater than 0.\n");
		return -1;
	}

	stringstream ss;

	ss << SC_CreateMV << " " << size << " " << name;
	//ss << name;

	DEBUG('V', "\n\nCREATE MSG: %s\n\n", ss.str().c_str());

	clientSendMail((char*) ss.str().c_str());

	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;

	bool success;
	rs >> success;
	int MVID;
	if(success){
		rs >> MVID;
	}else{
		printf("Error...Failed to create MV.\n");
		return -1;
	}

	return MVID;
}

void DestroyMV_Syscall(int MVID){
	if(MVID < 0){
		printf("Bad MVID. Unable to Destroy.\n");
		return;
	}

	stringstream ss;
	ss << SC_DestroyMV;
	ss << " ";
	ss << MVID;

	clientSendMail((char*)ss.str().c_str());
	return;
}


void Set_Syscall(int MVID, int index, int value){
	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;

	if(MVID < 0){
		printf("Bad MVID. Unable to Set.\n");
		return;
	}
	if(index < 0){
		printf("Bad MV index passed to Set. Unable to Set.\n");
		return;
	}


	stringstream ss;
	ss << SC_Set << " " << MVID << " " << index << " " << value;

	clientSendMail((char*)ss.str().c_str());

	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;
	bool success;
	rs >> success;

	if(!success){
		printf("Error during MV Set. Unable to Set.\n");
	}

	return;
}

int Get_Syscall(int MVID, int index){
	DEBUG('V', "\n\nGET SYSCALL: MVID: %i INDEX: %i\n\n", MVID, index);
	char buffer[MaxMailSize];
	PacketHeader inPktHdr;
    MailHeader inMailHdr;


	if(MVID < 0){
		printf("Bad MVID. Unable to Get.\n");
		return 0;
	}
	if(index < 0){
		printf("Bad MV index passed to Get. Unable to Get.\n");
		return 0;
	}


	stringstream ss;
	ss << SC_Get;
	ss << " ";
	ss << MVID;
	ss << " ";
	ss << index;

	clientSendMail((char*)ss.str().c_str());


	int value = 0;
	bool success;
	
	postOffice->Receive(currentThread->getThreadID(), &inPktHdr, &inMailHdr, buffer);

	stringstream rs;
	rs << buffer;
	
	rs >> success;

	if(!success){
		printf("Error during MV Get. Unable to Get.\n");
		return value;
	}

	rs >> value;

	return value;
}




#endif
//End for with NETWORK








































/////////////////////////////////////////////////////////////////
// Demand Paged Virtual Memory
//
//	////		////		//		 //		///		   ///
//	//	//		// //		 //		//		// /	  /	//
//	//	//		////		  //   //		//  //	//	//
//	//  //		//			   // //		//   ////	//
//	////		//				//			//	  //	//
/////////////////////////////////////////////////////////////////

////////////////////////////////////
int currentTLB = 0; //To keep track of which TLB entry to replace






/////////////////
//If TLB valid and dirty propogate changes to IPT
void propogateCurTLBDirty(int tlbIndex){
	TranslationEntry* curTLB;
	curTLB = &(machine->tlb[tlbIndex]);
	if(curTLB->valid && curTLB->dirty){
		//Propogate dirty bit to IPT
		IPT[curTLB->physicalPage].dirty = TRUE;
	}
}


////////////////////
// populate TLB from a given IPT entry
// first propogate changes
void populateTLBFromIPTEntry(int ppn){

	propogateCurTLBDirty(currentTLB);

	machine->tlb[currentTLB].virtualPage = IPT[ppn].virtualPage;
	machine->tlb[currentTLB].physicalPage = IPT[ppn].physicalPage;
	machine->tlb[currentTLB].valid = IPT[ppn].valid;
	machine->tlb[currentTLB].use = IPT[ppn].use;
	machine->tlb[currentTLB].dirty = IPT[ppn].dirty;
	machine->tlb[currentTLB].readOnly = IPT[ppn].readOnly;

	currentTLB = (currentTLB + 1) % TLBSize;
	DEBUG('T', "Populated TLB[%i] with IPT[%i]\n", currentTLB, ppn);
}//End pupulateTLB


//////////////
int writePageToSwap(int ppn){
	int byteOffset = -1;
	if(IPT[ppn].valid && IPT[ppn].dirty){
		int swapIndex = swapBitMap->Find();
		if(swapIndex == -1){
			printf("FATAL ERROR: SwapFile Full. Please increase the size of the swap file.\n");
			interrupt->Halt();
		}
		
		byteOffset = swapIndex * PageSize;

		swapFile->WriteAt(&(machine->mainMemory[PageSize * ppn]), PageSize, byteOffset);
		DEBUG('S', "WritePageToSwap: Wrote space %i VPN %i from PPN %i to SWAP page %i\n",IPT[ppn].PID, IPT[ppn].virtualPage, ppn, swapIndex);
	}
	return byteOffset;
}
//////////////
void readPageFromSwapToPPN(int byteOffset, int ppn){
	int swapIndex = byteOffset / PageSize;
	swapBitMap->Clear(swapIndex);
	swapFile->ReadAt(&(machine->mainMemory[PageSize * ppn]), PageSize, byteOffset);
	 DEBUG('S', "readPageFromSwapToPPN swap %i to ppn %i\n", swapIndex, ppn);
}



/////////////
void removePageFromFIFOQ(int ppn){
	//Remove this page from the eviction Q
	for(unsigned int i = 0; i < FIFOList.size(); i++){
		if(FIFOList[i] == ppn){
			FIFOList.erase(FIFOList.begin()+i);
			return;
		}
	}
	printf("This should never be printed. Because this ppn should be in the Q if its in memory.\n");
}



///////////////////
// handleMemoryFull
int handleMemoryFull(){
	//Memory is full select a page to evict...
	int ppn = -1;
	if(USEFIFO){
		DEBUG('M' ,"handleMemoryFull: Using %s replacement policy.\n", "FIFO");
		ppn = FIFOList[0];
	}else{
		DEBUG('M' ,"handleMemoryFull: Using %s replacement policy.\n", "RAND");
		ppn = rand() % NumPhysPages;	//Random page to evict
	}
	ASSERT(ppn >= 0 && ppn < NumPhysPages);
	ASSERT(IPT[ppn].valid);

	DEBUG('M' ,"handleMemoryFull: Going to evict page %i.\n", ppn);

	//Check if this IPT/memory entry is in the TLB and invalidate it / propogate changes to IPT
	for(int i = 0; i < TLBSize; i++){
		if(machine->tlb[i].valid && machine->tlb[i].physicalPage == ppn ){
			DEBUG('M' ,"handleMemoryFull: TLB[%i] matched page %i. ", i, ppn);
			if(machine->tlb[i].dirty){
				IPT[ppn].dirty = TRUE;
				DEBUG('M' ,"TLB[%i] dirty.", i);
			}
			machine->tlb[i].valid = FALSE;
			DEBUG('M' ,"\n");
			break;
		}
	}
	//TLB changes have been propogated to IPT
	DEBUG('M', "handleMemoryFull: TLB checked.\n");
	///If dirty write to swap & update pageTable for that page
	if(IPT[ppn].dirty && IPT[ppn].valid){
		DEBUG('M' ,"handleMemoryFull: IPT[%i] dirty. Writing to SWAP...\n", ppn);
		AddrSpace* space = IPT[ppn].PID;
		space->pageTable[IPT[ppn].virtualPage].location = SWAP;
		space->pageTable[IPT[ppn].virtualPage].byteOffset = writePageToSwap(ppn);
		space->pageTable[IPT[ppn].virtualPage].dirty = IPT[ppn].dirty;

	}else{//If !dirty update pageTable...no need to save
		DEBUG('M' ,"handleMemoryFull: IPT[%i] NOT dirty. Let's say the page is in EXEC.\n", ppn);
		AddrSpace* space = IPT[ppn].PID;
		DEBUG('M', "handleMemoryFull: PID: %i PPN: %i VPN: %i\n", space, ppn, IPT[ppn].virtualPage);
		space->pageTable[ IPT[ppn].virtualPage ].location = EXEC;
		//If its going to exec it was always in exec...
			//no need to update byteOffset...and its not dirty so don't need to change anything else.
	}
	IPT[ppn].valid = FALSE;
	DEBUG('M' ,"Evicted page %i.\n", ppn);

	removePageFromFIFOQ(ppn);

	return ppn;
}//end handleMemoryFull




int handleIPTmiss(int vpn){
	DEBUG('T', "IPT miss vpn %i\n", vpn);
	int ppn = pageTableBitMap->Find();	//The PPN of an unused page.
	AddrSpace* space = currentThread->space;

	if(ppn == -1){
		//Main Memory (and IPT) are full...need to evict a page.
		ppn = handleMemoryFull();
		DEBUG('P', "IPT miss vpn %i memory full. Evicted page %i.\n", vpn, ppn);
	}

	
	//Assumed ppn is a free page in memory and IPT is !valid and changes have been propogated from TLB
	if(space->pageTable[vpn].location == EXEC){//Read page from executable
		DEBUG('P', "IPT miss vpn %i reading from executable.\n", vpn);
		space->executable->ReadAt( &(machine->mainMemory[PageSize * ppn]), PageSize, space->pageTable[vpn].byteOffset );
	}else if(space->pageTable[vpn].location == SWAP){ //Its in the swap...
		//Read from swap to mainmemory
		DEBUG('P', "IPT miss vpn %i reading from swap.\n", vpn);
		readPageFromSwapToPPN(space->pageTable[vpn].byteOffset, ppn);
		DEBUG('S', "IPTMiss for space %i vpn %i to ppn %i\n",space, vpn, ppn);
		//clear swapFileBitmap
	}else if(space->pageTable[vpn].location == VOID){
		DEBUG('P', "IPT miss vpn %i its nowhere.\n", vpn);
		//its nowhere...don't need to do anything...
	}

	//DEBUG('P', "IPT miss vpn %i updating pageTable.\n", vpn);
   	space->pageTable[vpn].physicalPage = ppn;
    space->pageTable[vpn].valid = TRUE;
    space->pageTable[vpn].location = MAIN;
        
	//DEBUG('P', "IPT miss vpn %i updating IPT.\n", vpn);
    //Populate IPT
	IPT[ppn] = space->pageTable[vpn];
    IPT[ppn].PID = space;

    //Add this memory page to back of the eviction Q
    FIFOList.push_back(ppn);

    return ppn;
}





//Handles a TLB miss / PageFaultException
void handleTLBMiss(){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	unsigned int VA = machine->ReadRegister(BadVAddrReg);
	int VP = VA / PageSize;
	DEBUG('T', "TLB Miss: Need virtual address %i in virtual page %i for AddrSpace: %i\n", VA, VP, currentThread->space);

	
	

	//machine->tlb[currentTLB] = currentThread->space->getPageTableEntry(VP);

	//Populate TLB from IPT
	int ppn = -1;
	//Look for the needed VP in the IPT
	for(int i = 0; i < NumPhysPages; i++){
		if(IPT[i].valid == TRUE && IPT[i].PID == currentThread->space && IPT[i].virtualPage == VP){
			//Found needed entry in IPT
			ppn = i;
			break;
		}
	}

	if(ppn == -1){
		//IPT miss
		ppn = handleIPTmiss(VP);
		if(ppn == -1){printf("IPT miss failed!!\n"); ASSERT(FALSE);}
		else{DEBUG('P', "IPTmiss returned ppn %i\n", ppn);}
	}

	//Populate TLB
	populateTLBFromIPTEntry(ppn);

	DEBUG('T', "TLB Miss: Completed.\n");
	(void) interrupt->SetLevel(oldLevel);   // restore interrupts
}//End TLB miss








/*//Print the contents of the TLB...for debugging purposes...
	if(FALSE){
		for(int i = 0; i < 4; i++){
			printf("\nTLB[%i]\n", i);
			printf("\t virtualPage: %i\n", machine->tlb[i].virtualPage);
			printf("\t physicalPage: %i\n", machine->tlb[i].physicalPage);
			printf("\t Valid: %i\n", machine->tlb[i].valid);
			printf("\t use: %i\n", machine->tlb[i].use);
			printf("\t dirty: %i\n", machine->tlb[i].dirty);
			printf("\t readOnly: %i\n", machine->tlb[i].readOnly);
		}
	}//End print TLB contents...
*/



























void ExceptionHandler(ExceptionType which) {
		int type = machine->ReadRegister(2); // Which syscall?
		int rv=0; 	// the return value from a syscall

		if ( which == SyscallException ) {
	switch (type) {
			default:
		DEBUG('a', "Unknown syscall - shutting down.\n");
			case SC_Halt:
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
		break;
			case SC_Create:
		DEBUG('a', "Create syscall.\n");
		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
			case SC_Open:
		DEBUG('a', "Open syscall.\n");
		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
			case SC_Write:
		DEBUG('a', "Write syscall.\n");
		Write_Syscall(machine->ReadRegister(4),
						machine->ReadRegister(5),
						machine->ReadRegister(6));
		break;
			case SC_Read:
		DEBUG('a', "Read syscall.\n");
		rv = Read_Syscall(machine->ReadRegister(4),
						machine->ReadRegister(5),
						machine->ReadRegister(6));
		break;
			case SC_Close:
		DEBUG('a', "Close syscall.\n");
		Close_Syscall(machine->ReadRegister(4));
		break;
			
		case SC_Fork:
			DEBUG('a', "Fork syscall.\n");
			Fork_Syscall(machine->ReadRegister(4));
		break;

		case SC_Yield:
			DEBUG('a', "Yield syscall.\n");
			currentThread->Yield();
		break;

		case SC_PrintInt:
			DEBUG('a', "PrintInt syscall.\n");
			PrintInt_Syscall(machine->ReadRegister(4));
		break;
		
		case SC_PrintString:
			DEBUG('a', "PrintString syscall.\n");
			DEBUG('N', "\nTHREAD ID: %i: ", currentThread->getThreadID() );
			PrintString_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		
		case SC_Exit:
			DEBUG('a', "Exit syscall.\n");
			Exit_Syscall(machine->ReadRegister(4));
		break;

		case SC_Exec:
			DEBUG('a', "Exec syscall.\n");
			rv = Exec_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_CreateLock:
			DEBUG('a', "CreateLock syscall.\n");
			rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_Acquire:
			DEBUG('a', "Acquire syscall.\n");
			Acquire_Syscall(machine->ReadRegister(4));
		break;

		case SC_Release:
			DEBUG('a', "Release syscall.\n");
			Release_Syscall(machine->ReadRegister(4));
		break;

		case SC_DestroyLock:
			DEBUG('a', "DestroyLock syscall.\n");
			DestroyLock_Syscall(machine->ReadRegister(4));
		break;

		case SC_CreateCondition:
			DEBUG('a', "CreateCondition syscall.\n");
			rv = CreateCondition_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_Wait:
			DEBUG('a', "Wait syscall.\n");
			Wait_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_Signal:
			DEBUG('a', "Signal syscall.\n");
			Signal_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_Broadcast:
			DEBUG('a', "Broadcast syscall.\n");
			Broadcast_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_DestroyCondition:
			DEBUG('a', "DestroyCondition syscall.\n");
			DestroyCondition_Syscall(machine->ReadRegister(4));
		break;

		case SC_Rand:
			DEBUG('a', "Rand syscall.\n");
			rv = Rand_Syscall();
		break;

		case SC_Sleep:
			DEBUG('a', "Sleep/Delay syscall.\n");
			Sleep_Syscall(machine->ReadRegister(4));
		break;

		case SC_StringConcatInt:
			StringConcatInt_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
		break;

		case SC_CreateMV:
			DEBUG('a', "CreateMV syscall.\n");
			rv = CreateMV_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
		break;

		case SC_Get:
			DEBUG('a', "Get syscall.\n");
			rv = Get_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;

		case SC_Set:
			DEBUG('a', "Set syscall.\n");
			Set_Syscall(machine->ReadRegister(4), machine->ReadRegister(5), machine->ReadRegister(6));
		break;

		case SC_DestroyMV:
			DEBUG('a', "DestroyMV syscall.\n");
			DestroyMV_Syscall(machine->ReadRegister(4));
		break;

	}

	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
		} else if(which == PageFaultException){
			DEBUG('T', "PageFaultException\n");
			handleTLBMiss();
			return;
		}else {
			cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
			interrupt->Halt();
		}
}
