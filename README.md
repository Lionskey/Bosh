# Bosh

Welcome to the Bosh project! A minimal Bash-like shell for GNU/Linux operating systems that is accepting contributions!

## Feature set
1. SIGINT handling
2. Command pipes
3. Whitespace parsing
4. History
5. Tab completion

And much more to come, such as redirection!

## Dependencies
1. libreadline (GNU C readline library)
2. gcc (GNU C Compiler)
3. make (optional, but highly recommended)

## Installation instructions
1. Git clone the repo

```git clone https://github.com/Lionskey/Bosh```

2. Change directory to Bosh source tree

```cd Bosh```

3. Use make to invoke the makefile for compilation and installation

```make && sudo make install```

4. Call the bosh binary from path

```bosh```
