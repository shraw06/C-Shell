# C-Shell

A custom Unix shell implemented in C with support for command execution, piping, redirection, and job control. The source code is organized into src/ and include/ directories.

Core Functionality & Parsing

    Prompt: A dynamic prompt <Username@SystemName:current_path> is displayed. The shell's home directory (~) is set to the directory where the shell executable is first run.

    Parsing: The shell uses a two-stage parsing process. First, commands are split by ; and &. Each resulting string is then tokenized by spaces to separate the command and its arguments. The parsed output is stored in a structured format that supports pipelines.

    Syntax: The parser validates command syntax by checking the order of tokens, preventing invalid constructs like | ;.
    ASSUMPTION: Empty commands or commands don't give an invalid syntax, instead it follows bash like behaviour.

Built-in Commands

    hop: Implemented in src/hop.c. Changes the current directory.

        hop - is handled using a static char[] to store the previous working directory.

        hop ~ and hop (no arguments) change to the shell's initial home directory.

    reveal: Implemented in src/reveal.c. Lists directory contents.

        Directories are sorted on the basis of their ASCII values.

        A custom filter function hides files starting with . unless the -a flag is present.

        Flags -l and -a can be combined (e.g., -la).

    log: Implemented in src/log.c. Provides command history.

        Persistence: History is stored in history.txt in the home directory.

        Logic: A maximum of 15 unique, non-log commands are stored. The implementation reads the file, adds the new command, and overwrites the oldest if the limit is exceeded, effectively creating a circular log.

Execution & Job Control

    Execution Model: The core logic in src/executor.c uses a fork-execvp-wait model. execvp is used so commands in the system's PATH are found.

    Pipes & Redirection: Pipelines are handled by creating a child process for each command. dup2 is used to redirect file descriptors for I/O (<, >, >>) and to connect the stdout of one command to the stdin of the next. All unused pipe file descriptors are closed in each child to prevent processes from hanging.

    Background Jobs (&): Background processes are managed via a global array of Job structs (see src/process.c). The shell forks the process but does not wait for it, allowing the prompt to return immediately.

    Zombie Reaping: Before each prompt, the shell checks for terminated background processes using waitpid with the WNOHANG option.

    Job Control:

        activities: Lists all running or stopped background jobs by iterating through the global job_list.

        fg/bg: Manages jobs from the list, using kill to send SIGCONT to stopped processes.

        Signal Handling (Ctrl-C/Ctrl-Z): Implemented in src/signals.c. Signal handlers forward SIGINT and SIGTSTP only to the currently active foreground process, whose PID is tracked in a global variable.

