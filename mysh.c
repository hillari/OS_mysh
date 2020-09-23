// Hillari M. Denny
// CSCE A321 - Operating Systems
// Fall 2017
//
// Update 9/23/2020
// This file implements a simple shell in c. The purpose of this exercise is to gain
// a better understanding of inter-process communication (IPC). Note there are some 
// minor bugs that were not worked out due to time constraints. Most notably, EOF (CTRL+D)
// seems to be a bit buggy. Perhaps someday I'll go back and debug this...The project
// could also use some structural re-organization (headers, etc). 


#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define BUF_SIZE 4098
#define MAX_PARAMS 100
#define DELIMITERS " \n\0"

//PROTOTYPES
void parseCmd();
void clearCmd();
void fillCmd();
int splitCmd();
void prompt();
void printArgs(char* args[]);

char* outputFile;
char* inputFile;

static char* cwd;  //for cd
static char input='\0';
int stuff = 0; 
char buffer[MAX_PARAMS]; //array for storing input from cmd line
char* myArgv[100]; //array of char pointers, for storing arguments
char* myArgv2[100];
int myArgc = 0; //to keep track of number of arguments
int myArgc2 = 0;

/**
* Grab user input and store into a buffer. Gets each char and puts it into a buffer while not newline
* 
* returns the character read as an unsigned char cast to an int or EOF on endoffile error
*/

void parseCmd() {
   clearCmd();
	while((input != '\n')){

		if(input == EOF) { //check for CTRL+D
			exit(1);
		}

		buffer[stuff++] = input; 
		input = getchar(); 

	}
	buffer[stuff] = 0x00; //set to 0
}


/**
* Take the buffer,tokenize, and store to char array
*/
void fillCmd() {
	/*STRTOK - each cal to strtok returns null-terminated string containing the next token. 
does NOT include delimiters */

	char* bPtr; //token
	bPtr = strtok(buffer, DELIMITERS); 
	while(bPtr) { 
		myArgv[myArgc] = bPtr; //each bPtr equals a string we want, so store to array
		bPtr = strtok(NULL, DELIMITERS); //tokenize again. Start of next token foud by scanning forward to next delimiter byte
		myArgc++; //increment index
	}
}

/**
* Look for redirects and pipe, set descriptor accordingly,
* and store any needed files for duping. Called in execute()
*/
int splitCmd() {
	int descriptor = -1;
	
	//make two arrays, split command into two arg arrays

	for (int i = 0; i < myArgc; ++i) {

		if(strcmp(">",myArgv[i]) == 0) {
			outputFile = myArgv[i+1];
			myArgv[i+1] = NULL;
			descriptor = 1;
			//printf("Output file is: %s\n", outputFile);
		//shift elements in array so we don't store ">"
			for (int j = i; j < myArgc; ++j) {
				 myArgv[j] = myArgv[j+1];
				 myArgc--;
			}
		}

		else if (strcmp("<",myArgv[i]) == 0) {
			inputFile = myArgv[i+1];
			myArgv[i+1] = NULL;
			descriptor = 0;
			//printf("Input file is: %s\n", inputFile);
		//shift elements in array so we don't store "<"
			for (int j = i; j < myArgc; ++j) {
				  myArgv[j] = myArgv[j+1];
				  myArgc--;
			}    
		}


		else if(strcmp("|",myArgv[i]) == 0) {
			descriptor = 2;
		  //shift elements in array so we don't store "|"
			//myArgv[i+1] = NULL;

			for (int j = i; j < i + 1; ++j) {
				 myArgv[j] = myArgv[j+1];
				 myArgc--;
			}

			//TODO store contents to two arrays 

			// for (int j = 0; j < myArgc; j++) {
			// 	myArgv2[j] = myArgv[j];
			// 	//myArgc--;
			// }
		}
	}
		// puts("Argv array 1:");
		// printArgs(myArgv);
		// puts("Argv array 2");
		// printArgs(myArgv2);

	return descriptor; 
}



/**
* Clear command argument array and user input buffer
*/
void clearCmd() {
	//clear buffer
		for (int i = 0; i < sizeof(buffer); ++i) {
			buffer[i] = '\0';
	    }

        while (myArgc != 0) {
             myArgv[myArgc] = NULL; // delete element at argc
             myArgv2[myArgc2] = NULL;
             myArgc--;       // decrement the number of words in the command line
        }
        stuff = 0;       // erase the number of chars in the buffer
}

/**
* Pipe, fork and execute. This should be split up into separate functions
* I just didn't have time to make it more prettier
*/

