# Shell-Emulator

## Overview

Simple **command-line shell emulator** written in C.  
It replicates the basic functionality of a UNIX shell — including executing external programs, handling built-in commands (`cd`, `pwd`, `exit`), managing **I/O redirection**, and supporting **multiple pipes** in a single command line.

The goal of this project is to gain an understanding of **process control** and **system call interfaces** in Linux, while maintaining modular and readable C code.

---

## Features Implemented

### Built-in Commands
| Command | Description |
|----------|-------------|
| `cd [dir]` | Changes the current working directory. If no argument is provided, it defaults to `$HOME` |
| `pwd` | Prints the current working directory |
| `exit [status]` | Exits the shell (optionally accepts an exit code) |

### External Command Execution
- Executes any valid executable in your `$PATH`.
- Handles invalid commands gracefully with error messages.

### I/O Redirection
| Operator | Description | Example |
|-----------|--------------|----------|
| `>` | Redirects standard output to a file (creates/truncates file) | `ls > out.txt` |
| `<` | Redirects standard input from a file | `sort < data.txt` |

### Pipes
Supports multiple pipes, allowing chained commands:
```bash
cat input.txt | grep error | sort | uniq
```

### Status Substitution
Supports $? to access the exit status of the last executed command:
```bash
echo $?
```

### Error Handling
- Detects missing filenames or syntax errors in redirections
- Detects empty pipe commands (e.g., `| ls` or `ls | | grep`) and ignores them (`ls || grep` becomes `ls | grep`)
- Prints descriptive errors using strerror(errno)
- Can exit out of running commands using Ctrl+C

### Interactive & Script Modes
- Interactive mode: Standard command-line interface ($ prompt)
- Batch mode: Execute commands from a file:
```bash
./shell commands.txt
```

### How to run
1. To compile the shell
```bash
make
```
2. Start the shell in interactive mode
```bash
./shell
```
3. To clean the directory
```bash
make clean
```

## Current Limitations / Future Improvements
| Command | Description |
|----------|-------------|
| Background jobs (&) | Allow commands to run asynchronously in the background |
| Command history | Support !!, !n, and arrow-key navigation |
| Environment variables | Expand $PATH, $HOME, etc., and support export |
| Append redirection (>>) | Add output appending functionality |
| Logical operators | Allow sequential and conditional command execution |