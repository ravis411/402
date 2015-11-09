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

  Write(welcomeString, sizeof(welcomeString), ConsoleOutput);

  lock1 = CreateLock("Lock1", sizeof("Lock1") );

  PrintString("Got lockID: ", sizeof("Got lockID: "));
  PrintInt(lock1);
  PrintString("\n", 1);


	PrintString("Done.\n", sizeof("Done.\n"));
  Exit(0);
}