#include <bits/stdc++.h>
#include <sys/wait.h>
#include <unistd.h> 
using namespace std;

string EXIT = "tata";
string ECHO = "echo";
string TYPE = "type";
string PWD = "pwd";
string SHELL_NAME = "shell_name";
string CD = "cd";



set<string> builtin_set= {"tata", "echo", "type", "pwd", "shell_name", "cd"}; 



void change_dir(vector<string>& words) {
    string path = words[1];

    if (path == "~"){
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

void echo_func(vector<string>& words){

	if (words[0] == ECHO) {
		for (int i = 1; i < words.size(); i++){
			cout << words[i] << " ";
		}
		cout << endl;
	}
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

void type_func(vector<string>& words) {	
	if (words.size() != 2) {
        cout << "Usage : type <command>" << endl;
        return;
    }

    string cmd = words[1];

    for (auto &str: builtin_set) {
        if (cmd == str) {
            cout << cmd << " is a shell builtin" << endl;
            return;
        }
    }

    auto result = find_executables(cmd);
    if (result.first) {
        cout << cmd << " is " << result.second << endl;
        return;
    }
    cout << cmd << ": not found" << endl;
}

int main() {
  // Flush after every std::cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

    while (true) {
        cout << "$";
        
        string input;
        getline(cin, input);

        stringstream ss(input);
        string segment;
        vector<string> words;
    
        while (getline(ss, segment, ' ')) {
            words.push_back(segment);
        }
    
        if (words.size() == 0) continue;
        
        string cmd = words[0];

        if (cmd == EXIT) {
            if (words.size() == 1) return 0;
            else if (words[1] == "0") return 0;
            else if (words[1] == "1") return 1;
        }
        
        if (cmd == ECHO) echo_func(words);
        else if (cmd == CD && words.size() == 2) change_dir(words);
        else if (cmd == SHELL_NAME && words.size() == 1) cout << "Taran's Shell running" << endl;
        else if (cmd == PWD && words.size() == 1) cout << get_current_directory() << endl;
        else if (cmd == TYPE) type_func(words);        
        else if (builtin_set.find(cmd) == builtin_set.end() && find_executables(cmd).first) execute_func(words);
        else cout<< input << ": command not found" << endl;
    }


    return 0;
}
