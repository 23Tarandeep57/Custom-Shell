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
	if (words.size() == 2) {
		for (auto str: builtin_vec) {
			if (str == words[1]) {
				cout << str << " is a builtin command"<< endl;
				return;
			}
		}
	}
	for (int i = 1; i < words.size(); i++) cout << words[i] << " ";
       cout << ": NOT FOUND" << endl;	
 }

int main() {
  // Flush after every std::cout / std:cerr
    cout << unitbuf;
    cerr << unitbuf;

    cout << "$ ";
  
    string input;
    getline(cin, input);
    stringstream ss(input);
    string segment;
    vector<string> words;

    while (getline(ss, segment, ' ')) {
        words.push_back(segment);
    }



    if (words.size() == 0) main();    

    else if (words[0] == ECHO) echo_func(words);
    else if (words[0] == TYPE) type_func(words);
    else if (words.size() == 1) 
        if (words[0] == EXIT) return 0;
        
    else if (words.size() == 2) {
        if (words[0] == EXIT && words[1] == "0") return 0;
        else if (words[0] == EXIT && words[1] == "1") return 1;
    }
    
    else cout<< input << ": command not found" << endl;

    main();
}
