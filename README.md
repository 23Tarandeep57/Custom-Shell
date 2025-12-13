# Taran's Custom Shell

A feature-rich Unix shell implementation written in C++ with support for built-in commands, pipes, redirections, quoting, tab completion, and persistent command history.

## Features

### Built-in Commands

| Command | Description |
|---------|-------------|
| `echo <args>` | Print arguments to stdout |
| `pwd` | Print current working directory |
| `cd <path>` | Change directory (supports `~` for home) |
| `type <cmd>` | Display command type (builtin or executable path) |
| `history` | Display command history |
| `history <n>` | Show last n history entries |
| `history -w <file>` | Write history to file |
| `history -r <file>` | Read history from file |
| `history -a <file>` | Append history to file |
| `shell_name` | Display shell name |
| `byee [code]` | Exit shell with optional exit code |

### Quoting & Escaping

- **Single quotes (`'...'`)**: Preserve literal value of all characters
- **Double quotes (`"..."`)**: Allow escape sequences for `\"`, `\\`, and spaces
- **Backslash escaping**: `\ `, `\\`, `\'`, `\"` outside quotes

### I/O Redirection

| Operator | Description |
|----------|-------------|
| `>` or `1>` | Redirect stdout (overwrite) |
| `>>` or `1>>` | Redirect stdout (append) |
| `2>` | Redirect stderr (overwrite) |
| `2>>` | Redirect stderr (append) |

### Pipes

Full pipeline support with `|` operator:
```bash
ls -la | grep ".txt" | wc -l
echo hello | cat | wc -c
```

Builtins work within pipes:
```bash
ls | type echo
echo "hello world" | cat
```

### Tab Completion

- Auto-completes built-in commands
- Searches PATH directories for executable completions
- Longest common prefix (LCP) completion for multiple matches

### Command History

- **Arrow key navigation**: Use ↑/↓ to browse previous commands
- **Persistent storage**: History saved to `~/.tshell_history`
- **Session persistence**: History loads on startup and saves after each command
- **Configurable limit**: Default 1000 entries

## Implementation Details

### Architecture

```
src/
├── main.cpp      # Main shell loop, builtins, tokenizer, redirections
├── pipe.cpp      # Pipeline execution module
└── pipe.h        # Pipeline header
```

### Tokenizer

The shell uses a custom quote-aware tokenizer that:
1. Tracks single/double quote state
2. Handles escape sequences based on context
3. Returns tokens with metadata about quote usage (for `cd ~` handling)

```cpp
struct Token {
    string text;
    bool had_single_quotes;
    bool had_double_quotes;
};
```

### Process Execution

- **External commands**: Uses `fork()` + `execv()` with PATH lookup
- **Output capture**: Pipe-based stdout capture for redirection support
- **Pipes**: Fork chain with `pipe()` and `dup2()` for file descriptor manipulation

### Redirection Implementation

```cpp
// Stderr redirection parsing
pair<bool, bool> parse_stderr_redirection(vector<string>& argv, 
                                          optional<string>& stderr_path, 
                                          string& err);

// Temporary stderr redirect for builtins
void with_stderr_redir(const optional<string>& stderr_path, 
                       bool append_mode, 
                       const function<void()>& body);
```

### Pipeline Module

The pipe module (`pipe.cpp`) handles multi-stage pipelines:
1. Splits command by `|` character
2. Creates pipe file descriptors between stages
3. Forks child processes for each stage
4. Handles both external commands and builtins in pipeline context

## Building

### Dependencies

- GCC with C++17 support
- GNU Readline library
- ncurses library

### Compile

```bash
cd src
g++ -fPIE -o main main.cpp pipe.cpp -lreadline -lhistory -lncurses
```

### Run

```bash
./main
```

## Usage Examples

```bash
# Basic commands
Taran@Shell:~$ echo Hello, World!
Hello, World!

Taran@Shell:~$ pwd
/home/taran

# Quoting
Taran@Shell:~$ echo 'single quotes preserve $everything'
single quotes preserve $everything

Taran@Shell:~$ echo "double quotes allow \"escapes\""
double quotes allow "escapes"

# Redirection
Taran@Shell:~$ echo "log entry" >> log.txt
Taran@Shell:~$ cat nonexistent 2> errors.txt

# Pipes
Taran@Shell:~$ ls -la | grep ".cpp" | wc -l
3

# History
Taran@Shell:~$ history 5
   45  ls -la
   46  cd src
   47  pwd
   48  echo test
   49  history 5

# Tab completion
Taran@Shell:~$ ech<TAB>
Taran@Shell:~$ echo
```

## File Structure

| File | Purpose |
|------|---------|
| `main.cpp` | Main shell implementation (~730 lines) |
| `pipe.cpp` | Pipeline execution (~195 lines) |
| `pipe.h` | Pipeline header declarations |
| `~/.tshell_history` | Persistent history file |

## Author

**Taran** - Custom Shell Project

## License

This project is open source and available for educational purposes.
