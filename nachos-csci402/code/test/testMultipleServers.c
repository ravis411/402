/* testPageTable.c
 *	Simple program to test Lock syscalls
 */

#include "syscall.h"

char welcomeString[] = "\nLock Syscall test...\n\n";
char string1[] = "String 1...\n";
char string2[] = "String 2...\n";
char string3[] = "String 3...\n";
char string4[] = "String 4...\n";
char string5[] = "String 5...\n";
char string6[] = "String 6...\n";

int lock1;



int main() {
  int lock2;
  int lock3;
  int CV1;
  int i;
  int MV;
  int startCVMVLock;
  int startCVMVLockMV;

  Write(welcomeString, sizeof(welcomeString), ConsoleOutput);

  lock1 = CreateLock("Lock1", sizeof("Lock1") );

  PrintString("Got lockID1: ", sizeof("Got lockID: "));
  PrintInt(lock1);
  PrintString("\n", 1);
  

  Exit(0);
  

  lock2 = CreateLock("Lock2", sizeof("Lock2") );

  PrintString("Got lockID2: ", sizeof("Got lockID: "));
  PrintInt(lock2);
  PrintString("\n", 1);

  lock3 = CreateLock("Lock3", sizeof("Lock3") );

  PrintString("Got lockID3: ", sizeof("Got lockID: "));
  PrintInt(lock3);
  PrintString("\n", 1);

 

  startCVMVLock = CreateLock("startCVMVLock", sizeof("startCVMVLock"));
  /*startCVMVLockMV = CreateMV("startCVMVLockMV", sizeof("startCVMVLockMV"), 1);*/

  Acquire(startCVMVLock);

  PrintString("Acquired a lock.\n", sizeof("Acquired a lock.\n"));

  Release(startCVMVLock);

    /*Delay to give time to start another instance.*/
    Sleep(3);
  

  Acquire(lock1);
  Acquire(lock1); /*We already own this lock */
  


  for(i = 9; i >= 0; i--){
    PrintInt(i);
    PrintString(" ", 1);
    /*Delay*/
    Sleep(1);
  }

  Release(lock1);

  Release(lock2); /*We don't own lock2...*/

  PrintString("\n\nFinished Lock Tests\n\n", sizeof("\n\nFinished Lock Tests\n\n"));




  PrintString("\n\nGoing to test CVs and MVs\n\n", sizeof("\n\nGoing to test CVs and MVs\n\n"));

  CV1 = CreateCondition("CV1", sizeof("CV1"));
  MV = CreateMV("MV1", sizeof("MV1"), 2);

  PrintString("Got CVID: ", sizeof("Got CVID: "));
  PrintInt(CV1);
  PrintString("\n", 1);



  Acquire(startCVMVLock);
  i = Get(startCVMVLockMV, 0); /*This will allow us to count how instances have been started...So we can wait before the CV section.*/
  i--;
  Set(startCVMVLockMV, 0, i);
  if(i == 0){
    PrintString("I was last to finsh waking them all up.\n", sizeof("I was last to finsh waking them all up.\n"));
    /*Wake them all up...*/
    Acquire(lock2);
    Broadcast(CV1, lock2);
    Release(startCVMVLock);
    Release(lock2);
    Acquire(lock2);

  }
  else{
    PrintString("Waiting for everyone to catch up.\n", sizeof("Waiting for everyone to catch up.\n"));
    Acquire(lock2);
    Release(startCVMVLock);
    Wait(CV1, lock2);
  }
  

  /*Acquire CV lock*/
  /*Acquire(lock2);Alreadyacquired.*/

  PrintString("First.\n", sizeof("First.\n"));
  PrintString("\tMV[0]: ", sizeof("\tMV[0]: "));
  i = Get(MV, 0);
  PrintInt(i);
  Set(MV, 0, ++i);
  PrintString(" MV[1]: ", sizeof("MV[1]: "));
  i = Get(MV, 1);
  PrintInt(i);
  Set(MV, 1, --i);
  PrintString("\n", 1);

  Sleep(1);
  Signal(CV1, lock2);
  Wait(CV1, lock2);

  PrintString("Second.\n", sizeof("Second.\n"));
  PrintString("\tMV[0]: ", sizeof("\tMV[0]: "));
  i = Get(MV, 0);
  PrintInt(i);
  Set(MV, 0, ++i);
  PrintString(" MV[1]: ", sizeof("MV[1]: "));
  i = Get(MV, 1);
  PrintInt(i);
  Set(MV, 1, --i);
  PrintString("\n", 1);


  Sleep(1);
  Signal(CV1, lock2);
  Wait(CV1, lock2);

  PrintString("Third.\n", sizeof("Third.\n"));
  PrintString("\tMV[0]: ", sizeof("\tMV[0]: "));
  i = Get(MV, 0);
  PrintInt(i);
  Set(MV, 0, ++i);
  PrintString(" MV[1]: ", sizeof("MV[1]: "));
  i = Get(MV, 1);
  PrintInt(i);
  Set(MV, 1, --i);
  PrintString("\n", 1);
  
  Sleep(1);
  Signal(CV1, lock2);
  Wait(CV1, lock2);
  Broadcast(CV1, lock2);

  Release(lock2);


  DestroyLock(lock1);
  DestroyLock(lock2);
  DestroyCondition(CV1);
  DestroyMV(MV);

	PrintString("Done.\n", sizeof("Done.\n"));
  Exit(0);
}