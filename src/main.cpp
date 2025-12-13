#include <bits/stdc++.h>
#include <sys/wait.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <optional>
#include <iomanip>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib>
#include <dirent.h>
#include "pipe.h"
using namespace std;

string EXIT = "byee";
string ECHO = "echo";
string TYPE = "type";
string PWD = "pwd";
string SHELL_NAME = "shell_name";
string CD = "cd";
string HISTORY = "history";


string get_history_file_path() {
    const char* home = getenv("HOME");
    if (home) return string(home) + "/.tshell_history";
    return ".tshell_history";
}

void load_history() {
    string history_file = get_history_file_path();
    read_history(history_file.c_str());
}

void save_history() {
    string history_file = get_history_file_path();
    write_history(history_file.c_str());
}

void history_func(int limit = -1) {
    HIST_ENTRY** hist_list = history_list();
    if (hist_list) {
        // Count total entries
        int total = 0;
        while (hist_list[total]) total++;
        
        // Determine start index based on limit
        int start = 0;
        if (limit > 0 && limit < total) {
            start = total - limit;
        }
        
        for (int i = start; hist_list[i]; i++) {
            cout << setw(5) << (i + history_base) << "  " << hist_list[i]->line << endl;
        }
    }
}

bool history_write(const string& filepath) {
    if (write_history(filepath.c_str()) == 0) {
        return true;
    }
    cerr << "history: cannot write to " << filepath << endl;
    return false;
}

bool history_read(const string& filepath) {
    if (read_history(filepath.c_str()) == 0) {
        return true;
    }
    cerr << "history: cannot read from " << filepath << endl;
    return false;
}

bool history_append(const string& filepath) {
    if (append_history(history_length, filepath.c_str()) == 0) {
        return true;
    }
    cerr << "history: cannot append to " << filepath << endl;
    return false;
}


set<string> builtin_set= {"byee", "echo", "type", "pwd", "shell_name", "cd", "history"}; 
vector<string> vocab = {"byee", "echo", "type", "pwd", "shell_name", "cd", "history"};

struct Token {
    string text;
    bool had_single_quotes = false;
    bool had_double_quotes = false;
};


// Parse only stderr redirection of the form: 2> <path> or 2>> <path>
// Mutates argv to remove the redirection tokens when present and sets stderr_path.
// Returns {success, append_mode}
pair<bool, bool> parse_stderr_redirection(vector<string>& argv, optional<string>& stderr_path, string& err) {
    stderr_path.reset();
    err.clear();
    bool append_mode = false;
    if (argv.empty()) return {true, false};

    vector<string> out;
    for (size_t i = 0; i < argv.size(); ++i) {
        const string& t = argv[i];
        if (t == "2>>") {
            if (i + 1 >= argv.size()) {
                err = "redirection: missing file after 2>>";
                return {false, false};
            }
            stderr_path = argv[i + 1];
            append_mode = true;
            ++i; // skip the path
            continue;
        }
        if (t == "2>") {
            if (i + 1 >= argv.size()) {
                err = "redirection: missing file after 2>";
                return {false, false};
            }
            stderr_path = argv[i + 1];
            append_mode = false;
            ++i; // skip the path
            continue;
        }
        out.push_back(t);
    }
    argv.swap(out);
    return {true, append_mode};
}


// Temporarily redirect stderr (fd 2) for builtins
void with_stderr_redir(const optional<string>& stderr_path, bool append_mode, const function<void()>& body) {
    if (!stderr_path) { body(); return; }
    int saved = dup(STDERR_FILENO);
    if (saved == -1) { body(); return; }

    int flags = O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC);
    int fd = open(stderr_path->c_str(), flags, 0644);
    if (fd == -1) {
        // If target can't be opened, emit an error and run without redirection
        cerr << *stderr_path << ": cannot open for writing" << endl;
        close(saved);
        body();
        return;
    }
    if (dup2(fd, STDERR_FILENO) == -1) {
        close(fd);
        close(saved);
        body();
        return;
    }
    close(fd);

    body();

    fflush(stderr);
    dup2(saved, STDERR_FILENO);
    close(saved);
}

