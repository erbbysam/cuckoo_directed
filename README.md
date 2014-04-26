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

```
Example output:
With PROOFSIZE = 2

$ ./cuckoo
Running Cuckoo cycle finder with parameters
SIZESHIFT: 17, SIZE: 131072 possible values, STORAGE SIZE: 1048576 bytes Possible nonce values: 1310700

string: 59 EXAMPLE BLOCK
hash: 056EA409D666BFF41A9A45C269DD593D616D0A7B7E7054A2C3B9F3857A12278F (1 zeros)
Sol:
Size:2 Memory:14732 Chains:153670 Edges:248089
2: u:52215(recomputed) v:26033 nonce:207089 j:207089
1: u:52215 v:26033(recomputed) nonce:0 j:1

string: 216 EXAMPLE BLOCK
hash: 006A8410024DFE84236B459551F1FA0ABCF2CE4737D20CF0D2EB171EEEBAE2D4 (2 zeros)
Sol:
Size:2 Memory:27371 Chains:101215 Edges:150865
2: u:26328(recomputed) v:3753 nonce:122597 j:122597
1: u:26328 v:3753(recomputed) nonce:0 j:1

string: 1045 EXAMPLE BLOCK
hash: 00033FD84A3E634379D3EC40CB1316C706C6033647BFDE2E7EB5B79DEFACB89C (3 zeros)
Sol:
Size:2 Memory:27371 Chains:116018 Edges:177772
2: u:3429(recomputed) v:6950 nonce:145398 j:145398
1: u:3429 v:6950(recomputed) nonce:0 j:1



With PROOFSIZE = 10 (maxed out at 589.9MiB of RAM during short run)


$ ./cuckoo
Running Cuckoo cycle finder with parameters
SIZESHIFT: 17, SIZE: 131072 possible values, STORAGE SIZE: 1048576 bytes Possible nonce values: 1310700

string: 42 EXAMPLE BLOCK
hash: 094A434A8BF4551DB24E407A52959EB07D37FEB222B0C94ED1CEC4B62728BCFD (1 zeros)
Sol:
Size:10 Memory:110300 Chains:658151 Edges:3746822
10: u:4527(recomputed) v:4463 nonce:745029 j:12621711076756
9: u:8543 v:4463(recomputed) nonce:653862 j:8436321297704
8: u:8543(recomputed) v:1490 nonce:598515 j:10391480702421
7: u:61310 v:1490(recomputed) nonce:547159 j:4653268088339
6: u:61310(recomputed) v:49299 nonce:532559 j:9236867797551
5: u:25011 v:49299(recomputed) nonce:293643 j:10115235193649
4: u:25011(recomputed) v:2295 nonce:291125 j:11927218090779
3: u:11873 v:2295(recomputed) nonce:266625 j:35283286125
2: u:11873(recomputed) v:43714 nonce:132333 j:132333
1: u:4527 v:43714(recomputed) nonce:0 j:1
```
