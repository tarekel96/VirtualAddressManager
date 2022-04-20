# CPSC 380 Assignment 6 - Virtual Address Manager

### NOTE: Collaboration project between Jessica Roux and Tarek El-Hajjaoui.

## Description of Program:
- A C program that translates logical to physical addresses for a virtual address space of size 216 = 65,536 bytes.

## Instructions to run the program:
1. Compile with gcc:
```
gcc vmmgr.c -o vmmgr -Wall
```
2. Run the program:
```
./vmmgr addresses.txt
```

## Source Files:
- vmmgr.c
- addresses.txt
- BACKING_STORE.bin
- output.txt (output of program)
- README.md


## Sources referred to:
- https://www.geeksforgeeks.org/logical-and-physical-address-in-operating-system/
- https://www.cs.cornell.edu/courses/cs4410/2016su/slides/lecture11.pdf
- https://www.geeksforgeeks.org/bitwise-operators-in-c-cpp/
- https://man7.org/linux/man-pages/man2/read.2.html
- https://man7.org/linux/man-pages/man3/free.3.html
