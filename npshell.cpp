#include <iostream>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include <stdio.h>

#define NUMPIPEMAX 1001
#define ORDINARYPIPEMAX 2

using namespace std;

struct pipeStruct {
    pid_t pid = -1;
    // ordinary pipe
    vector<string> arg;
    bool fileRedirection = false;
    string fileName = "";

    //number pipe
    int numberPipe = -1;
    int numberPipeErr = -1;
    int numberPipeNo = -1;
    int squareNum = -1;
    bool square = false;
};

vector<pipeStruct> numberArg;
int numberPipe[NUMPIPEMAX][2];
int numberPipeNo;
int ifPipe[NUMPIPEMAX];
int presquare;

int numPipeNo(int num) {
    return num % NUMPIPEMAX;
}

void parseInput(string input, vector<pipeStruct>& structArg) {
    int words = 1;
    vector<string> arg;
    stringstream ss(input);
    string word;
    pipeStruct tmpStruct;
    int tmpNumberPipeNo = numberPipeNo;

    if (ss.str().empty()) {
        return;
    }

    while (ss >> word) {
        if (word == "|") {
            tmpStruct.arg = arg;
            structArg.push_back(tmpStruct);
            tmpStruct.arg.clear();
            arg.clear();
            words++;
        } else if (word[0] == '|') {
            // |number
            tmpStruct.arg = arg;
            arg.clear();
            words++;
            int pipeNum = stoi(word.substr(1));
            tmpStruct.numberPipe = pipeNum;
            int tmpNum = pipeNum + tmpNumberPipeNo;
            tmpStruct.numberPipeNo = numPipeNo(tmpNum);
            structArg.push_back(tmpStruct);
            numberArg.push_back(tmpStruct);
            tmpStruct.arg.clear();
            tmpStruct.numberPipe = -1;
            tmpStruct.numberPipeNo = -1;
            tmpNumberPipeNo++;
        } else if (word[0] == '!') {
            // !number
            tmpStruct.arg = arg;
            arg.clear();
            words++;
            int pipeNum = stoi(word.substr(1));
            tmpStruct.numberPipeErr = pipeNum;
            int tmpNum = numPipeNo(pipeNum + tmpNumberPipeNo);
            tmpStruct.numberPipeNo = tmpNum;
            structArg.push_back(tmpStruct);
            numberArg.push_back(tmpStruct);
            tmpStruct.arg.clear();
            tmpStruct.numberPipeErr = -1;
            tmpStruct.numberPipeNo = -1;
            tmpNumberPipeNo++;
        } else if (word == ">") {
            tmpStruct.fileRedirection = true;
        } else if (tmpStruct.fileRedirection) {
            tmpStruct.fileName = word;
        } else if (tmpStruct.square) {
            tmpStruct.squareNum = stoi(word);
            arg.push_back(word);
            tmpStruct.arg = arg;
            structArg.push_back(tmpStruct);
            arg.clear();
        } else if (word == "square") {
            tmpStruct.square = true;
            arg.push_back(word);
        }
        else {
            arg.push_back(word);
        }
    }

    if (!arg.empty()) {
        tmpStruct.arg = arg;
        structArg.push_back(tmpStruct);
        arg.clear();
    } else {
        words--;
    }

    for (int i = 0; i < numberArg.size(); i++) {
        if (ifPipe[numberArg[i].numberPipeNo] == -1) {
            ifPipe[numberArg[i].numberPipeNo] = 1;
        }
    }
}

bool executeBuiltInCommands(vector<pipeStruct>& structArg, int words) {
    if (words == 1 && structArg[0].arg[0] == "setenv") {
        if (structArg[0].arg.size() == 1) {
            return true;
        }
        else if (structArg[0].arg.size() == 2) {
            setenv(structArg[0].arg[1].c_str(), "", 1);
        } else {
            setenv(structArg[0].arg[1].c_str(), structArg[0].arg[2].c_str(), 1);
        }
        return true;
    } else if (words == 1 && structArg[0].arg[0] == "printenv") {
        if (structArg[0].arg.size() == 1) {
            return true;
        } else {
            if (getenv(structArg[0].arg[1].c_str()) != NULL) {
                cout << getenv(structArg[0].arg[1].c_str()) << endl;
            }
        }
        return true;
    }
    return false;
}

int checkNumPipe(int num) {
    for (int i = 0; i < numberArg.size(); i++) {
        if (numberArg[i].numberPipeNo == num) {
            return i;
        }
    }
    return -1;
}

bool checkInRange(int num, int front, int behind) {
    if (front < behind) {
        if (front <= num && num < behind) {
            return true;
        } else {
            return false;
        }
    } else {
        if (front <= num && num < NUMPIPEMAX) {
            return true;
        } else if (0 <= num && num < behind) {
            return true;
        } else {
            return false;
        }
    }
}


