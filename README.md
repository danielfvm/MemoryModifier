# MemoryModifier

MemoryModifier is a tool and a library for scanning and modifying memory on Linux.
It can also be used for creating patterns to search in your own C scripts.

## Installation

```
$ cd MemoryModifier 
$ make 
$ make install 
```

## Use MemoryModifier

To use this tool you will need sudo permissions.

```
$ sudo mm
```

## Use library

[examples](https://github.com/danielfvm/MemoryModifier/tree/master/examples)

### Open a process
First open a process by name
```c
Process *p;

if ((p = openProcess("application")) == NULL) {
    fprintf(stderr, "Failed to open process memory");
}
```

### Open Memory Region
Now open the memory region, you can manually look at it in ``/proc/PID/maps``
In this case we open the heap.
```c
MemoryRegion heap;
if (getMemoryRegion(p, "[heap]", &heap) == 0) {
    fprintf(stderr, "Failed to open memory region");
}
```

### Read memory
```c
byte number[4];
if (readProcessMemory(heap, heap.start + offset, , sizeof(int)) == 0) {
    fprintf(stderr, "Failed to write to memory");
}
```

### Write to memory
```c
if (writeProcessMemory(heap, heap.start + offset, getBytes(int, 1234), sizeof(int)) == 0) {
    fprintf(stderr, "Failed to write to memory");
}
```
