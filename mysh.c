#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

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
int splitCmd();

static char* cwd;
char input='\0';
int buffStuff = 0;
char buffer[MAX_PARAMS]; //array for storing input from cmd line
char* myArgv[10]; //array of char pointers, for storing arguments
int myArgc = 0; //to keep track of number of arguments

char* fileArgs[5];
char* fileArgNum;


void prompt() {
	printf("%s  ~%s $ ", getenv("LOGNAME"), getcwd(cwd, 1024));
}

void welcome() {
	printf("\n-------------------------------------------------\n");
	puts("Welcome to my shell. Enter a command, or type exit");
	printf("-------------------------------------------------\n");
    printf("\n\n");

}

/**
* Grab user input and store into a buffer
*/
void parseCmd() {
   clearCmd();
 //gets each char and puts it into a buffer while not newline
	while((input != '\n')){
		buffer[buffStuff++] = input; 
		input = getchar(); 
	}
	buffer[buffStuff] = 0x00; //set to 0
	splitCmd();
}

/**
* Look for redirects and pipe
*/
int splitCmd() {
	int descriptor;
	int mark;
	for (int i = 0; i < MAX_PARAMS; ++i) {
		if(buffer[i] == '>') { //out
			puts("found the mark");
			mark = i;
			fileArgNum = fileArgs[i]; //storing the filename for redirect
			descriptor = 1;
		}
			else if(buffer[i] == '<') { //in
			puts("found the mark");
			mark = i;
			descriptor = 0;
		}

		//NEED TO make separate arrays for arg1 and arg2
			else if (buffer[i] == '|') { //pipe
			puts("found the mark");
			mark = i;
			descriptor = 2;
		}
	}
	printf("Mark is: %d\n", mark);

	return descriptor;
}

/**
* Take the buffer with chars in it and tokenize and store to char array
*/
void fillCmd() {
	//puts("In fill function");
	char* bPtr; //token
	bPtr = strtok(buffer, DELIMITERS); 
	char* fileArg;
	int i;

	while(bPtr) { 
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


/**
* Pretty self explanatory
*/
void clearCmd() {
	//clear buffer
		for (int i = 0; i < sizeof(buffer); ++i) {
			buffer[i] = '\0';
	    }

        while (myArgc != 0) {
             myArgv[myArgc] = NULL; // delete the pointer to the string
             myArgc--;       // decrement the number of words in the command line
        }
        buffStuff = 0;       // erase the number of chars in the buffer
}

/**
* Pipe, fork and execute. This should be split up into separate functions
* I just didn't have time to make it neater
*/
void execute(char* cmd[], int fileDescriptor) {
	pid_t pid;
	int status;
	int bytesRead;
	char readBuffer[50]; //what size??
	int pidC, pidP; //for printing process IDs

	//if the first argument is string "exit", quit
    if (strcmp("exit", myArgv[0]) == 0) { //if strcmp returns 0 they're ==
          exit(EXIT_SUCCESS);
    }

    //this works, but for some reason gives me execvp error
    if (strcmp("cd", myArgv[0]) == 0) { 
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
			if(close(fd[0]) == -1) { //close read end of pipe
				perror("Close fd[0]error");
				exit(1);
			}

				//redirect stdout to the write end of pipe 
			if (splitCmd() == 1) { //then it's an out command
				fileDescriptor = open(fileArgNum, O_CREAT | O_TRUNC | O_WRONLY, 0600);                                        // open file for read only (it's STDIN)
      			dup2(fileDescriptor, STDOUT_FILENO);
        		close(fileDescriptor);
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
			if (close(fd[1]) == -1) {  //
				perror("Couldn't close");
				exit(1);
			}

			//read from fd[0] (child), and write to stdout
			while(bytesRead = read(fd[0], readBuffer, 1) != 0) {
				if (bytesRead == -1) {
					perror("read error");
					exit(1);
				}
				write(1,readBuffer,1);
			}
			//wait for child to die and print exit status
			waitpid(pid, &status, WNOHANG);	
			if(WIFEXITED(status)){
			printf("Exited with status: %d\n", WEXITSTATUS(status));
		}
	}
	close(*fd);
}

int main(int argc, char* argv[]) {
	welcome();
	prompt();
	int descriptor = splitCmd(); //determine how to execute

 while(1) {
    input = getchar(); 
	switch(input) {
		case '\n':
		  prompt();
		  break;
		default:
		   parseCmd();
		   fillCmd();
		   execute(myArgv, descriptor);
		   prompt();
		   break;
	}
 }//while
 return 0;
}//main
