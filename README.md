# stella-garbage-collector

[Online compiler](https://fizruk.github.io/stella/)

## Build

```shell
$ mkdir build && cd build
$ cmake .. -DMAX_ALLOC_SIZE_VALUE=<MAX_ALLOC_SIZE>[ -DCMAKE_BUILD_TYPE=Debug]
$ cmake --build .
$ g++ <name>.o libstella_garbage_collector.a -o name
```

## Run

```shell
$ ./name
```

## Compile-time variables

* `MAX_ALLOC_SIZE_VALUE` --- maximum size of heap region (in bytes);
* `CMAKE_BUILD_TYPE` --- when set to `Debug`, enables printing runtime statistics.

## `print_gc_alloc_stats`

```shell
Garbage collector (GC) statistics:
Total memory allocation: 415417328 bytes (25963563 objects)
Total GC invocation: 6 cycles
Maximum residency: 134217720 bytes (8388606 objects)
Total memory use: 64397696 reads and 0 writes
Total barriers triggering: 0 read barriers and 0 write_barriers
```

## `print_gc_state`

```shell
Heap state:
From-space: 134217728 bytes at 0x7a63223ff010
Stella object with 1 fields at 0x7a63223ff010, Stella object with 1 fields at 0x7a63223ff020, ..., Stella object with 1 fields at 0x7a632a3ff000
To-space: 134217728 bytes at 0x7a632a3ff010
GC variable values: scan = 0x7a63249daf10, next = 0x7a63291f3288, limit = 0x7a632a1a1410
Set of roots: 0x7a63223ff010, 0x7a63223ff020, ..., 0x7a632a3ff000
Current memory allocation: 117775992 bytes (7360998 objects)
Current memory available: 16441736 bytes
```
