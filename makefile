#builds an executable named mySh from mysh.c 
all:mysh.c
	gcc -o mySh mysh.c 
clean:
	$(RM) mySh