pair<bool , vector<Token>> tokenize(const string& input) {
    vector<Token> tokens;
    string buff;

    bool in_single = false;
    bool in_double = false;
    bool token_had_single = false;
    bool token_had_double = false;


    auto push_token = [&]() {
        tokens.push_back(Token{buff, token_had_single, token_had_double });
        buff.clear();
        token_had_single = false;   
        token_had_double = false;
    };

    for (size_t i = 0; i  < input.size(); i++) {
        char c = input[i];

        if (!in_single && !in_double && c == '\\' && i+1 < input.size()) {
            char next = input[i + 1];

            if (next == ' ' || next == '\\' || next == '\'' || next == '\"') {
                buff.push_back(next);
                i++;
                continue;
            }
        }

        if (c == '\'' && !in_double) {
            in_single = !in_single;
            token_had_single = true;
            continue;
        }
        if (c == '\"' && !in_single) {
            in_double = !in_double;
            token_had_double = true;
            continue;
        }

        if (in_double && c == '\\' && i + 1 < input.size()) {
            char next = input[i + 1];
            if (next == '\"' || next == '\\' || next == ' ') {
                buff.push_back(next);
                i++;
                continue;
            }
        }

        if (!in_single && !in_double && isspace(static_cast<unsigned char>(c))) {
            if (!buff.empty() || token_had_single || token_had_double) {
                push_token();
            }
            continue;
        }

        buff.push_back(c);
    }
    if (in_single) {
        cerr << "unterminated single quote" << endl;
        return {false, {}};
    }

    if (in_double) {
        cerr << "unterminated double quote" << endl;
        return {false, {}};
    }

    if (!buff.empty() || token_had_single || token_had_double) {
        push_token();
    }

    return {true, tokens};
}

pair< pair<vector<string>, string> , bool > to_texts(const vector<Token>& toks) {
    vector<string> out;
    bool append_mode = 0;
    string redirect_loc;
    bool redirect_output = false;
    for (const auto& t : toks) {
        if (t.text == "1>" || t.text == ">" || t.text == ">>" || t.text == "1>>") {
            redirect_output = true;
            if (t.text == ">>" || t.text == "1>>") {
                append_mode = 1;
            }
            continue;
        }
        if (redirect_output) {
            redirect_loc = t.text;
            continue;
        }
        else out.push_back(t.text);
    }
    return {{out, redirect_loc}, append_mode};
}


void change_dir(vector<Token>& tokens) {
    string path = tokens[1].text;

    if (path == "~" && !tokens[1].had_single_quotes && !tokens[1].had_double_quotes) {
        const char* home = getenv("HOME");
        if (home != NULL) {
            path = string(home);
        } else {
            cerr << "HOME environment variable not set" << endl;
            return;
        }
    }

    if (chdir(path.c_str()) != 0){
        cerr << path << ": No such file or directory" << endl;
    }

}

string echo_func(vector<string>& words){
    string ret;
	if (words[0] == ECHO) {
		for (int i = 1; i < words.size(); i++){
			ret += words[i];
            if (i + 1 < words.size()) ret += " ";
		}
		ret += "\n";
	}
    return ret;
}

pair<bool, string> find_executables(string cmd) {
    char* path_env = getenv("PATH");
    if (path_env == NULL) {
        cout << cmd << ": not found" << endl;
        return {false, ""};
    }

    string path_str(path_env);
    // cout << "path _string: "<<path_str << endl;
    stringstream ss(path_str);
    string dir;

    while (getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        string full_path = dir + "/" + cmd;

        if (access(full_path.c_str(), X_OK) == 0) {
            return {true, full_path};
        }
    }
    return {false, ""};
}

string get_current_directory() {
    char buffer[FILENAME_MAX];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        return string(buffer);
    } else {
        cerr << "getcwd() error" ;
        return "";
    }
}


pair<bool, string> exec_capture_stdout(const vector<string>& words, const optional<string>& stderr_path = nullopt, bool stderr_append = false) {
    if (words.empty()) return {false, ""};
    auto found = find_executables(words[0]);
    if (!found.first) return {false, ""};

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return {false, ""};
    }

    pid_t pid = fork();
    if (pid == -1) {
        // fork failed
        close(pipefd[0]);
        close(pipefd[1]);
        return {false, ""};
    }

    if (pid == 0) {
  
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            _exit(127);
        }
       
        close(pipefd[1]);

        // Apply stderr redirection if requested
        if (stderr_path) {
            int eflags = O_WRONLY | O_CREAT | (stderr_append ? O_APPEND : O_TRUNC);
            int efd = open(stderr_path->c_str(), eflags, 0644);
            if (efd == -1) {
                _exit(127);
            }
            if (dup2(efd, STDERR_FILENO) == -1) {
                close(efd);
                _exit(127);
            }
            close(efd);
        }        
        vector<char*> args;
        args.reserve(words.size() + 1);
        for (const auto& w : words) args.push_back(const_cast<char*>(w.c_str()));
        args.push_back(nullptr);

        execv(found.second.c_str(), args.data());
      
        _exit(127);
    }

   
    close(pipefd[1]);
    string out;
    char buf[4096];
    ssize_t n;
    while (true) {
        n = read(pipefd[0], buf, sizeof(buf));
        if (n > 0) {
            out.append(buf, buf + n);
        } else if (n == 0) {
            break; 
        } else {
            if (errno == EINTR) continue;
            break;
        }
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    return {true, out};
}

