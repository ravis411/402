/*
*
*	proj3TestSuite.c
*
*/

#include "syscall.h"

char welcomeString[] = "\nproj3TestSuite.\n\n";
char matFileName[] = "../test/matmult";
char sortFileName[] = "../test/sort";

int
main()
{
    PrintString(welcomeString, sizeof(welcomeString));

    PrintString("Going to Exec 2 testVirtualMemory.c and 1 sort.c\n", sizeof("Going to Exec 2 testVirtualMemory.c and 1 sort.c\n"));
    PrintString("\ttestVirtualMemory forks 2 matmults.\n", sizeof("\ttestVirtualMemory forks 2 matmults.\n"));
    PrintString("\tExpected output is 8 Exit statuses, 4 7220s and 1 1023\n", sizeof("\tExpected output is 8 Exit statuses, 4 7220s and 1 1023\n"));
    PrintString("\tPlease note: This may take some time.\n\n", sizeof("\tPlease note: This may take some time.\n\n"));

	Exec(matFileName, sizeof(matFileName));
	Exec(matFileName, sizeof(matFileName));
	Exec(sortFileName, sizeof(sortFileName));

	Exit(0);
}
