#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

//read, parse, execute 
//1-Read line from user, store to buffer
//2-tokenize line, put commands into array 
//next- fork where needed, execute, pipe, etc.....

#define BUF_SIZE 1024
#define MAX_PARAMATERS 100

//PROTOTYPES
void parseCmd();
void clearCmd();

char input='\0';
int bufferChars = 0;
char buffer[BUF_SIZE]; //array for storing input from cmd line

char* myArgv[10]; //array of char pointers, for storing arguments
int myArgc = 0; //to keep track of number of arguments

void parseCmd() {
	clearCmd();
 //gets each char and puts it into a buffer while not newline
	while((input != '\n')){
		buffer[bufferChars++] = input; 
		input = getchar(); 

	}

// //***PRINT BUFFER***//
// 		for (int i = 0; i < sizeof(buffer); ++i) {
// 		printf("%c",buffer[i]);
// 	}
// 	puts("");
}

void clearCmd() {

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

void fillCmd() {
	char* bPtr; //token
	bPtr = strtok(buffer, " "); 

	while(bPtr !=NULL) {
		myArgv[myArgc] = bPtr; //put words into the array
		bPtr = strtok(NULL, " ");
		myArgc++; //go to next array element
	}

	// for (int i = 0; i < sizeof(myArgv); ++i) {
	// 	printf("%s",myArgv[i]);
	// }
}


int main(int argc, char* argv[]) {

while(1) {

	clearCmd();
	parseCmd();
	fillCmd();

	int cpid;
	int fd[2];
	pid_t pid;
	int bytesRead;

//pipe this shit
	if (pipe(fd) == -1) {
		perror("Pipe error");
		exit(1);
	}

//fork it 
	if((pid = fork()) == -1) {
		perror("Fork fail. You suck");
		exit(1);
	}
//----------crotch fruit (child) process----------//
if (pid == 0) { 
	if(close(fd[0] == -1)) {
		perror("fd close error womp");
		exit(1);
	}

//redirect stdout
	if (dup2(fd[1], STDOUT_FILENO) == -1) {
		perror("dup2 error");
		exit(1);
	}

//Run the commands
	if (execvp(myArgv[1], myArgv +1) == -1) {
			perror("Execve error");
			exit(1);
		}
//----------parental unit----------//
} else { 
	if (close(fd[1]) == -1) {
		perror("Close error");
		exit(1);
	}

	while (bytesRead = read(fd[0], &buffer[0], 1) != 0){
			if (bytesRead == -1) {
				perror("");
				exit(1);
			}
			write(1,buffer,1);
}
	close(fd);
	waitpid(pid, 0, 0); //wait for child to die
}

// 	int cpid;
// 	int fd[2];
// 	pid_t pid;

// //pipe this shit
// 	if (pipe(fd) == -1) {
// 		perror("Pipe error");
// 		exit(1);
// 	}

// //fork it 
// 	if((pid = fork()) == -1) {
// 		perror("Fork error");
// 		exit(1);
// 	}

// if (pid == 0) { //child
// 	if(close(fd[0] == -1)) {
// 		perror("Close fd error");
// 		exit(1);
// 	}

// 	char* args[2];
// 	args[0] = argv[1];
// 	args[1] = argv[2];

// 	if (dup2(fd[1], STDOUT_FILENO) == -1) {
// 		perror("dup2 error");
// 		exit(1);
// 	}

// //execute command
// 	if (execvp(argv[1], argv +1) == -1) {
// 			perror("Execve error");
// 			exit(1);
// 		}

// } else { //parent
// 	if (close(fd[1]) == -1) {
// 		perror("Close error");
// 		exit(1);
// 	}
// }

// // close(fd); //where to close fd ?

 }
}
