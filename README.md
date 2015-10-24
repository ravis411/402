# CSCI402
CSCI350/402 Fall 2015 Class Projects

# Project 3: Virtual Memory


1.
Implement TLB. 
Handle TLB misses.
Keep track of dirty and use flags.
matmult should pass 7220 to Exit()
Have a functioning Exec system call.

2.
Implement virtual memory.
Store and retrieve pages from disk to memory and memory to disk.
Single swap file organized by pages.
Nothing is preloaded from executable when a process starts.
Keep track of whether a virtual page is in memory, swap, or executable.
When memory is full, select a page to remove from memory.
Keep track of pages currently in use.
'core map' or IPT: translates physical page numbers to virtual pages.
Implement Random page replacement and FIFO
Be able to Fork or Exec two matmult/sort threads.

3.
Implement remote procedure calls
Lock, Condition, integer Monitor variables,
Create, retreive, set.
