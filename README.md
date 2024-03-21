# myshell: Shell Implementation in C

myshell is a shell implemented in C. It provides basic shell functionalities such as parsing commands, executing them, handling pipes, and logging executed commands.

## Features
- **Command Parsing**: Parses user input into commands and arguments.
- **Command Execution**: Executes commands with support for pipes.
- **Pre-Shell Commands**: Executes pre-defined commands before starting the shell.
- **Configuration Options**: Supports configuration files for customizing prompt, input size, and pre-shell commands.
- **Logging**: Logs executed commands along with their status to a log file.

## Usage
1. **Compilation**: Compile the program using any C compiler (`gcc` recommended).

gcc -o myshell myshell.c

2. **Run the Shell**: Execute the compiled binary.

./myshell

3. **Interact**: Enter commands at the prompt and press Enter to execute. Enter `exit` to exit the shell.

## Configuration
- Customize the shell behavior by editing system-wide or user-specific configuration files.
- System-wide configuration: `/etc/myshell`
- User-specific configuration: `~/.myshell_rc`

## Logging
myshell provides logging functionality to keep track of executed commands. Log entries include the timestamp, executed command, and its execution status (success or failure). The log file is located at `$HOME/.myshell.log`.
