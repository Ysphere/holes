In this assignment, I use a double-linked list to store the holds. At start, we
init a single hold with size 128. Every time we need a hold, just search the list and find a fit hold. If the found hold is large than the process size, it will
 split two part, one part for the process and another one remain in the list. Wh
en we push a hold back to the list, we also check if it should merge with the pr
ev or the next hold. If it is, just do merge.
Assumptions: I assume that data in the input file is valid.