// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}


/////////////////////////////////////////////////////////////////
//  PageTableEntry
///////////////////////////////////////////////////////////////////
PageTableEntry &PageTableEntry::operator=(const PageTableEntry& entry){
    DEBUG('f', "PageTableEntry assignment opperator.\n");
    if(&entry != this) // check for self assignment
    {
        virtualPage = entry.virtualPage;
        physicalPage = entry.physicalPage;
        valid = entry.valid;
        use = entry.use;
        dirty = entry.dirty;
        readOnly = entry.readOnly;

        location = entry.location;
        byteOffset = entry.byteOffset;
    }
    return *this;
}



//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(char *filename) : fileTable(MaxOpenFiles) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    NoffHeader noffH;
    unsigned int i, size;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    //Open the executable...
    executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        ASSERT(FALSE);//If we can't open the file...we need to quit since this AddrSpace will be broken.
        return;
    }
    DEBUG('e', "AddrSpace filename: %s \n", filename);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    numCodePages = divRoundDown(noffH.code.size, PageSize); //Want to round down to be sure that only code is on the page.
    numNonStackPages = divRoundUp(size, PageSize);
    numPages = numNonStackPages + divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
						// to leave room for the stack
    size = numPages * PageSize;

    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new PageTableEntry[numPages];


    #ifdef THREADTABLE
            ThreadTableEntry* t = new ThreadTableEntry();
            t->threadID = currentThread->getThreadID();
            //vector<ThreadTableEntry*> threadTable;
            threadTable.insert(pair<int, ThreadTableEntry*>(t->threadID, t));
    #endif
    
    for (i = 0; i < numPages; i++) {
        //int ppn = FindPPN();//The PPN of an unused page.

       	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
    	pageTable[i].physicalPage = -1;
    	pageTable[i].valid = FALSE;//TRUE
    	pageTable[i].use = FALSE;
    	pageTable[i].dirty = FALSE;
    	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
        
        

        if(i < numNonStackPages){//Not stack
            pageTable[i].location = EXEC;//If its not stack its in the executable...
            pageTable[i].byteOffset = ( noffH.code.inFileAddr + (i * PageSize) ); //This is where the page is in the executable.
            //executable->ReadAt( &(machine->mainMemory[PageSize * ppn]), PageSize, noffH.code.inFileAddr + (i * PageSize) );
        }else{//Stack
            pageTable[i].location = VOID;//Stack pages are nowhere...
            pageTable[i].byteOffset = -1; //byteOffset is nowhere for stack untill it goes to swap...
            DEBUG('a', "Initializing stack page, vpn: %i\n", i);

            #ifdef THREADTABLE
                DEBUG('E', "Initializing stack page threadtable, vpn: %i, for threadID: %i\n", i, currentThread->getThreadID());
                threadTable[currentThread->getThreadID()]->stackPages.push_back(i);
            #endif

        }
    }

    //We need to remember where this thread's stack is...


// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    //bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    /*
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
    */
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
    DEBUG('a', "Address space, initialized with with threadID: %i\n", currentThread->getThreadID()); 
}//End AddrSpace Constructor


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    //Clear used pages
    for(unsigned int i = 0; i < numPages; i++){
        if(pageTable[i].valid){
            //Clear any valid pages from Memory
            
           if(pageTable[i].location == MAIN){
                DEBUG('E', "~Addrspace: Clearing VPN %i pageTableBitMap and IPT: %i\n", i, pageTable[i].physicalPage);
                DEBUG('S', "~Addrspace %i: Clearing VPN %i pageTableBitMap and IPT: %i\n", currentThread->space, i, pageTable[i].physicalPage);
                pageTableBitMap->Clear(pageTable[i].physicalPage);
                //Remove this page from the eviction Q
                for(int ii = 0; ii < FIFOList.size(); ii++){
                    if(FIFOList[ii] == pageTable[ii].physicalPage){
                        FIFOList.erase(FIFOList.begin()+ii);
                        break;
                    }
                }
                IPT[pageTable[i].physicalPage].valid = FALSE;
           }else if(pageTable[i].location == SWAP){
                int swapIndex = pageTable[i].byteOffset / PageSize;
                DEBUG('E', "~Addrspace: Clearing VPN %i from SWAP page %i.\n", i, swapIndex);
                DEBUG('S', "~Addrspace %i: Clearing VPN %i from SWAP page %i.\n", currentThread->space, i, swapIndex);
                swapBitMap->Clear(swapIndex);
           }
           pageTable[i].valid = FALSE;
        }
    }
    
    //For good measure (or maybe its excessive) save state to invalidate the TLB
    SaveState();
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
    delete pageTable;
    delete executable;
}



/////////////////////////
// FindPPN
///////////////////////
int AddrSpace::FindPPN(){
    int ppn = pageTableBitMap->Find();
    if(ppn == -1){
        printf("Fatal Error: Nachos out of memory. Bitmap retured -1. Aborting...\n");
        interrupt->Halt();
    }
    return ppn;
}


///////////////////////////////////////////////////////////////////
// AddrSpace::Fork()
//
//  Adds 8 pages to pageTable,
//       inits registers for current thread.
//
//////////////////////////////////////////////////////////////////

