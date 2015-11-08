/*
*
*	proj3TestSuite.c
*
*/

#include "syscall.h"

char welcomeString[] = "\nproj3TestSuite.\n\n";
char matFileName[] = "../test/testVirtualMemory.c";
char sortFileName[] = "../test/sort";

int
main()
{
    PrintString(welcomeString, sizeof(welcomeString));

    PrintString("Going to Exec 2 testVirtualMemory.c and 1 sort.c\n", sizeof("Going to Exec 2 testVirtualMemory.c and 1 sort.c\n"));
    PrintString("\ttestVirtualMemory forks 2 matmults.\n", sizeof("\ttestVirtualMemory forks 2 matmults.\n"));
    PrintString("\tExpected output is 5 Exit statuses, 4 7220s and 1 1023\n\n", sizeof("\tExpected output is 5 Exit statuses, 4 7220s and 1 1023\n\n"));

	Exec(matFileName, sizeof(matFileName));
	Exec(matFileName, sizeof(matFileName));
	Exec(sortFileName, sizeof(sortFileName));

	Exit(0);
}
