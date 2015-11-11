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
  int CV1;
  int i;

  Write(welcomeString, sizeof(welcomeString), ConsoleOutput);

  lock1 = CreateLock("Lock1", sizeof("Lock1") );

  PrintString("Got lockID1: ", sizeof("Got lockID: "));
  PrintInt(lock1);
  PrintString("\n", 1);

  lock2 = CreateLock("Lock2", sizeof("Lock2") );

  PrintString("Got lockID2: ", sizeof("Got lockID: "));
  PrintInt(lock2);
  PrintString("\n", 1);

  Acquire(lock1);
  Acquire(lock1); /*We already own this lock */
  /*Delay to give time to start another instance.*/
  Sleep(3);

  for(i = 10; i >= 0; i--){
    PrintInt(i);
    PrintString(" ", 1);
    /*Delay*/
    Sleep(1);
  }

  Release(lock1);

  Release(lock2); /*We don't own lock2...*/

  PrintString("\n\nFinished Lock Tests\n\n", sizeof("\n\nFinished Lock Tests\n\n"));


  PrintString("\n\nGoing to test CVs\n\n", sizeof("\n\nGoing to test CVs\n\n"));

  CV1 = CreateCondition("CV1", sizeof("CV1"));

  PrintString("Got CVID: ", sizeof("Got CVID: "));
  PrintInt(CV1);
  PrintString("\n", 1);


  //Acquire CV lock
  Acquire(lock2);

  PrintString("First.\n", sizeof("First.\n"));
  Sleep(1);
  Signal(CV1, lock2);
  Wait(CV1, lock2);

  PrintString("Second.\n", sizeof("Second.\n"));
  Sleep(1);

  Signal(CV1, lock2);
  Wait(CV1, lock2);

  PrintString("Third.\n", sizeof("Third.\n"));
  Sleep(1);

  Signal(CV1, lock2);
  Wait(CV1, lock2);
  Broadcast(CV1, lock2);


  DestroyLock(lock1);
  DestroyLock(lock2);
  DestroyCondition(CV1);

	PrintString("Done.\n", sizeof("Done.\n"));
  Exit(0);
}