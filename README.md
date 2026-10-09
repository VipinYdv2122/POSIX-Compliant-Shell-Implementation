
# README — POSIX-like Shell (AOS Assignment 2)

## Summary
This project implements a small interactive POSIX-like shell in C++ with support for:
- Prompt display (`user@host:cwd>`), with `~` for home.
- Built-in commands: `cd`, `ls` (`-a`, `-l`), `echo`, `pwd`, `pinfo`, `history`, `search`, `exit`.
- Running external commands in foreground or background (`&`).
- I/O redirection (`<`, `>`, `>>`).
- Pipelines (`|`) across multiple commands.
- Command separators (`;`) to run multiple commands on a line.
- Autocomplete using GNU Readline (commands & filenames).
- Persistent history (`~/.myshell_history`).



## Files
- `q1.cpp` — main source containing the full shell implementation.
- `Makefile` — build file to compile the project into `myshell`.


## Build prerequisites
On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential libreadline-dev
```

---

## Makefile (how to build)
A simple Makefile:

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
LDFLAGS = -lreadline

TARGET = myshell
SRC = q1.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
```

Build:
```bash
make
```

Run:
```bash
./myshell
```

---

## Program initialization and main loop (high level)
1. **Terminal attributes**: code queries `tcgetattr()` and `tcsetattr()` — used to configure terminal behavior (control character echo etc.).  
2. **Signal handlers**: registers handlers like `signal(SIGINT, controlc)` and `signal(SIGTSTP, controlz)` so the shell can react to Ctrl-C and Ctrl-Z. It also ignores `SIGTTIN` and `SIGTTOU` to avoid background terminal access issues.  
3. **Home directory**: `chdir(home)` if available, so shell starts in user home.  
4. **History**: loads history from `~/.myshell_history` (uses Readline `read_history`).  
5. **Autocomplete**: registers `rl_attempted_completion_function` to implement custom completions; binds TAB to `rl_complete`.  
6. **Read-eval loop**: shell prints prompt from `display_user()` and calls `readline()` to read input. If input is empty, it loops; otherwise it stores in history and writes history file.

---

## Input handling — splitting/extracting commands

- `cmdsplit(const string &cmd)`  
  Splits the raw input line on semicolons `;`. Example: `ls; echo hi` → `["ls", " echo hi"]`. After splitting, the shell trims whitespace on each command using `trim()`.

- `tokanize(string &command, vector<string> &tokens)`  
  Tokenizes a single command by whitespace using `istringstream >>`. **Note:** this tokenizer is simple and does not honor quotes or escaped spaces — tokens containing spaces are not handled.

---

## Prompt and helper functions

- `display_user()`:
  - Uses `getlogin()` to get username (falls back to `"user"` if null).
  - Uses `gethostname()` to get host name.
  - Uses `getcwd()` to get current working directory path.
  - Replaces the home directory prefix with `~` for display.
  - Returns `username@host:path>` string (used as prompt for `readline()`).

- `trim()`:
  - Trims leading and trailing whitespace from a string.

---

## Built-in commands — workflow and code-level details

### `cd` — `cdFunction(vector<string> &tokens)`
Purpose: change current working directory.

Flow:
1. If `tokens.size() > 2`, function prints `Invalid command` with `perror()` (not ideal, but follows code).
2. Stores current directory in `curr_directory` using `getcwd()` to later set `prev_dir`.
3. Compute `new_dir`:
   - If no argument or `~` → `getenv("HOME")`.
   - If `-` → switch to `prev_dir` (if empty prints perror).
   - Else → `tokens[1]`.
4. Calls `chdir(new_dir.c_str())`; if failure prints perror. On success sets `prev_dir = curr_directory`.

Notes:
- `prev_dir` is global; used for `cd -`.
- No tilde expansion inside paths besides the explicit `~` token case.

---

### `echo` — `echoFunction(vector<string> &tokens)`
Purpose: print tokens 1..n separated by spaces.

Flow:
- Iterates `tokens` from index `1` to end, prints each with a space, then newline.

Notes:
- No interpretation of escape sequences (`\n`, etc.) or flags like `-n`.

---

