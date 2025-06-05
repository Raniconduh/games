# Games

A collection of small games. Configuration is done in the source file of each
game.

Each game is in its own directory, accompanied with a README file explaining how
to play the game and what configuration options are available. Additionally,
each game is only a single source file, allowing them the be turned into what
may be referred to as "C scripts".

## Compilation

Each game requires only ncurses. Since the source for each game is a single
file, compilation is fairly simple, and no makefile is provided.

Enter the directory for whichever game. Compile the single C file using any C
compiler, linking against ncurses. (`tcc` is my preferred compiler for this
project.) Once compiled, simply run the binary.

For example, to run `snake`:

```
$ cd snake
$ cc -o snake snake.c -lncurses
$ ./snake
```

If using `tcc`, the game can be run directly from the compiler:

```
$ cd snake
$ tcc -lncurses -run snake.c
```

## Configuring

Configuration is done by either specifying the config options in the compiler
command line, or by `#define`-ing them at the top of the C file.

To configure from the command line, the compilation command may look like this:

```
$ cc -DX=128 -DY=64 -o snake snake.c -lncurses
```

This will set the `X` option equal to `128` and the `Y` option to `64`.

Configuring directly in the source file may look like this:

```c
// define your options here vvv
#define X 128
#define Y 64

// this is stuff already in the file vvv
#include <ncurses.h>
...
```
