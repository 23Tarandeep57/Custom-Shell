#include <bits/stdc++.h>
#include <sys/wait.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <optional>
using namespace std;

string EXIT = "byee";
string ECHO = "echo";
string TYPE = "type";
string PWD = "pwd";
string SHELL_NAME = "shell_name";
string CD = "cd";



set<string> builtin_set= {"byee", "echo", "type", "pwd", "shell_name", "cd"}; 


struct Token {
    string text;
    bool had_single_quotes = false;
    bool had_double_quotes = false;
};


bool parse_stderr_redirection(vector<string>& argv, optional<string>& stderr_path, string& err) {
    stderr_path.reset();
    err.clear();
    if (argv.empty()) return true;

    vector<string> out;
    for (size_t i = 0; i < argv.size(); ++i) {
        const string& t = argv[i];
        if (t == "2>") {
            if (i + 1 >= argv.size()) {
                err = "redirection: missing file after 2>";
                return false;
            }
            stderr_path = argv[i + 1];
            ++i; 
            continue;
        }
        out.push_back(t);
    }
    argv.swap(out);
    return true;
}


void with_stderr_redir(const optional<string>& stderr_path, const function<void()>& body) {
    if (!stderr_path) { body(); return; }
    int saved = dup(STDERR_FILENO);
    if (saved == -1) { body(); return; }

    int fd = open(stderr_path->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
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

pair<vector<string>, string> to_texts(const vector<Token>& toks) {
    vector<string> out;
    string redirect_loc;
    bool redirect_output = false;
    for (const auto& t : toks) {
        if (t.text == "1>" || t.text == ">") {
            redirect_output = true;
            continue;
        }
        if (redirect_output) {
            redirect_loc = t.text;
            continue;
        }
        else out.push_back(t.text);
    }
    return {out, redirect_loc};
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


pair<bool, string> exec_capture_stdout(const vector<string>& words, const optional<string>& stderr_path = nullopt) {
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

        if (stderr_path) {
            int efd = open(stderr_path->c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
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


void write_to_file(const string& filename, const string& content) {
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << content;
        outfile.close();
    } else {
        cerr << "Error opening file: " << filename << endl;
    }
}

int main() {
  // Flush after every std::cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

    while (true) {
        cout << "$";
        
        string input;
        getline(cin, input);

       // Use quote-aware tokenizer
        auto [ok, tokens] = tokenize(input);
        if (!ok) continue;

        if (tokens.empty()) continue;

        auto texts = to_texts(tokens);
        vector<string> words = texts.first;
        string redirect_loc = texts.second; 
        bool redir = !redirect_loc.empty();

        // Parse stderr redirection (2>) from words into stderr_path, and remove it from argv copy
        vector<string> argv_no_stderr = words;
        optional<string> stderr_path;
        string parse_err;
        if (!parse_stderr_redirection(argv_no_stderr, stderr_path, parse_err)) {
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
            if (!redir) with_stderr_redir(stderr_path, [&]{ cout << echo_func(argv_no_stderr); });
            else {
                string echo_out = echo_func(argv_no_stderr);
                write_to_file(redirect_loc, echo_out);
            }
        }
        else if (cmd == CD && argv_no_stderr.size() == 2) with_stderr_redir(stderr_path, [&]{ change_dir(tokens); });
        else if (cmd == SHELL_NAME && argv_no_stderr.size() == 1) {
            string sh_name = "Taran's Shell running";
            if (redir) write_to_file(redirect_loc, sh_name + "\n");
            else with_stderr_redir(stderr_path, [&]{ cout << sh_name << endl; });
        }
        else if (cmd == PWD && argv_no_stderr.size() == 1) {
            if (!redir) with_stderr_redir(stderr_path, [&]{ cout << get_current_directory() << endl; });
            else {
                string cwd = get_current_directory() + "\n";
                write_to_file(redirect_loc, cwd);
            }
        }
        else if (cmd == TYPE) {
            if (!redir) with_stderr_redir(stderr_path, [&]{ cout << type_func(argv_no_stderr); });
            else {
                string type_out = type_func(argv_no_stderr);
                write_to_file(redirect_loc, type_out);
            }
        }
        else if (builtin_set.find(cmd) == builtin_set.end()) {
            auto [okRun, outStr] = exec_capture_stdout(argv_no_stderr, stderr_path);
            if (okRun) {
                if (redir) write_to_file(redirect_loc, outStr);
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
    }

    return 0;
}