### `pwd` — `pwdFunction()`
Purpose: print current working directory.

Flow:
- Calls `getcwd()` and prints the result.
- If `getcwd()` fails, prints perror message.

---

### `ls` — `lsFunction(vector<string> &tokens)`
Purpose: list file(s) and directory contents with optional flags.

Supported flags:
- `-a` — show hidden files.
- `-l` — long listing (permissions, links, owner, group, size, time, name).

Flow:
1. Parse `tokens` from index 1 to detect flags and collect path arguments into `path` vector. If no path supplied, default to `"."`.
2. For each `p` in `path`:
   - `stat(p.c_str(), &pstat)` to determine if `p` is a file or directory.
   - If `S_ISREG(pstat.st_mode)`:
     - If `-l`: print permission string (from `permissions()`), link count, owner name, group name, size, last modification time (formatted via `strftime` and `localtime`), then file name.
     - Else: print file name.
   - If directory:
     - `opendir(p.c_str())`, iterate `readdir()` entries:
       - Skip hidden files unless `-a` set.
       - For each entry create `finalpath = p + "/" + dirname` and `stat(finalpath, &newst)`.
       - If `-l`: build permissions and print detailed info as for files.
       - Else: print name.
   - After finishing a path, if more than 1 path, prints a blank line between outputs.

Helper function:
- `permissions(mode_t mode, string &p)`:
  - Builds a string like `-rwxr-xr-x` by checking file type bits (regular, dir, symlink) and read/write/execute bits for user/group/others.

Notes:
- Time format used: `"%b %d %H:%M"` (month day hour:minute).
- Owner & group retrieved with `getpwuid()` and `getgrgid()`.

---

### `pinfo` — `pinfoFunction(vector<string> &tokens)`
Purpose: display info about a process (current process if no PID provided).

Flow:
1. Determine `id`:
   - `tokens.size() == 1` → use `getpid()`.
   - `tokens.size() == 2` → parse PID from `tokens[1]`.
2. Read `/proc/<id>/stat` using `open()` and `read()`. The file contains several space-separated fields.
3. Parse:
   - `status` — the process state (character).
   - `p` and `t` — two numeric fields compared to decide if `+` should be appended to status.
   - `memory` — field corresponding to virtual memory (index c==22 in code; note: `/proc/<pid>/stat` field indices are sensitive — the code approximates this).
4. `readlink("/proc/<id>/exe", exepath, ...)` to get the path of the executable.
5. Print: PID, process status (with `+` if foreground), memory, executable path.

Notes:
- The code assumes a particular ordering of fields read from `/proc/<pid>/stat` and uses counters; it's fragile but works on many Linux systems.
- If `/proc` files are not accessible (e.g. permission), appropriate error messages printed.

---

### `history`
Purpose: print last up to N commands from readline history.

Flow:
- Uses `history_list()` to get an array of `HIST_ENTRY*` and `history_length`.
- By default prints last 10 lines; if `history N`, parses N from `tokens[1]` and limits N to 20 in code.
- Iterates and prints recent entries.

Notes:
- History is persisted to `~/.myshell_history` via `write_history` every command.

---

### `search` — `searchfile(string &currdir, string &filename)`
Purpose: recursively search from the given directory for a filename; returns true if found.

Flow:
1. `opendir(currdir)`.
2. Iterate directory entries with `readdir`.
3. Skip `.` and `..`.
4. If `dirname == filename`, return true.
5. If entry is a directory (`stat` and `S_ISDIR`), recursively call `searchfile(fullpath, filename)`. If found, close directory and return true.
6. If loop finishes, close directory and return false.

Notes:
- This is a depth-first recursive search. It uses `stat` and could hit permission errors on some directories.

---

## External commands & job control

### `backforcmd(vector<string> &tokens)`
Handles running external commands, with optional background execution.

Flow:
1. Detect background: if last token is `"&"`, set `flag=true` and `pop_back()` the token.
2. `fork()`:
   - **Child**:
     - Build `char *v[]` array of C strings from `tokens`.
     - `execvp(v[0], v)` to run the command.
     - On failure prints error and `exit(1)`.
   - **Parent**:
     - If not background:
       - Sets global `fpid = id;` (used by signal handlers to forward signals).
       - `waitpid(id, NULL, WUNTRACED)` which waits for the child to finish or stop; then sets `fpid = -1`.
     - If background:
       - Prints `[<pid>]running in background` and does not wait.

