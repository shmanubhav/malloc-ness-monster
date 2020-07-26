## malloc-ness-moster

A thread-safe memory allocater written in C, faster than GNU's malloc.
(Tested on Collatz Conjecture simulation datasets)

---

### Stratgey:
- Allocate one thread local arena for each thread.
- Initiliaze all bins to NULL.
- If bin is null then allocate 2 pages to the bins 2048bytes+
- If less memory is needed, break larger bin into a two bins
 of half the size of the original bin and put it in the 
 corresponding free-list
 bin[0] - size 32 bytes
 bin[1] - size 64 bytes
 bin[2] - size 128 bytes
 bin[3] - size 256 bytes
 bin[4] - size 512 bytes
 bin[5] - size 1024 bytes
 bin[6] - size 2048+ bytes
- repeat the process until we get a bin of the desired size
- replace bin with header and footer and return a pointer
 after the header
 eg. if we want to allocate 150 bytes we allocate an entire bin
    of 256 bytes.

- for freeing, create a new node at the pointer to be freed
- return bin to corresponding freelist

----

###  Runtimes
|             |  hw7   |  par  |  sys  |
|-------------|--------|-------|-------|
| ivec 10000  | 30.105 | 0.049 | 0.060 |
| ivec 100000 | 600.00 | 0.421 | 0.433 |
| list 10000  | 600.00 | 0.081 | 0.128 |
| list 100000 | 600.00 | 0.887 | 1.369 |

### System Specifications
- RAM : 16GB
- CPU : Intel® Core™ i7-6600U CPU @ 2.60GHz × 4 
- OS : Ubuntu 16.04 LTS

---

### Results:
- We reduced mmaps and munmaps to a bare minimum.
- We do not need to sort the freelist.
- We do not use mutexes to make optmalloc thread safe.
- This lack of system calls makes optmalloc much faster than the
unthreaded allocator and relatively faster than GNU malloc.
