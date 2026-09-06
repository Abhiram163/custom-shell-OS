# Custom Unix Shell

A basic Unix shell written in C as an Operating Systems project.

The shell can run normal Linux commands and supports a few shell features such as pipes, output redirection, sequential execution, and parallel execution.

## Features

* Run normal Linux commands
* `cd` command
* `exit` command
* Sequential execution using `##`
* Parallel execution using `&&`
* Output redirection using `>`
* Piping using `|`
* Basic signal handling for `Ctrl+C` and `Ctrl+Z`

## Commands

### Normal commands

Commands are executed using `fork()` and `execvp()`.

```bash
ls
```

```bash
pwd
```

```bash
date
```

### Change directory

```bash
cd /home/user
```

Running `cd` without an argument goes to the home directory.

```bash
cd
```

### Sequential execution

Use `##` to run commands one after another.

```bash
pwd ## ls ## date
```

The next command starts after the previous one finishes.

### Parallel execution

Use `&&` to start multiple commands without waiting for each one individually.

```bash
sleep 5 && date
```

The shell waits for all the processes after starting them.

> In this project, `&&` is used for parallel execution. It does not have the same meaning as `&&` in Bash.

### Output redirection

Use `>` to redirect the output of a command to a file.

```bash
ls > output.txt
```

The output is appended to the file.

### Pipes

Commands can be connected using `|`.

```bash
ls | wc
```

Multiple pipes are also supported:

```bash
ls | grep ".c" | wc
```

Pipes are implemented using the `pipe()` system call and `dup2()`.

## How it works

The shell continuously:

1. Displays the current working directory.
2. Reads the user's input.
3. Splits the input into arguments.
4. Checks for special operators (`>`, `&&`, `##`, `|`).
5. Runs the appropriate command handler.
6. Creates child processes using `fork()` when required.
7. Executes commands using `execvp()`.
8. Waits for child processes using `waitpid()`.

The main command-handling functions are:

```text
tokenizeInput()
runSimpleCommand()
runParallelCommands()
runSequentialCommands()
runRedirectedCommand()
runPipedCommands()
```

## System calls used

Some of the main Unix/Linux functions used in the project are:

* `fork()` - create a child process
* `execvp()` - execute a program
* `waitpid()` - wait for a child process
* `pipe()` - create a pipe between processes
* `dup2()` - redirect file descriptors
* `open()` - open/create files
* `close()` - close file descriptors
* `chdir()` - change the current directory
* `getcwd()` - get the current working directory
* `signal()` - handle signals

## Signals

The shell ignores `SIGINT` and `SIGTSTP` so that `Ctrl+C` and `Ctrl+Z` do not directly terminate or stop the shell.

Child processes restore the default signal behaviour, allowing the commands being executed to respond normally to these signals.

## Compilation

Compile using GCC:

```bash
gcc main.c -o shell
```

Run:

```bash
./shell
```

## Example

```text
/home/user$ pwd
/home/user

/home/user$ ls
main.c
output.txt

/home/user$ echo Hello
Hello

/home/user$ ls | wc

/home/user$ pwd ## date
/home/user
Sun Sep  6 ...

/home/user$ ls > output.txt

/home/user$ exit
Exiting shell...
```

## Limitations

This is a basic shell implementation and does not try to replicate all Bash features.

Some things that are not supported include:

* Quoted arguments
* Environment variable expansion
* Wildcards
* Input redirection using `<`
* Error redirection
* Background execution using `&`
* Command substitution
* Bash scripting

The input parser is also fairly simple and expects commands and operators to be separated appropriately by spaces.

## Files

```text
.
├── main.c
└── README.md
```

## Author

Abhiram Vadhri

## Note

This project was developed as part of an Operating Systems assignment to understand process creation, process execution, pipes, file descriptors, signals, and basic shell functionality.