void execute_func(vector<string>& words) {
    string cmd = words[0];
    auto result = find_executables(cmd);
    if (result.first) {
        pid_t pid = fork();
        if (pid == 0) {
            vector<char*> args;
            for (const auto& word : words) {
                args.push_back(const_cast<char*>(word.c_str()));
            }
            args.push_back(NULL);

            execv(result.second.c_str(), args.data());
            cerr << "Execution failed" << endl;
            exit(1);

        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);

        } else {
            cerr << "Fork failed" << endl;
        }
    }
    
}

string type_func(vector<string>& words) {
    string ret;	
	if (words.size() != 2) {
        cerr << "Usage : type <command>\n";
        return "";
    }

    string cmd = words[1];

    for (auto &str: builtin_set) {
        if (cmd == str) {
            ret = cmd + " is a shell builtin\n";
            return ret;
        }
    }

    auto result = find_executables(cmd);
    if (result.first) {
        ret = cmd + " is " + result.second + "\n";
        return ret;
    }
    ret = cmd + ": not found\n";
    return "";

}


void write_to_file(const string& filename, const string& content, bool append = false) {
    if (append) {
        ofstream outfile(filename, std::ios::app);
        if (outfile.is_open()) {
        outfile << content;
        outfile.close();
        } else {
            cerr << "Error opening file: " << filename << endl;
        }

    } else{
        ofstream outfile(filename);
        if (outfile.is_open()) {
        outfile << content;
        outfile.close();
        } else {
            cerr << "Error opening file: " << filename << endl;
        }
    }
    
}

// Check if input contains a pipe character (outside quotes)
bool has_pipe(const string& input) {
    bool in_single = false;
    bool in_double = false;
    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == '\'' && !in_double) in_single = !in_single;
        else if (c == '"' && !in_single) in_double = !in_double;
        else if (c == '|' && !in_single && !in_double) return true;
    }
    return false;
}

// Execute a pipeline using the pipe module
void execute_pipeline(const string& input) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process runs the pipeline
        execute_pipeline_cmd(input.c_str());
        _exit(127);  // Only reached if pipeline fails
    } else if (pid > 0) {
        // Parent waits for pipeline to complete
        int status;
        waitpid(pid, &status, 0);
    } else {
        cerr << "fork error" << endl;
    }
}


char* completion_generator(const char* text, int state) {
  static vector<string> matches;
  static size_t match_index = 0;

  if (state == 0) {
    // During initialization, compute the actual matches for 'text' and keep
    // them in a static vector.
    matches.clear();
    match_index = 0;

    // Collect a vector of matches: vocabulary words that begin with text.
    string textstr = string(text);
    
    // First, check builtin commands
    for (auto word : vocab) {
      if (word.size() >= textstr.size() &&
          word.compare(0, textstr.size(), textstr) == 0) {
        matches.push_back(word);
      }
    }
    
    // search for executables in PATH directories
    char* path_env = getenv("PATH");
    if (path_env != NULL) {
      string path_str(path_env);
      stringstream ss(path_str);
      string dir;
      set<string> seen; 
      
      // Add builtins to seen set
      for (auto& m : matches) seen.insert(m);
      
      while (getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        
        DIR* dp = opendir(dir.c_str());
        if (dp == nullptr) continue;
        
        struct dirent* entry;
        while ((entry = readdir(dp)) != nullptr) {
          string name = entry->d_name;
          // Skip . and ..
          if (name == "." || name == "..") continue;
          
          // Check if name starts with text
          if (name.size() >= textstr.size() &&
              name.compare(0, textstr.size(), textstr) == 0) {
            // Check if it's executable
            string full_path = dir + "/" + name;
            if (access(full_path.c_str(), X_OK) == 0) {
              if (seen.find(name) == seen.end()) {
                matches.push_back(name);
                seen.insert(name);
              }
            }
          }
        }
        closedir(dp);
      }
    }
    sort(matches.begin(), matches.end());
  }

  if (match_index >= matches.size()) {
    // We return nullptr to notify the caller no more matches are available.
    return nullptr;
  } else {
    // Return a malloc'd char* for the match. The caller frees it.
    return strdup(matches[match_index++].c_str());
  }
}

char** completer(const char* text, int start, int end) {
  // Don't do filename completion even if our generator finds no matches.
//   rl_attempted_completion_over = 1;

  // Note: returning nullptr here will make readline use the default filename
  // completer.
  return rl_completion_matches(text, completion_generator);
}



