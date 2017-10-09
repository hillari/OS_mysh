#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

//read, parse, execute 
//1-Read line from user, store to buffer
//2-tokenize line, ***put commands into array 
//next- fork where needed, execute, pipe, etc.....

#define BUF_SIZE 1024
#define MAX_PARAMS 100
#define DELIMITERS " |<>\n"

//PROTOTYPES
void parseCmd();
void clearCmd();
void fillCmd();
void splitCmd();

static char* cwd;

char input='\0';
int bufferChars = 0;
char buffer[MAX_PARAMS]; //array for storing input from cmd line

char* myArgv[10]; //array of char pointers, for storing arguments
int myArgc = 0; //to keep track of number of arguments

void prompt() {
	printf("%s  ~%s $ ", getenv("LOGNAME"), getcwd(cwd, 1024));
}

void welcome() {
	printf("\n-------------------------------------------------\n");
	puts("Welcome to my shell. Enter a command, or type exit");
	printf("-------------------------------------------------\n");
    printf("\n\n");

}
void parseCmd() {
	//puts("In parse function");
   clearCmd();
 //gets each char and puts it into a buffer while not newline
	while((input != '\n')){
		buffer[bufferChars++] = input; 
		input = getchar(); 
	}
	buffer[bufferChars] = 0x00;

	splitCmd();

//***PRINT user input BUFFER***//
	// puts("User input buffer contents:");
	// 	for (int i = 0; i < sizeof(buffer); ++i) {
	// 		if(buffer[i] != '\0')
	// 	printf("[%d]: %c\n",i,buffer[i]);
	// }
	// puts("");
}

void fillCmd() {
	//puts("In fill function");
	char* bPtr; //token
	bPtr = strtok(buffer, DELIMITERS); 
	char* fileArg;
	int i;

	while(bPtr) { 
		//printf("bPtr= %s\n", bPtr);
		myArgv[myArgc] = bPtr; //put words into the array
		bPtr = strtok(NULL, DELIMITERS);
		myArgc++; //go to next array element
	}

//***PRINT arguments array***//
	puts("Argv array contents:");
	for (int i = 0; i < myArgc; i++) { //do i< myargC
		printf("[%d]: %s\n",i,myArgv[i]);
	}
}

void splitCmd() {
	int mark;
	for (int i = 0; i < MAX_PARAMS; ++i) {
		if(buffer[i] == '<') {
			mark = i;
			printf("%d\n", mark);
		}
	}
}

void clearCmd() {
	//puts("In clear function");
	//clear buffer
		for (int i = 0; i < sizeof(buffer); ++i) {
			buffer[i] = '\0';
	    }

        while (myArgc != 0) {
                myArgv[myArgc] = NULL; // delete the pointer to the string
                myArgc--;       // decrement the number of words in the command line
        }
        bufferChars = 0;       // erase the number of chars in the buffer
}

void execute(char* cmd[]) {
	//puts("In execute function");

	pid_t pid;
	int status;
	int bytesRead;
	char readBuffer[50]; //what size??
	int pidC, pidP; //for printing process IDs

	//if the first argument is string "exit", quit
    if (strcmp("exit", myArgv[0]) == 0) {
          exit(EXIT_SUCCESS);
    }

    if (strcmp("cd", myArgv[0]) == 0) { //if strcmp returns 0 they're ==
          if(chdir(myArgv[1]) == -1) {
          	printf("The directory %s does not exist\n", myArgv[1]);
          }
    }

//--create pipe
	int fd[2]; 
	if(pipe(fd) == -1) {
		perror("Pipe error");
		exit(1);
	}
//--create child process
	pid = fork();
	switch(pid) {

		case -1:
			perror("You forking failed");
			break;

		case 0: //+++child
		  // puts("I am the child");
		   pidP = getppid();
		   pidC = getpid();
		  // printf("Child here, my parent pid: %d, My pid: %d\n", pidP, pidC);
				if(close(fd[0]) == -1) { //close read end of pipe
					perror("Close fd[0]error");
					exit(1);
				}
				//redirect stdout to the write end of pipe 
				if(dup2(fd[1], STDOUT_FILENO) == -1) {
					perror("Couldn't dup2");
					exit(1);
				}
			//execute commands	
				puts("---Command results:---");
			if(execvp(*cmd, cmd) == -1) {
				perror("execvp error");
				exit(1);
			}	
			break;

		default: //+++parent
		//puts("I am the parent");
			if (close(fd[1]) == -1) {  //
				perror("Couldn't close");
				exit(1);
			}

			while(bytesRead = read(fd[0], readBuffer, 1) != 0) {
				if (bytesRead == -1) {
					perror("read error");
					exit(1);
				}

				write(1,readBuffer,1);
			}

			waitpid(pid, &status, WNOHANG);	
			if(WIFEXITED(status)){
			printf("Exited with status: %d\n", WEXITSTATUS(status));
		}
			pidC = getpid();
			//printf("Parent here, my pid:%d\n", pidC);
	}
	close(*fd);
}


int main(int argc, char* argv[]) {
	welcome();
	prompt();
 while(1) {

//input=getchar(), fixed issue where parse was executing before input entered
    input = getchar(); 
	switch(input) {
		case '\n':
		  prompt();
		  break;
		default:
		   parseCmd();
		   fillCmd();
		   execute(myArgv);
		   prompt();
		   break;
	}
 }//while
 return 0;
}//main
