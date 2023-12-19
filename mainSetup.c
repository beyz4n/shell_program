#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

	printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	} /* end of switch */
     }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */


char** split_paths(const char *str, const char *splitter, int *num_of_paths){
    char *copy_all_paths = strdup(str);

    // Find number of paths inside
    int count = 0;
    char *path_token = strtok(copy_all_paths, splitter);
    while (path_token != NULL) {
        count++;
        path_token = strtok(NULL, splitter);
    }

    // Memory allocation for path array
    char **paths = (char**)malloc(count * sizeof(char*));

    strcpy(copy_all_paths, str); 

    path_token = strtok(copy_all_paths, splitter);
    for (int i = 0; i < count; i++) {
        paths[i] = strdup(path_token);
        path_token = strtok(NULL, splitter);
    }

    *num_of_paths = count;

    free(copy_all_paths); // free the copy variable's allocated memory 

     return paths;

}



int createProcess(char **PATH, int number_of_paths ,char **args, int *background){
        int execv_return_val = 0;

        pid_t pid = fork();

        if (pid == -1) {
            perror("Failed to fork\n");
        }
        if (pid == 0){ // child process

            char *temp_path = (char *)malloc(strlen("/") + strlen(args[0]) + 1);

            strcpy(temp_path, "/");
            strcat(temp_path, args[0]);

            for(int i = 0; i < number_of_paths; i++){
                strcat(PATH[i], temp_path);
                args[0] = PATH[i];
                execv_return_val = execv(PATH[i], args);
                usleep(350);
            }
        } 
        else { // parent process
            if(background){
                return 0; // backgroundsa çıkar beklemez
            }else{
                wait(NULL); // background değilse childlarını bekliyor
            }
        }
    return execv_return_val;
}

 
int main(void)
{
            char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
            int background; /* equals 1 if a command is followed by '&' */
            char *args[MAX_LINE/2 + 1]; /*command line arguments */
            //char *PATH; // path sonradan doldurulacak
            while (1){  
                        char *PATH = getenv("PATH");
                        // printf("%s", PATH);
                        background = 0;
                        printf("myshell: ");
                        /*setup() calls exit() when Control-D is entered */
                        setup(inputBuffer, args, &background);
                        int number_of_paths = 0;
                        char splitter = ':';
                        char** paths = split_paths(PATH, &splitter, &number_of_paths);
            
                        createProcess(paths, number_of_paths, args, &background);


                        
                        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
						(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */

                        

            }

                                    
}