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

// close all pipe fd
void close_all(int* pipes, int pipe_count){
	for(int i =0;i<pipe_count*2;i++){
		close(pipes[i]);
	}
}

// various errors
void parsing_error(){
	printf("ERROR: Parsing error.\n");
	exit(EXIT_FAILURE);
}

void exec_error(){
	printf("ERROR: Excecution error.\n");
	exit(EXIT_FAILURE);
}

void file_error() {
	printf("ERROR: File I/O error.\n");
	exit(EXIT_FAILURE);
}

void word_error() {
	printf("ERROR: Word(s) in command contains illegal character(s).\n");
	exit(EXIT_FAILURE);
}

// convert to c_str
void str_to_c(char* command, int command_size, string line){
	for(int i = 0;i<command_size;i++){
			command[i] = line.at(i);
		}
	command[command_size] = '\0';
}

// redirect to file and relevant checks
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

// back up stdout
void backup_stdout(int& stdout_backup){
	stdout_backup = dup(1);
}

// recover stdout from backup to printout error message in child processes.
void recover_stdout(int& stdout_backup){
	dup2(stdout_backup, 1);
	close(stdout_backup);
}

// check if the char is valid
bool is_valid_char(char c){
	bool is_digit = c >= 48 && c <58;
	bool is_letter = (c>=65 && c<91) || (c>=97 && c<123);
	bool is_symbol = c=='-' || c=='.' || c=='_';
	return is_digit||is_letter||is_symbol;
}

// check if the whole word is valid
bool is_valid_word(char* word){
	while((*word)!=0){
		if(!is_valid_char(*word)){
			return false;
		}
		word+=1;
	}
	return true;
}


int main ()
{
	// store the raw command
	string line;
	// exit string token
	string ext("exit");
	// ">" and "<" c_str token
	char greater[] = ">";
	char less[] = "<";

	// fetching command
	while(getline(cin,line)){
		// checking "exit"
		if(line.compare(ext)==0){
			break;
		}

		// checking length
		int command_size = line.size();
		if(command_size>MAXSIZE){
			printf("ERROR: The command is too long!\n");
			continue;
		}

		// fork a "sandbox" in parent for easy handling of errors
		if(fork()==0){
			// convert string to c_str
			char command[command_size+1];
			str_to_c(command, command_size, line);
			
			// count number of pipes (processes)
			int pipe_count = 0;
			for(int i=0; i<command_size; i++){
				char c = command[i];
				if(c=='|'){
					// handle "<command1>|<command2>" case
					if(i!=0 && i!=command_size-1 && (command[i-1]!=' ' or command[i+1]!=' '))
						parsing_error();
					pipe_count++;
				}
			}

			// store an array of c_str token_groups
			char* token_groups[pipe_count+1];
			int group_index = 0;
			// store the token_groups into token_groups array
			char* token_group;
			token_group = strtok (command,"|");
			while (token_group != NULL){
				token_groups[group_index] = token_group;
				group_index++;
				token_group = strtok (NULL, "|");
			}

			// to see if any token group is empty
			if(pipe_count != group_index-1){
				parsing_error();
			}

			// create pipes according to number of processes
			int pipes[pipe_count*2];
			for(int i = 0; i < pipe_count;i++){
				pipe(pipes+i*2);
			}
			
			// back up stdout
			int stdout_backup = -1;
			backup_stdout(stdout_backup);

			//for each token_group, excecute setup the redirections and do the work
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
				
				
				// split the token_group and store in tokens
				char* token = strtok(tg," ");
				while(token != NULL){
					// read tokens until a > or < is reached
					if(strcmp(token, greater) == 0 || strcmp(token, less)== 0){
						//if redirect1 is empty
						if (redirect1 == '\0') {
							redirect1 = token[0];
							break;
						}
					}
					
					if(!is_valid_word(token)){
						word_error();
					}

					tokens[token_index] = token;
					token_index++;
					token = strtok(NULL," ");
				}
				// close tokens
				tokens[token_index] = NULL;

				// too see if there is empty token group
				if(token_index==0)
					parsing_error();

				// fork a child process to do the work
				if(fork()==0){
					// set up flags to check if there are conflicted assignment
					bool stdin_assigned = false;
					bool stdout_assigned = false;

					// redirect pipes and stdIO
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
						// can only be file name
						token = strtok(NULL," ");
						if(token==NULL){
							recover_stdout(stdout_backup);
							parsing_error();
						} else {
							//check word validity
							if(!is_valid_word(token)){
								recover_stdout(stdout_backup);
								word_error();
							}
							file1 = token;
							// refer files to stdin/stdout according to redirect token
							if (redirect1=='<'){
								// check stdin assignment conflict
								if(stdin_assigned){
									recover_stdout(stdout_backup);
									parsing_error();
								}
								open_file_read(file1, inFile, stdin_assigned);
							} else {
								// check stdout assignemnt conflicts
								if(stdout_assigned){
									recover_stdout(stdout_backup);
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
									recover_stdout(stdout_backup);
									parsing_error();
								} else {
									// check word validity
									if(!is_valid_word(token)){
										recover_stdout(stdout_backup);
										word_error();
									}

									// check stdout assignment conflict
									if(stdout_assigned){
										recover_stdout(stdout_backup);
										parsing_error();
									}

									file2 = token;
									// open file to write to
									open_file_write(file2, outFile, stdout_assigned);
									// cannot have anymore tokens
									token = strtok(NULL," ");
									if(token!=NULL){
										recover_stdout(stdout_backup);
										parsing_error();
									}
								}
							} else {
								recover_stdout(stdout_backup);
								parsing_error();
							}
						}
					}
					// do the work
					execvp(*tokens, tokens);
					// if this line is reach, then execution is not successful
					recover_stdout(stdout_backup);
					exec_error();
				}

			}
	        
	        // close all pipes in the parent process
	        close_all(pipes,pipe_count);

	        // wait for child processes to exit
	        for(int i = 0;i<group_index;i++){
				wait(NULL);
			}
		}
		// wait for the sandbox to exit
		wait(NULL);
	}
	return 0;
}