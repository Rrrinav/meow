# Meow

🐾 This is what cat is actually supposed to do.

A common cli utility as a wrapper over 'cat' command and some more features I personally need.

> [!WARNING]
> This is for linux only
> This is for my personal use. If it fits your use-case, use it.

---

## Build Instructions

### Requirements

Any C++ compiler that supports C++23 standard.  
I am using g++ (GCC) 15.0.1 & clang++ (Clang) 20.1.2, so take it as minimum requirement.

### Building

**Fast**
```bash
    $ g++ ./bld -o ./bld.cpp                      # Bootstrap build system
    $ ./bld                                       # run the build system
```

```bash
    $ git clone https://github.com/Rrrinav/meow   # Clone repo 
    $ cd meow                                     # Change directory
    $ g++ ./bld -o ./bld.cpp                      # Bootstrap build system
    $ ./bld                                       # run the build system
```

**Build options**
```bash
    $ bld clean               # delete build directory
    $ bld run                 # run the executable
```

bld will detect the compiler used to build it and use it to build the project too.
