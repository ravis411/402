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
  Acquire(lock1);
  /*Delay to give time to start another instance.*/
  Sleep(3);

  for(i = 0; i < 20; i++){
    PrintInt(i);
    PrintString(" ", 1);
    /*Random Chance of Delay*/
    if(true){
      Sleep(1);
    }
  }

  Release(lock1);

  Release(lock2); /*We don't own lock2...*/

  DestroyLock(lock1);
  DestroyLock(lock2);


	PrintString("Done.\n", sizeof("Done.\n"));
  Exit(0);
}