# Portable Compiler For 8086 Systems

Cant find a C compiler targeting general 8086?

Me neither! So i made this, a very portable compiler depending on the p compiler, which in itself is quite portable.

You can compile P itself, or compile P++, a flavour thats much easier to use.

## Features

* P compiler (smallos program or general 8086)
* P++ compiler (flavour of p)
* Raw memory management (even more than C)
* While and for loops (in P++)
* Easy output

## Example programs

### Hello world

```
print Hello, World!$
halt
```

### Ten hello worlds

```
let x = 0
for x = 0 x < 10 inc 1
    print Hello, World!$
endf
halt
```
