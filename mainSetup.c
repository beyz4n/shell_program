#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
//#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

struct Bookmark{
    char *name;
    struct Bookmark* next;    
};


struct Bookmark* createBookmark(const char* name) {
    struct Bookmark* newBookmark = (struct Bookmark*)malloc(sizeof(struct Bookmark));
    newBookmark->name = strdup(name);
    newBookmark->next = NULL;
    return newBookmark;
}

void insertBookmark(struct Bookmark** head, const char* name) {
    struct Bookmark* newBookmark = createBookmark(name);

    if (*head == NULL) {
        *head = newBookmark;
        return;
    }

    struct Bookmark* current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = newBookmark;
}

void deleteBookmark(struct Bookmark** head, const char* name) {
    struct Bookmark* current = *head;
    struct Bookmark* prev = NULL;

    while (current != NULL && strcmp(current->name, name) != 0) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Bookmark '%s' not found.\n", name);
        return;
    }

    if (prev != NULL) {
        prev->next = current->next;
    } else {
        *head = current->next;
    }

    free(current->name);
    free(current);

    printf("Bookmark '%s' deleted.\n", name);
}

void listBookmarks(struct Bookmark* head) {
    if (head == NULL) {
        printf("No bookmarks found.\n");
        return;
    }

    printf("Bookmarks:\n");
    while (head != NULL) {
        printf("%s\n", head->name);
        head = head->next;
    }
}

void freeBookmarks(struct Bookmark** head) {
    struct Bookmark* current = *head;
    while (current != NULL) {
        struct Bookmark* temp = current;
        current = current->next;
        free(temp->name);
        free(temp);
    }
    *head = NULL;
}


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

                        // ********bookmark*********


                        if(!strcmp(args[0], "bookmark")){
                            if(args[1] == "i"){

                            }else if (args[1] == "d")
                            {
                                
                            }else if(args[1] == "l"){

                            }else{

                            }
                            

                        }




                        int number_of_paths = 0;
                        char splitter = ':';
                        char** paths = split_paths(PATH, &splitter, &number_of_paths);

                        int fd;
                        int i = 0;
                        int argument_length = 0;
                        int isRedirection = 0;  
                        while(args[i] != NULL){

                            printf("args: %s\n", args[i]);
                            if(!strcmp(args[i], ">")){
                                printf("args2: %s", args[2]);
                                fd = open(args[2],CREATE_FLAGS, 0777);
                                argument_length = i;
                                isRedirection = 1;
                                break;
                            }
                            else if(!strcmp(args[i], ">>")){
                                fd = open(args[2],CREATE_FLAGS, 0777);
                                argument_length = i;
                                isRedirection = 1;
                                break;
                            }
                            else if(!strcmp(args[i], "<")){
                                fd = open(args[2], CREATE_FLAGS, 0777);
                                argument_length = i;
                                isRedirection = 1;
                                break;
                            }
                            else if(!strcmp(args[i], "2>")){
                                fd = open(args[2], CREATE_FLAGS, 0777);
                                argument_length = i;
                                isRedirection = 1;
                                break;
                            }
                            i++;
                        }

                        printf("isRed: %d\n", isRedirection);

                       if(isRedirection){
                            printf("aaa\n");

                            const char *arg_copy[argument_length + 1];
                            memcpy(arg_copy, args, (argument_length + 1) * sizeof(char*));
                            
                            printf("aaa2\n");

                            if (fd == -1) {
                                perror("Failed to open my.file");
                                return 1;
                            }
                            printf("aaa3\n");

                            if (dup2(fd, STDOUT_FILENO) == -1) {
                                perror("Failed to redirect standard output");
                                return 1;
                            }else{
                                printf("redirection operation");
                            }
                            printf("aaa4\n");
                            
                     
                        createProcess(paths, number_of_paths,arg_copy , &background);
                       if (close(fd) == -1) {
                            perror("Failed to close the file");
                            return 1;
                        }
                       }
                       else{
                        createProcess(paths, number_of_paths, args, &background);
                       }
                        printf("after creat");

                        
                            
                        

                        
                        /** the steps are:
                        (1) fork a child process using fork()
                        (2) the child process will invoke execv()
						(3) if background == 0, the parent will wait,
                        otherwise it will invoke the setup() function again. */
                        
                        

            }

                                    
}