int main() {
  // Flush after every std::cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

    rl_attempted_completion_function = completer;

    rl_bind_key('\t', rl_complete);
    // Enable history with persistence
    using_history();
    stifle_history(1000);  // Limit history to 1000 entries
    load_history();  // Load history from file on startup

    while (true) {
        
        string tem = "Taran@Shell:" + get_current_directory() + "$ ";
        
        char* inp = readline(tem.c_str());

        if (inp && *inp) { 
            add_history(inp);
            save_history();  // Persist history after each command
        }
        string input = inp ? string(inp) : "";
        
        // Check if input contains a pipe - if so, use pipeline execution
        if (has_pipe(input)) {
            execute_pipeline(input);
            free(inp);
            continue;
        }
        
       // Use quote-aware tokenizer
        auto [ok, tokens] = tokenize(input);
        if (!ok) continue;

        if (tokens.empty()) continue;

        auto texts = to_texts(tokens);
        vector<string> words = texts.first.first;
        string redirect_loc = texts.first.second;
        bool append_mode = texts.second; 
        bool redir = !redirect_loc.empty();

        // Parse stderr redirection (2> or 2>>) from words into stderr_path, and remove it from argv copy
        vector<string> argv_no_stderr = words;
        optional<string> stderr_path;
        string parse_err;
        auto result = parse_stderr_redirection(argv_no_stderr, stderr_path, parse_err);
        bool parse_ok = result.first;
        bool stderr_append = result.second;
        if (!parse_ok) {
            if (!parse_err.empty()) cerr << parse_err << endl;
            continue;
        }
        if (argv_no_stderr.empty()) continue;

        string cmd = argv_no_stderr[0];

        if (cmd == EXIT) {
            if (words.size() == 1) return 0;
            else if (words[1] == "0") return 0;
            else if (words[1] == "1") return 1;
        }
        
        if (cmd == ECHO) {
            if (!redir) with_stderr_redir(stderr_path, stderr_append, [&]{ cout << echo_func(argv_no_stderr); });
            else {
                string echo_out = echo_func(argv_no_stderr);
                write_to_file(redirect_loc, echo_out, append_mode);
            }
        }
        else if (cmd == CD && argv_no_stderr.size() == 2) with_stderr_redir(stderr_path, stderr_append, [&]{ change_dir(tokens); });
        else if (cmd == SHELL_NAME && argv_no_stderr.size() == 1) {
            string sh_name = "Taran's Shell running";
            if (redir) write_to_file(redirect_loc, sh_name + "\n", append_mode);
            else with_stderr_redir(stderr_path, stderr_append, [&]{ cout << sh_name << endl; });
        }
        else if (cmd == PWD && argv_no_stderr.size() == 1) {
            if (!redir) with_stderr_redir(stderr_path, stderr_append, [&]{ cout << get_current_directory() << endl; });
            else {
                string cwd = get_current_directory() + "\n";
                write_to_file(redirect_loc, cwd, append_mode);
            }
        }
        else if (cmd == TYPE) {
            if (!redir) with_stderr_redir(stderr_path, stderr_append, [&]{ cout << type_func(argv_no_stderr); });
            else {
                string type_out = type_func(argv_no_stderr);
                write_to_file(redirect_loc, type_out, append_mode);
            }
        }
        else if (cmd == HISTORY) {
            if (argv_no_stderr.size() == 3 && argv_no_stderr[1] == "-w") {
                // history -w <filepath>
                with_stderr_redir(stderr_path, stderr_append, [&]{ history_write(argv_no_stderr[2]); });
            } else if (argv_no_stderr.size() == 3 && argv_no_stderr[1] == "-r") {
                // history -r <filepath>
                with_stderr_redir(stderr_path, stderr_append, [&]{ history_read(argv_no_stderr[2]); });
            } else if (argv_no_stderr.size() == 3 && argv_no_stderr[1] == "-a") {
                // history -a <filepath>
                with_stderr_redir(stderr_path, stderr_append, [&]{ history_append(argv_no_stderr[2]); });
            } else {
                // history or history <n>
                int limit = -1;
                if (argv_no_stderr.size() == 2) {
                    try {
                        limit = stoi(argv_no_stderr[1]);
                    } catch (...) {
                        limit = -1;
                    }
                }
                with_stderr_redir(stderr_path, stderr_append, [&]{ history_func(limit); });
            }
        }
        else if (builtin_set.find(cmd) == builtin_set.end()) {
            auto [okRun, outStr] = exec_capture_stdout(argv_no_stderr, stderr_path, stderr_append);
            if (okRun) {
                if (redir) write_to_file(redirect_loc, outStr, append_mode);
                else {
                    cout << outStr;
                    if (!outStr.empty() && outStr.back() != '\n') {
                        cout << '\n';
                    }
                }
                
            } else {
                cerr << cmd << ": not found" << endl;
            }
        } else cout<< input << ": command not found" << endl;


        free(inp);
    }

    return 0;
}