void
AddrSpace::Fork(int nextInstruction)
{
    DEBUG('f', "In AddrSpace::Fork\n");

    //Should we really disable interrupts?
    // Would be better to acquire a lock but that has to be acquired wherever changes to these values take place...
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

    unsigned int newNumPages = numPages + divRoundUp(UserStackSize,PageSize);
    //ASSERT(newNumPages <= NumPhysPages);       // check we're not trying to run anything too big --

    //copy old table
    PageTableEntry* newPageTable = new PageTableEntry[newNumPages];
    for(unsigned int i = 0; i < numPages; i++){
        (newPageTable[i]) = (pageTable[i]); //Overloaded = operator now does a deep copy
    }
    delete pageTable;
    pageTable = newPageTable;

    #ifdef THREADTABLE
            ThreadTableEntry* t = new ThreadTableEntry();
            t->threadID = currentThread->getThreadID();
            //vector<ThreadTableEntry*> threadTable;
            threadTable.insert(pair<int, ThreadTableEntry*>(t->threadID, t));
    #endif

    //Add 8 pages for stack
    for(unsigned int i = numPages; i < newNumPages; i++){
        //int ppn = FindPPN();
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = FALSE; //TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;

        pageTable[i].location = VOID;   //Stack pages are nowhere. They're in the void.

        #ifdef THREADTABLE
            DEBUG('E', "Initializing stack page threadtable, vpn: %i, for threadID: %i\n", i, currentThread->getThreadID());
            threadTable[currentThread->getThreadID()]->stackPages.push_back(i);
        #endif

        /*************
        * Populate IPT
        ****************/
       // IPT[ppn] = pageTable[i];
       // IPT[ppn].PID = this;
        /***End IPT */

    }

    numPages = newNumPages;
    InitRegisters();
    machine->WriteRegister(PCReg, nextInstruction);
    machine->WriteRegister(NextPCReg, nextInstruction + 4);
    RestoreState();

    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts

    DEBUG('f', "End AddrSpace::Fork\n");
}//End Fork


////////////////////////////////////////////////////////////////////
// Exit()
//
//  Removes the stack for the current thread.
////////////////////////////////////////////////////////////////////
void AddrSpace::Exit(){
   
    unsigned int stackPagesCleared = 0;
    int currentThreadID = currentThread->getThreadID();
    DEBUG('E', "In AddrSpace::Exit for thread %i\n", currentThreadID);
    //Should we really disable interrupts?
    // Would be better to acquire a lock but that has to be acquired wherever changes to these values take place...
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts

    //reclaim 8 pages of stack
    //vpn,ppn,valid
    //memoryBitMap->Clear(ppn)
    //valid = false

    //We need to find where our 8 pages are...This should not be done like this...but whatever for now...?


    #ifdef THREADTABLE
        for(int i = 0; i < 8; i++){//There are 8 pages of stack...hope this dosen't change...
            int vpn = threadTable[currentThread->getThreadID()]->stackPages[i];
            int ppn = pageTable[vpn].physicalPage;
            DEBUG('E', "Clearing stack page, vpn: %i, for threadID: %i\n", vpn, currentThread->getThreadID());

            if(pageTable[vpn].valid){
                //Clear any valid pages from Memory
                if(pageTable[vpn].location == MAIN){
                    DEBUG('E', "Addrspace::Exit: Clearing VPN %i pageTableBitMap and IPT: %i\n",vpn, pageTable[vpn].physicalPage);
                     DEBUG('S', "Addrspace::Exit %i: Clearing VPN %i pageTableBitMap and IPT: %i\n",vpn, pageTable[vpn].physicalPage);
                    pageTableBitMap->Clear(pageTable[vpn].physicalPage);
                    //Remove this page from the eviction Q
                    for(int ii = 0; ii < FIFOList.size(); ii++){
                        if(FIFOList[ii] == ppn){
                            FIFOList.erase(FIFOList.begin()+ii);
                            break;
                        }
                    }
                    IPT[pageTable[vpn].physicalPage].valid = FALSE;
                }else if(pageTable[vpn].location == SWAP){
                    int swapIndex = pageTable[vpn].byteOffset / PageSize;
                    DEBUG('E', "Addrspace::Exit: Clearing VPN %i from SWAP page %i.\n", vpn, swapIndex);
                    DEBUG('S', "~Addrspace::Exit %i: Clearing VPN %i from SWAP page %i.\n", vpn, swapIndex);
                    swapBitMap->Clear(swapIndex);
                }
                pageTable[vpn].valid = FALSE;
            }

            pageTable[vpn].valid = FALSE;
           // pageTableBitMap->Clear( ppn );
            pageTable[vpn].physicalPage = -1;
            stackPagesCleared++;

            /*************
            * Invalidate Entry in IPT
            ****************/
           // IPT[ppn].valid = FALSE;
           // IPT[ppn].PID = NULL;
            /***End IPT */
        }
        
    #endif

    SaveState();//Maybe clear the TLB...should just clear an entry if its there, this is simpler.
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts

    ASSERT( stackPagesCleared == (UserStackSize / PageSize) );
    DEBUG('E', "End AddrSpace::Exit\n");

}//End Exit











PageTableEntry AddrSpace::getPageTableEntry(unsigned int VP){
    ASSERT(VP >= 0 && VP < numPages);
    
    PageTableEntry entry;

    entry = pageTable[VP];
    return entry;
}  












//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    //Invalidate the TLB on a context switch
    for(int i = 0; i < TLBSize; i++){
        if(machine->tlb[i].valid && machine->tlb[i].dirty){
            //Propogate changes...
            IPT[machine->tlb[i].physicalPage].dirty = TRUE;
        }
        machine->tlb[i].valid = FALSE;
    }
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    //machine->pageTable = pageTable;
    //machine->pageTableSize = numPages;
}
