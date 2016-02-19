#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#define MAXSIZE 80
using namespace std;
// TODO:
// hard code parsing part: file name, word
void print_arr(char* arr[], int size){
	for(int i =0;i<size;i++){
		printf("%s ", arr[i]);
	}
	printf("\n");
}

void close_all(int* pipes, int pipe_count){
	for(int i =0;i<pipe_count;i++){
		close(pipes[i]);
	}
}

void parsing_error(){
	exit(1);
}

void exec_error(){
	exit(2);
}

// convert to c_str
void str_to_c(char* command, int command_size, string line){
	for(int i = 0;i<command_size;i++){
			command[i] = line.at(i);
		}
	command[command_size] = '\0';
}

void open_file_write(char* file, int& outFile){
	outFile = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
	if (outFile < 0) {
		fprintf(stderr, "%s\n", "Error opening output file.");
	}
	if (outFile > 0) {
		dup2(outFile, 1);
		close(outFile);
	}
}

void open_file_read(char* file, int& inFile) {
	inFile = open(file, O_RDONLY);
	if (inFile < 0) {
		fprintf(stderr, "%s\n", "Error opening input file.");
	}
	dup2(inFile, 0);
	close(inFile);
}

void backup_IO(int& stdin_backup, int& stdout_backup){
	stdin_backup = dup(0);
	stdout_backup = dup(1);
}

void recover_IO(int& stdin_backup, int& stdout_backup){
	dup2(stdin_backup, 0);
	dup2(stdout_backup, 1);
	close(stdin_backup);
	close(stdout_backup);
}


int main ()
{
	// store the raw command
	string line;
	int status_driver;
	int status;
	// exit token
	string ext("exit");
	while(getline(cin,line)){
		if(line.compare(ext)==0){
			printf("Bye!\n");
			exit(0);
		}
		int command_size = line.size();
			if(command_size>MAXSIZE){
				printf("Too long!\n");
			}

		// fork a sandbox in parent
		if(fork()==0){
			int command_size = line.size();
			if(command_size>MAXSIZE){
				parsing_error();
			}

			// c_str for command
			char command[command_size+1];
			str_to_c(command, command_size, line);
			
			// count number of processes
			int pipe_count = 0;
			for(int i=0; i<command_size; i++){
				char c = command[i];
				if(c=='|'){
					pipe_count++;
				}
			}

			// store an array of token_groups (c_str)
			char* token_groups[pipe_count+1];
			int group_index = 0;
			// store the token_group into token_groups
			char* token_group;
			token_group = strtok (command,"|");
			while (token_group != NULL){
				token_groups[group_index] = token_group;
				group_index++;
				token_group = strtok (NULL, "|");
			}

			// to see if the token group is empty
			if(pipe_count != group_index-1){
				parsing_error();
			}

			// create pipes
			int pipes[pipe_count*2];
			if(pipe_count!=0){
				for(int i = 0; i < pipe_count;i++){
					pipe(pipes+i*2);
				}
			}
			// stdin_assigned/stdout_assigned
			bool stdin_assigned = false;
			bool stdout_assigned = false;
			// back up stdin and out for the whole command
			int stdin_backup = -1;
			int stdout_backup = -1;
			backup_IO(stdin_backup,stdout_backup);

			//for each token_group, tokenize it into tokens
			for(int i = 0; i<group_index;i++){
				// current token_group
				char* tg = token_groups[i];
				// token array
				char* tokens[MAXSIZE];
				int token_index=0;
				// redirects and files
				char redirect1='\0';
				char* file1;
				char* file2;
				int inFile = -1;
				int outFile = -1;

				// split and store in tokens
				char* token = strtok(tg," ");
				while(token != NULL){
					// read tokens until a > or < is reached
					if(token[0]=='>' || token[0]=='<'){ //edge cases: ">>>>" '<sdsewd' 
						//if redirect1 is empty
						if (redirect1 == '\0') {
							redirect1 = token[0];
							break;
						}
					}
					tokens[token_index] = token;
					token_index++;
					// printf("Token: %s\n", token);
					token = strtok(NULL," ");
				}

				// empty token group
				if(token_index==0)
					parsing_error();

				// close tokens
				tokens[token_index] = NULL;
				// TODO: pan duan start with redirect !!!
				// parse input/output rediretion
				if(redirect1!='\0'){
					// can only be file !!!
					token = strtok(NULL," ");
					if(token==NULL){
						parsing_error();
					} else {
						file1 = token;
						if (redirect1=='<'){
							if(stdin_assigned)
								parsing_error();
							open_file_read(file1, inFile);
							stdin_assigned = true;
						} else {
							if(stdout_assigned)
								parsing_error();
							open_file_write(file1, outFile);
							stdout_assigned = true;
						}

					}
					// can only be >
					token = strtok(NULL," ");
					if(token!=NULL){
						if(token[0]=='>'){
							// can only be file name
							token = strtok(NULL," ");
							if(token == NULL){
								parsing_error();
							} else {
								if(stdout_assigned)
									parsing_error();
								open_file_write(file2, outFile);
								stdout_assigned = true;
								token = strtok(NULL," ");
								if(token!=NULL){
									parsing_error();
								}
							}
						} else {
							parsing_error();
						}
					}
				}

				// do the work
				if(fork()==0){
					if(pipe_count!=0){
						if(i == 0){
							if(stdout_assigned){
								parsing_error();
							}
							dup2(pipes[i*2+1],1);
						}else if(i==pipe_count){
							if(stdin_assigned){
								parsing_error();
							}
							dup2(pipes[i*2-2],0);
						}else{
							if(stdin_assigned || stdout_assigned){
								parsing_error();
							}
							dup2(pipes[i*2-2],0);
							dup2(pipes[i*2+1],1);
						}
						close_all(pipes,pipe_count);
					}
					execvp(*tokens, tokens);
					printf("HAHA\n");
					exec_error();
				}
			}
	        //close all pipes and if not valid command?
	        
	        close_all(pipes,pipe_count);
	        recover_IO(stdin_backup, stdout_backup);
	        int status2=0;
	        for(int i = 0;i<group_index;i++){
				wait(&status);
				if(status!=0){
					status2 = status;
				}
			}
			exit(status2);
		}
		wait(&status_driver);
		if(status_driver==1){
			printf("ERROR: Parsing error\n");
		} else if(status_driver==2){
			printf("ERROR: Parsing error\n");
		}
	}
	return 0;
}