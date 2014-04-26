cuckoo_directed
===============

Low quality code for my paper "Seeking a Better Proof or Work Function for Cryptocurrency".


This repository contains code for an example proof of work miner for the modification to the cuckoo proof of work function that were presented in the paper found here.

This code is of low quality, but is a proof of concept that shows how this can be done. 

This code works by storing the u/v nodes in a hash table which is then iterated over for each possible nonce value. 
This is repeated until a cycle is found. Something to note is that after the first pass, new partial cycles of length 1 are not created and therefore only edges that add to another partial cycle are saved.
Read this paper/the original paper for more details on the algorithm itself.

The only code used from the original cuckoo repo (https://github.com/tromp/cuckoo) was part of the header file(cuckoo.h). 

This code does require openssl at this time and the only non-portable piece is the function "read_off_memory_status" which will likely only work on linux systems (and is non-critical, just is used to print memory statistics).
-Sam