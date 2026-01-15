# stella-garbage-collector

[Online compiler](https://fizruk.github.io/stella/)

## Build

```shell
$ mkdir build && cd build
$ cmake .. -DMAX_ALLOC_SIZE_VALUE=<MAX_ALLOC_SIZE>[ -DCMAKE_BUILD_TYPE=Debug]
$ cmake --build .
$ g++ <name>.o libruntime.a libstella_garbage_collector.a -o name
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
Total memory allocation: 401009120 bytes (25063050 objects)
Total GC invocation: 4 cycles
Maximum residency: 134217720 bytes (8388606 objects)
Total memory use: 61696157 reads and 0 writes
Total barriers triggering: 3302699 read barriers and 0 write_barriers
```

## `print_gc_state`

```shell
Heap state:
From-space: 134217728 bytes at 0x764b0adff010
To-space: 134217728 bytes at 0x764b02dff010
Stella object at 0x764b02dff010: 10
Stella object at 0x764b02dff020: 9
...
GC variable values: scan = 0x7f66f4d37240, next = 0x7f66f74ac158, limit = 0x7f66f8ec6dd0
Set of roots:
Stella object at 0x764b02dff010: 10
Stella object at 0x764b02dff020: 9
...
Current memory allocation: 106845064 bytes (6677815 objects)
Current memory available: 27372664 bytes
```