void forkProcess(vector<pipeStruct>& structArg, int words, int pipes[][2]) {
    pid_t pid;
    bool isFirst = true;
    for (int i = 0; i < words; i++) {

        if (i < words - 1) {
            if (pipe(pipes[i % ORDINARYPIPEMAX]) == -1) {
                cerr << "pipe fail" << endl;
                exit(EXIT_FAILURE);
            }
        }

        if (i != 0) {
            if (structArg[i - 1].numberPipe >= 1 || structArg[i - 1].numberPipeErr >= 1) {
                isFirst = true;
                numberPipeNo++;
            }
        }

        int inNumPipe = checkNumPipe(numberPipeNo);
        int tmpNum = numberPipeNo;
        
        // number pipe
        if (ifPipe[structArg[i].numberPipeNo] == 1) {
            // cout << j << endl;
            if (pipe(numberPipe[structArg[i].numberPipeNo]) == -1) {
                cerr << i << endl;
                cerr << "pipe creation failed" << endl;
                exit(EXIT_FAILURE);
            }
            ifPipe[structArg[i].numberPipeNo] = 0;
        }

        while ((pid = fork()) < 0);
        if (pid == 0) {
            // child process
            // STDIN
            // if numberpipe has and count = 0
            if (inNumPipe != -1 && isFirst) {
                dup2(numberPipe[numberArg[inNumPipe].numberPipeNo][0], STDIN_FILENO);
                // cout << "!numbers empty end" << endl;
            } else if (i > 0) {
                dup2(pipes[(i - 1) % ORDINARYPIPEMAX][0], STDIN_FILENO);
            }
            
            //STDOUT
            if (structArg[i].fileRedirection) {
                int fd = open(structArg[i].fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) {
                    cerr << "open failed" << endl;
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } // else if ((structArg[i].numberPipe > 1 || structArg[i].numberPipeErr > 1) || ((structArg[i].numberPipe == 1 || structArg[i].numberPipeErr == 1) && i == words - 1)) {
            else if (structArg[i].numberPipe >= 1 || structArg[i].numberPipeErr >= 1) {
                // number pipe

                if (dup2(numberPipe[structArg[i].numberPipeNo][1], STDOUT_FILENO) == -1) {
                    perror("dup2 failed for STDOUT");
                    exit(EXIT_FAILURE);
                }
                if (structArg[i].numberPipeErr != -1) {
                    if (dup2(numberPipe[structArg[i].numberPipeNo][1], STDERR_FILENO) == -1) {
                        perror("dup2 failed for STDOUT");
                        exit(EXIT_FAILURE);
                    }
                }
                numberPipeNo++;

            } else if (i < words - 1) {
                dup2(pipes[i % ORDINARYPIPEMAX][1], STDOUT_FILENO);
            }

            // close pipes
            if (i > 0) close(pipes[(i - 1) % ORDINARYPIPEMAX][0]);
            if (i < words - 1) close(pipes[(i % ORDINARYPIPEMAX)][1]);
            for (int j = 0; j < NUMPIPEMAX; j++){
                // cout << j << endl;
                if (ifPipe[j] == 0) {
                    close(numberPipe[j][0]);
                    close(numberPipe[j][1]);
                }

            }
            // exec
            char* argChar[structArg[i].arg.size() + 1];
            for (int j = 0; j < structArg[i].arg.size(); j++) {
                argChar[j] = strdup(structArg[i].arg[j].c_str());
            }
            argChar[structArg[i].arg.size()] = nullptr;

            execvp(argChar[0], argChar);
            cerr << "Unknown command: [" << structArg[i].arg[0] << "]." << endl;
            for (int j = 0; j < structArg[i].arg.size(); j++) {
                free(argChar[j]);
            }
            exit(EXIT_SUCCESS);
        } else {
            structArg[i].pid = pid;
            isFirst = false;
            if (i > 0) close(pipes[(i - 1) % ORDINARYPIPEMAX][0]);
            if (i < words - 1) close(pipes[i % ORDINARYPIPEMAX][1]);
            if (inNumPipe != -1) {
                numberArg[inNumPipe].pid = pid;
                close(numberPipe[numberArg[inNumPipe].numberPipeNo][0]);
                close(numberPipe[numberArg[inNumPipe].numberPipeNo][1]);
                ifPipe[numberArg[inNumPipe].numberPipeNo] = -1;
            }
        }
    }
    if ((structArg[words - 1].numberPipe == -1 && structArg[words - 1].numberPipeErr == -1)) {
        waitpid(pid, nullptr, 0);
    }
    for (int i = 0; i < numberArg.size(); i++) {
        if (numberArg[i].numberPipeNo == numberPipeNo) {
            numberArg.erase(numberArg.begin() + i);
            i--;
        }
    }
}

void npshell(string input) {
    int numPipeSize = numberArg.size();
    vector<pipeStruct> structArg;
    // read out input
    parseInput(input, structArg);
    if (structArg.empty()) return;
    int words = structArg.size();

    // build-in commands
    if (executeBuiltInCommands(structArg, words)) {
        return;
    }
    for (int i = 0; i < structArg.size(); i++) {
        if (structArg[i].square) {
            presquare = structArg[i].squareNum * structArg[i].squareNum;
            // cout  << presquare << endl;
            return;
        }
    }
    

    // pipe
    int pipes[ORDINARYPIPEMAX][2];

    // fork process
    forkProcess(structArg, words, pipes);

    numberPipeNo++;
    numPipeNo(numberPipeNo);
}

void handler(int signo){
	while(waitpid(-1, nullptr, WNOHANG) > 0);
}

void init() {
    signal(SIGCHLD, handler);
    setenv("PATH", "bin:.", 1);
    numberArg.clear();
    numberPipeNo = 0;
    presquare = 0;
    for (int i = 0; i < NUMPIPEMAX; i++) {
        numberPipe[i][0] = -1;
        numberPipe[i][1] = -1;
        ifPipe[i] = -1;
    }
}

int main() {
    string input;
    init();
    while (true) {
        cout << "% ";
        if (presquare != -1) {
            cout << '[' << presquare << "] "; 
        }
        presquare = -1;
        getline(cin, input);
        if (input == "exit" || cin.eof()) break;
        npshell(input);
    }
    return 0;
}
