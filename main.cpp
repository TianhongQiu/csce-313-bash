#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parser.cpp"

using namespace std;

char** increase_arr_size(char** arr, int arr_size){
    arr_size *= 2;
    char** temp = new char* [arr_size];
    for (int j = 0; j < arr_size; j++){
        temp[j] = arr[j];
    }
    arr = temp;
    delete [] temp;
    return arr;
}

char* convert_to_char_arr(string part){
    char* ca = new char[part.size()+1];
    copy(part.begin(), part.end(), ca);
    ca[part.size()] = '\0';
    return ca;
}

void safe_close(int fd){
    if (close(fd) < 0) perror("close");
}

void safe_exec(char** command){
    execvp(command[0], command);
}

void safe_wait(pid_t pid, int status){
    if (waitpid(pid, &status, WUNTRACED) < 0) perror("wait");
}

int main(int argc, char** argv){
    bool prompt = true;
    string line;
    int fd [2];
    int status;
    pid_t pid;
    char* last_directory = nullptr;

    int i = 0;
    while (argv[i] != NULL){
        if (strcmp(argv[i], "-t") == 0) prompt = false;
        i++;
    }

    while (1){
        if (prompt) cout << "$> ";
        getline(cin, line);
        vector<string> parsed_line = parse_line(line);
        
        //Declaration of varibles needed throughout the process
        int arr_size = 8;
        char** command = new char* [arr_size];
        int i = 0;
        int stdin = dup(0);
        int stdout = dup(1);
        bool piped = false;
        bool redirected = false;
        bool background = false;
        int fds[2];

        for (int j = 0; j < parsed_line.size(); j++){
            // -----------------------------------------------------
            //PIPING
            if (parsed_line[j] == "|"){
                if (!redirected){
                    pipe(fds);
                    pid = fork();
                    if (pid == 0){
                        dup2(fds[1], 1);
                        safe_close(fds[0]);
                        command[i] = NULL;
                        safe_exec(command);
                    }
                    else if (pid < 0) perror("fork");
                    else{
                        safe_wait(pid, status);
                        safe_close(fds[1]);
                        dup2(fds[0],0);
                    }
                }
                else
                    redirected = false;

                //Resetting parameters back to what they were
                int arr_size = 8;
                char** temp = new char* [arr_size];
                delete [] command;
                i = 0;
                command = temp;
                piped = true;
            }
            // -----------------------------------------------------
            //REDIRECT IN
            else if (parsed_line[j] == "<"){
                int fds1[2];
                if (i >= arr_size){
                    command = increase_arr_size(command, arr_size);
                }
                command[i] = NULL;
                redirected = true;

                if (j < parsed_line.size() - 2) pipe(fds);
                pid = fork();
                //Running the last process
                if (pid == 0){
                    int fd0 = open(parsed_line[++j].c_str(), O_RDONLY, 0);
                    dup2(fd0, STDIN_FILENO);
                    safe_close(fd0);
                    if (j < parsed_line.size() - 2){
                        dup2(fds[1], 1);
                        safe_close(fds[0]);
                    }
                    safe_exec(command);
                }
                else if (pid < 0) perror("fork");
                else{
                    safe_wait(pid, status);
                    if (j < parsed_line.size() - 2){
                        safe_close(fds[1]);
                        dup2(fds[0],0);
                    }
                }

                //Resetting parameters back to what they w  ere
                int arr_size = 8;
                char** temp = new char* [arr_size];
                delete [] command;
                i = 0;
                command = temp;
            }
            // -----------------------------------------------------
            //REDIRECT OUT
            else if (parsed_line[j] == ">"){
                if (i >= arr_size){
                    command = increase_arr_size(command, arr_size);
                }
                safe_close(1);
                command[i] = NULL;
                FILE* file;
                file = fopen(parsed_line[++j].c_str(), "w");

                pid = fork();
                //Running the last process
                if (pid == 0){
                    safe_exec(command);
                }
                else if (pid < 0) perror("fork");
                else{
                    if (!background) safe_wait(pid, status);
                    else signal(SIGCHLD,SIG_IGN);
                }
                fclose(file);             

                //Resetting parameters back to what they were
                int arr_size = 8;
                char** temp = new char* [arr_size];
                delete [] command;
                i = 0;
                command = temp;
            }
            else if (parsed_line[j] == "cd"){
                if (strcmp(parsed_line[j + 1].c_str(), "-") == 0) chdir(last_directory);
                else{
                    char buffer [FILENAME_MAX];
                    last_directory = getcwd(buffer, FILENAME_MAX);
                    chdir(parsed_line[++j].c_str());
                }
            }
            // ----------------------------------------------------
            // SPECIFYING A BACKGROUND PROCESS
            else if (parsed_line[j] == "&"){
                background = true;
            }
            // -----------------------------------------------------
            // EXITING SHELL
            else if (parsed_line[j] == "exit"){
                cout << "Exiting Shell..." << endl;
                exit(1);
            }
            // -----------------------------------------------------
            // FORMING A COMMAND 
            else{
                char* ca = convert_to_char_arr(parsed_line[j]);

                if (i >= arr_size){
                    command = increase_arr_size(command, arr_size);
                }
                command[i++] = ca;
                if (parsed_line[j] == "ls"){
                    if (i >= arr_size){
                        command = increase_arr_size(command, arr_size);
                    }
                    command[i++] = convert_to_char_arr("--color=auto");
                }
            }
        }

        if (i >= arr_size){
            command = increase_arr_size(command, arr_size);
        }
        command[i] = NULL;
        dup2(stdout, 1);
        safe_close(stdout);

        pid = fork();
        //Running the last process
        if (pid == 0){
            safe_exec(command);
        }
        else if (pid < 0) perror("fork");
        else{
            if (!background) safe_wait(pid, status);
            else signal(SIGCHLD,SIG_IGN);
            if (piped || redirected) safe_close(fds[0]);
            i = 0;
            delete [] command;
        }

        dup2(stdin, 0);
        safe_close(stdin);
    }
    return 0;
}