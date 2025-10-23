#include <bits/stdc++.h>
using namespace std;

string EXIT = "exit";
string ECHO = "echo";
string TYPE = "type";

vector<string> builtin_vec = {"exit", "echo", "type"}; 

void echo_func(vector<string>& words){

	if (words[0] == ECHO) {
		for (int i = 1; i < words.size(); i++){
			cout << words[i] << " ";
		}
		cout << endl;
	}
}

void type_func(vector<string>& words) {	
	if (words.size() != 2) {
        cout << "Usage : Type <command>" << endl;
        return;
    }

    string cmd = words[1];

    for (auto &str: builtin_vec) {
        if (cmd == str) {
            cout << cmd << " is a shell builtin" << endl;
            return;
        }
    }

    char* path_env = getenv("PATH");
    if (path_env == NULL) {
        cout << cmd << " not found" << endl;
        return;
    }

    string path_str(path_env);
    stringstream ss(path_str);
    string dir;

    while (getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        string full_path = dir + "/" + cmd;
        if (access(full_path.c_str(), X_OK) == 0) {
            cout << cmd << " is " << full_path << endl;
            return;
        }
    }

    cout << cmd << ": not found" << endl;
}

int main() {
  // Flush after every std::cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

    while (true) {

        cout << "$ ";
      
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
        else if (cmd == TYPE) type_func(words);        
        else cout<< input << ": command not found" << endl;
    }


    return 0;
}