Notes:
- This is simple job control: it keeps track of the foreground child's PID in `fpid` so signal handlers can forward signals to it. It does NOT implement POSIX process groups using `setpgid()`/`tcsetpgrp()` — so signals and job control for pipelines are imperfect.

---

## I/O redirection — `iored(vector<string> &tokens)` and `inputred(string &inputfile)`
Purpose: support `<`, `>`, `>>` for stdin/stdout redirection.

Flow:
1. Parse `tokens` in order:
   - If token is `<`, next token is `inputfile`.
   - If token is `>`, next token is `outputfile` (truncate).
   - If token is `>>`, next token is `outputfile` (append).
   - Otherwise push token into argv vector `v`.
2. Push `NULL` terminator to `v`.
3. `fork()`:
   - **Child**:
     - If `inputfile` is set → `open(inputfile, O_RDONLY)`, `dup2(inputfd, STDIN_FILENO)`, close.
     - If `outputfile` set → `open(outputfile, O_WRONLY | O_CREAT | O_TRUNC/APPEND, 0644)`, `dup2(outputfd, STDOUT_FILENO)`, close.
     - `execvp(v[0], v.data())`.
   - **Parent**: `wait(NULL)` (wait for child).

Notes:
- The code doesn't handle combined background redirection specially.
- No support for file descriptor errors beyond `perror` and `exit`.

---

## Pipelines — `pipeline(vector<string> &tokens)`
Purpose: create a pipeline of N commands connected by `|`.

Flow:
1. `splitcmd()` splits tokens at `|` producing vector of `cmd` vectors.
2. Use `prfd` to hold read-end from previous pipe.
3. For each command index `i` from 0 to n-1:
   - If `i < n-1` create `pipe(pfd)`.
   - `fork()`:
     - **Child**:
       - If not last (`i<n-1`) `dup2(pfd[1], STDOUT_FILENO)` then close both pipe ends.
       - If `prfd != -1` -> `dup2(prfd, STDIN_FILENO)` and `close(prfd)`.
       - Parse redirections only for first or last commands using `checkred()`; otherwise build argv vector directly.
       - If first and has input redirection -> call `inputred`.
       - If last and has output redirection -> open output file and `dup2` to STDOUT.
       - `execvp(v[0], v.data())`.
     - **Parent**:
       - Add child pid to `vpid`.
       - Close `prfd` if set. If not last, close write-end `pfd[1]` and set `prfd = pfd[0]`.
4. After forking all children, parent waits for each pid in `vpid` using `waitpid`.

Notes:
- Redirections are only checked in first and last stages by `checkred`.
- Pipeline does not set process groups, so Ctrl-C may not kill the entire pipeline as a single unit.

---

## Signals — `controlc`, `controlz`, `controld`

- `controlc(int sig)`:
  - If `fpid > 0`, `kill(fpid, SIGINT)` — sends SIGINT to the child (not to a process group).
  - Else writes `^C` newline and uses Readline helpers `rl_replace_line`, `rl_on_new_line`, `rl_redisplay` to refresh the prompt.
  - Sets `fpid = -1` after forwarding.

- `controlz(int sig)`:
  - If `fpid > 0`, `kill(fpid, SIGTSTP)` to stop foreground child; sets `fpid=-1`.
  - Else refreshes prompt with Readline helpers.

- `controld(int sig)`:
  - Exits the shell.

- In `main()` the code registers:
  ```cpp
  signal(SIGINT, controlc);
  signal(SIGTSTP, controlz);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  ```

---

## Completion & History (Readline)
- Autocomplete callback: `autocomplete()` calls `rl_completion_matches()` with either `cmdgen` (for builtins) or `rl_filename_completion_function` for filenames.
- `cmdgen(const char* t, int s)` scans the `cmds` vector (built-in command names) to propose matches starting with typed prefix.
- History is kept with `add_history`, `read_history`, and `write_history`.



