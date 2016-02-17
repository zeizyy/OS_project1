#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#define MAXSIZE 80
using namespace std;
// Add one !!!

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

void invalid_command(){
	printf("Invalid Command\n");
	exit(0);
}


int main ()
{
	// store the raw command
	string line;
	while(getline(cin,line)){
		int command_size = line.size();
		if(command_size>MAXSIZE){
			printf("Invalid Command\n");
			exit(0);
		}

		char command[command_size+1];
		for(int i = 0;i<command_size;i++){
			command[i] = line.at(i);
		}
		command[command_size] = '\0';
		
		// to see if the command contains '|'
		bool contain_pipe = false;
		int outputr_count = 0;
		int inputr_count = 0;
		int pipe_count = 0;
		for(int i=0; i<command_size; i++){
			char c = command[i];
			if(c=='|'){
				contain_pipe = true;
				pipe_count++;
			}
			if(c=='>'){
				outputr_count++;
			}
			if(c=='<'){
				inputr_count++;
				if(outputr_count>0){
					invalid_command();
				}
			}
		}
		// if more than one > or more than one <, then exit
		if(outputr_count>1 || inputr_count>1){
			invalid_command();
		}
		// store an array of token_groups (c_str)
		char* token_groups[pipe_count+1];
		int token_index = 0;
		// store the token_group into token_groups
		char* token_group;
		token_group = strtok (command,"|");
		if(contain_pipe){
			while (token_group != NULL)
			{
				printf("%s\n", token_group);
				token_groups[token_index] = token_group;
				token_index++;
				token_group = strtok (NULL, "|");

			}
		} else {
			token_groups[token_index] = token_group;
		}
		// array of token arrays
		// char** commands[pipe_count+1];
		int pipes[pipe_count*2];
		for(int i = 0; i < pipe_count;i++){
			pipe(pipes+i*2);
		}
		// stdin_assigned/stdout_assigned
		bool stdin_assigned = false;
		bool stdout_assigned = false;
		//for each token_group, tokenize it into tokens
		for(int i = 0;i<pipe_count+1;i++){
			// current token_group
			char* tg = token_groups[i];
			// token array
			char* tokens[100];
			int token_index=0;
			// redirects
			char redirect1='\0';
			char redirect2='\0';
			char* file1;
			char* file2;
			int inFile = -1;
			int outFile = -1;
			// std
			// split
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
				printf("Token: %s\n", token);
				token = strtok(NULL," ");
			}
			// TODO: pan duan start with redirect
			// parse input/output rediretion
			if(redirect1!='\0'){
				token = strtok(NULL," ");
				printf("ll:%s\n",token);
				if(token==NULL || token[0]=='>'){
					invalid_command();
				} else {
					file1 = token;
					if (redirect1=='<'){
						inFile = open(file1, O_RDONLY);
						if (inFile < 0) {
							fprintf(stderr, "%s\n", "Error opening input file.");
						}
						stdin_assigned = true;
					} else {
						outFile = open(file1, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
						if (outFile < 0) {
							fprintf(stderr, "%s\n", "Error opening output file.");
						}
						stdout_assigned = true;
					}

				}
				token = strtok(NULL," ");
				if(token!=NULL){
					if(token[0]=='>'){
						token = strtok(NULL," ");
						if(token == NULL){
							invalid_command();
						} else {
							file2 = token;
							outFile = open(file2, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
							if (outFile < 0) {
								fprintf(stderr, "%s\n", "Error opening output file.");
							}
							stdout_assigned = true;
							token = strtok(NULL," ");
							if(token!=NULL){
								invalid_command();
							}
						}
					} else {
						invalid_command();
					}
				}
			}
			// TODO:
			// open the input file
			//if either > or < exist
			if (inFile > 0) {
				dup2(inFile, 0);
			}
			if (outFile > 0) {
				dup2(outFile, 1);
			}
			close(inFile);
			close(outFile);

			// do the work
			tokens[token_index] = NULL;
			if(fork()==0){
				if(i == 0){
					if(!stdout_assigned){
						dup2(pipes[i*2+1],1);
					}
				}else if(i==pipe_count){
					dup2(pipes[i*2-2],0);
				}else{
					if(stdin_assigned || stdout_assigned){
						invalid_command();
					}
					dup2(pipes[i*2-2],0);
					dup2(pipes[i*2+1],1);
					// close(pipes[i*2-1]);
				}
				close_all(pipes,pipe_count);
				execvp(*tokens, tokens);
			}
		}
		for(int i = 0;i<pipe_count+1;i++){
			wait();
		}
		// for(int i = 0;i<pipe_count+1;i++){
		// 	printf("%s\n", commands[i][0]); 
		// }
	}
	return 0;
}



	// while(true) {

	// }
 //  char str[] ="- This, a sample string.";
 //  char * pch;
 //  printf ("Splitting string \"%s\" into token_groups:\n",str);
 //  pch = strtok (str," ,.-");
 //  while (pch != NULL)
 //  {
 //    printf ("%s\n",pch);
 //    pch = strtok (NULL, " ,.-");
 //  }