void pipeCmd(char* cmd[]) {
	puts("Got to pipe cmd");
	int pfd[2];
	int ccpid = fork(); //make a second child to handle the command after pipe
		if(ccpid == -1) {
			perror("Second pipe error");
		}
		if(ccpid > 0) { //grandchild
			puts("got to gChild");
			if(close(pfd[0]) == -1) {
				perror("Close error in grandchild");
				exit(1);
			}
			dup2(pfd[1], STDOUT_FILENO); //redirect output to fd[1] (wr)
			if(execlp(cmd[1], cmd[1], NULL) == -1){
				perror("execlp grandchild");
				exit(1);
			} //child will execute command 
			puts("successful execute in gChild");
		}
		else { //parent
			puts("parent of Gchild here");
			wait(&ccpid); //waits for child to send output to pipe
			if(close(pfd[0]) == -1){
				perror("close fd[0] in pipeCmd");
				exit(1);
			}
			dup2(pfd[0],STDIN_FILENO);
			if (execvp(*cmd, cmd) == -1) {
				perror("execvp in pipeCmd");
				exit(1);
			}
			else {
				puts("execvp execution in pipeCmd complete");
			}
		}
}

void execute(char* cmd[]) {
	pid_t pid;
	int status;
	int bytesRead;
	char readBuffer[BUF_SIZE];
	int fd0, fd1;

    int descriptor = splitCmd();

	//if the first argument is string "exit", quit
    if (strcmp("exit", myArgv[0]) == 0) { //if strcmp returns 0 they're ==
          exit(EXIT_SUCCESS);
    }

    //this works, but for some reason gives me execvp error
    if (strcmp("cd", myArgv[0]) == 0) { 
          if(chdir(myArgv[1]) == -1) {
          	printf("The directory '%s' does not exist\n", myArgv[1]);
          	exit(1);
          }
    }

//--create pipe
	int fd[2]; 
	if(pipe(fd) == -1) {
		perror("Pipe error");
		exit(1);
	}

//--fork child process
	if((pid = fork()) == -1) {
		perror("Forking fail");
		exit(1);
	}

  //++CHILD++//
	if (pid == 0) { 
            if(close(fd[0]) == -1) { //close read end of pipe
				perror("Error closing stdin (fd[0])");
				exit(1);
			}

	//--Now child has stdin from the input file, stdout to output file
			if (descriptor == 1) { //OUT/WRITE command
				if(close(0) == -1) {
					perror("Close stdin error"); //close normal stdin 
					exit(1);
				}
				int fd1 = open(outputFile, O_CREAT | O_WRONLY, 0600); 
				if(fd1 == -1) {
					perror("Error opening output file");
					exit(1);
				}

				close(1); //close normal stdout
			    fd[1] = fd1;
      			if (dup2(fd[1], STDOUT_FILENO) == -1) { //redirect stdout to fd[1] (outputFile)
      				perror("Error duping stdout");
      				exit(1);
      			} 
        		if (close(fd[1]) == -1) {
        			perror("Error closing fd[1]");
        			exit(1);
        		}
			} 

			if (descriptor == 0) { //IN/READ command
				// if( close(1) == -1) { //THIS WAS PROBLEM - caused in redirection to fail 
				// 	perror("Close stdout error");
				// 	exit(1);
				// }
				int fd0 = open(inputFile, O_RDONLY); //open the input file in read only mode
				if(fd0 == -1) {
					perror("Error opening input file");
					exit(1);
				}
				close(0); //close normal stdin
				fd[0] = fd0;
      			if(dup2(fd[0], STDIN_FILENO) == -1) { //redirect stdin to fd[0] (inputFile)
      				perror("Error duping stdin");
      				exit(1);
      			}
        		if (close(fd[0]) == -1) {
        			perror("Error closing fd[0]");
        			exit(1);
        		}
			}

			if (descriptor == 2) { //PIPE 
				pipeCmd(myArgv);
			}

		//~~execute commands	
			if(execvp(*cmd, cmd) == -1) {
				perror("execvp error");
				exit(1);
			}	
	}

  //++PARENT++//
	else {
			if (close(fd[1]) == -1) {  //close the out end of the pipe/stdout
				perror("Couldn't close");
				exit(1);
			}

			//read from fd[0] (child), and write to stdout
			while(bytesRead = read(fd[0], readBuffer, 1) != 0) {
				if (bytesRead == -1) {
					perror("read error");
					exit(1);
				}
				if (write(1,readBuffer,1) == -1) {
					perror("write eRRor");
					exit(1);
				}
			}
			//wait for child to die and print exit status
			waitpid(pid, &status, WNOHANG);	
			if(WIFEXITED(status)){
			printf("Exited with status: %d\n", WEXITSTATUS(status));
		}

	}
	close(*fd);

}

/**
 * Prints a prompt 
 */
void prompt() {
	printf("%s  ~%s $ ", getenv("LOGNAME"), getcwd(cwd, 1024));
}

/**
 * Prints a welcome screen screen
 */
void welcome() {
	printf("\n---------------------------------------------------------------\n");
	puts(" *Welcome to my shell. Enter a command, or type exit/CTRL+D* ");
	printf("---------------------------------------------------------------\n");
    printf("\n\n");

}

/**
 *  Function that prints the contents of the array (for testing purposes)
 */
void printArgs(char* args[]) {
	for(int i =0; i < 10; i++) {
		//if(args[i] != NULL) {
		printf("[%d]: %s\n",i,args[i]);
	    //}
	}
}

int main(int argc, char* argv[]) {
	welcome();
	prompt();

 while(1) {
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
 }
 return 0;
}
