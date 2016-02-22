#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#define MAXSIZE 80
using namespace std;

void print_arr(char* arr[], int size){
	for(int i =0;i<size;i++){
		printf("%s ", arr[i]);
	}
	printf("\n");
}

void close_all(int* pipes, int pipe_count){
	for(int i =0;i<pipe_count*2;i++){
		close(pipes[i]);
	}
}

void parsing_error(){
	printf("ERROR: Parsing error.");
	exit(EXIT_FAILURE);
}

void exec_error(){
	printf("ERROR: Excecution error.");
	exit(EXIT_FAILURE);
}

void file_error() {
	printf("ERROR: File I/O error.");
	exit(EXIT_FAILURE);
}

// convert to c_str
void str_to_c(char* command, int command_size, string line){
	for(int i = 0;i<command_size;i++){
			command[i] = line.at(i);
		}
	command[command_size] = '\0';
}

void open_file_write(char* file, int& outFile, bool& stdout_assigned){
	if(stdout_assigned){
		exit(EXIT_FAILURE);
	}
	stdout_assigned = true;
	outFile = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
	if (outFile < 0) {
		file_error();
	}
	if (outFile > 0) {
		dup2(outFile, 1);
		close(outFile);
	}
}

void open_file_read(char* file, int& inFile, bool& stdin_assigned) {
	if(stdin_assigned){
		exit(EXIT_FAILURE);
	}
	stdin_assigned = true;
	inFile = open(file, O_RDONLY);
	if (inFile < 0) {
		file_error();
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

bool is_valid_char(char c){
	bool is_digit = c >= 48 && c <58;
	bool is_letter = (c>=65 && c<91) || (c>=97 && c<123);
	bool is_symbol = c=='-' || c=='.' || c=='_';
	return is_digit||is_letter||is_symbol;
}

bool is_valid_word(char* word){
	while((*word)!=0){
		if(!is_valid_char(*word)){
			return false;
		}
		word+=1;
	}
	return true;
}

void exit_if_not_valid(char* word){
	if(!is_valid_word(word))
		parsing_error();
}


int main ()
{
	// store the raw command
	string line;
	int status_driver;
	int status;
	// exit token
	string ext("exit");
	// c_str for > and <
	char greater[] = ">";
	char less[] = "<";
	while(getline(cin,line)){
		if(line.compare(ext)==0){
			exit(0);
		}

		int command_size = line.size();
		if(command_size>MAXSIZE){
			printf("ERROR: The command is too long!\n");
			continue;
		}

		// fork a sandbox in parent
		if(fork()==0){

			// c_str for command
			char command[command_size+1];
			str_to_c(command, command_size, line);
			
			// count number of processes
			int pipe_count = 0;
			for(int i=0; i<command_size; i++){
				char c = command[i];
				if(c=='|'){
					if(i!=0 && i!=command_size-1 && (command[i-1]!=' ' or command[i+1]!=' '))
						parsing_error();
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
					if(strcmp(token, greater) == 0 || strcmp(token, less)== 0){ //edge cases: ">>>>" '<sdsewd' 
						//if redirect1 is empty
						if (redirect1 == '\0') {
							redirect1 = token[0];
							break;
						}
					}
					
					exit_if_not_valid(token);

					tokens[token_index] = token;
					token_index++;
					// printf("Token: %s\n", token);
					token = strtok(NULL," ");
				}
				// close tokens
				tokens[token_index] = NULL;

				// too see if there is empty token group
				if(token_index==0)
					parsing_error();

				// do the work
				if(fork()==0){
					// stdin_assigned/stdout_assigned
					bool stdin_assigned = false;
					bool stdout_assigned = false;

					if(pipe_count!=0){
						if(i == 0){
							dup2(pipes[i*2+1],1);
							stdout_assigned = true;
						}else if(i==pipe_count){
							dup2(pipes[i*2-2],0);
							stdin_assigned = true;
						}else{
							dup2(pipes[i*2-2],0);
							dup2(pipes[i*2+1],1);
							stdout_assigned = true;
							stdin_assigned = true;
						}
						close_all(pipes,pipe_count);
					}

					// parse input/output rediretion
					if(redirect1!='\0'){
						// can only be file
						token = strtok(NULL," ");
						if(token==NULL){
							recover_IO(stdin_backup, stdout_backup);
							parsing_error();
						} else {
							if(!is_valid_word(token)){
								recover_IO(stdin_backup, stdout_backup);
								parsing_error();
							}
							file1 = token;
							if (redirect1=='<'){
								if(stdin_assigned){
									recover_IO(stdin_backup, stdout_backup);
									parsing_error();
								}
								open_file_read(file1, inFile, stdin_assigned);
							} else {
								if(stdout_assigned){
									recover_IO(stdin_backup, stdout_backup);
									parsing_error();
								}
								open_file_write(file1, outFile, stdout_assigned);
							}

						}
						// can only be > or NULL
						token = strtok(NULL," ");
						if(token!=NULL){
							if(strcmp(token, greater)==0){
								// can only be file name
								token = strtok(NULL," ");
								if(token == NULL){
									recover_IO(stdin_backup, stdout_backup);
									parsing_error();
								} else {
									if(!is_valid_word(token)){
										recover_IO(stdin_backup, stdout_backup);
										parsing_error();
									}

									if(stdout_assigned){
										recover_IO(stdin_backup, stdout_backup);
										parsing_error();
									}
									file2 = token;
									open_file_write(file2, outFile, stdout_assigned);
									token = strtok(NULL," ");
									if(token!=NULL){
										recover_IO(stdin_backup, stdout_backup);
										parsing_error();
									}
								}
							} else {
								recover_IO(stdin_backup, stdout_backup);
								parsing_error();
							}
						}
					}
					execvp(*tokens, tokens);
					recover_IO(stdin_backup, stdout_backup);
					exec_error();
				}

			}
	        
	        close_all(pipes,pipe_count);
	        recover_IO(stdin_backup, stdout_backup);
	        bool has_error = false;
	        for(int i = 0;i<group_index;i++){
				wait(&status);
				if(status!=0){
					has_error = true;
				}
			}
			if(has_error){
				exit(EXIT_FAILURE);
			} else {
				exit(0);
			}
		}
		wait(&status_driver);
		if(status_driver!=0){
			printf("\n");
		}
	}
	return 0;
}