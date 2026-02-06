Triangular Numbers in RISC-V (RIPES)

This program computes triangular numbers using four different approaches implemented in RISC-V assembly and tested in the RIPES simulator.
Triangular number : T[n] = n + T[n-1]

Implemented Functions
The program contains four implementations of Triangular(n):
1. Closed-Form Formula -  Computes the triangular number using:  T(n) = n(n+1)/2

2. Iterative Method - Uses a loop to sum numbers from 1 to n.

3. Recursive Method - Uses the recurrence:  T(n) = n + T(n−1)
                    - Base cases:  T(0) = 0  , T(1) = 1
                    - uses stack to store:  return address ('ra') , current value of n

4. Memoized Recursive Method - uses global array 'memo[]' which stores previously computed values:  memo[n] = T(n)

Program Behaviour
The program runs in a loop:

1. Prompts the user for an integer 'n'
2. Computes 'T(n)' using all four methods
3. Prints the results
4. Repeats until the user enters '-1'

Input Constraints

* Negative integers are not allowed (except '-1' to exit)
* Maximum supported value: 65535 - This limit prevents overflow in 32-bit arithmetic , as T(65535) < 2^31 - 1 < T(65536) .

Test Cases

The following inputs were tested:

0
1
2
3
8
10
65535
65536
70898
-2
-7
-1

All four implementations produced identical results.

Files Included
triangular.s
Readme.txt

