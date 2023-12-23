// CSE3033 Operating Systems - Homework 2
// 150120077 Beyza Nur Kaya
// 150120047 Sena Ektiricioğlu
// 150120061 Selin Aydın

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>

// Flags for file creation that does append or truncate accordingly.
#define CREATE_FLAGS_TRUNC (O_WRONLY | O_CREAT | O_TRUNC)
#define CREATE_FLAGS_APPEND (O_WRONLY | O_CREAT | O_APPEND)

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

// For paths, we had to limit the path and line lengths so that we can store them in an array.
#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 1024

// Global variables since we need them for handlers
pid_t pid;
int foreground;

int main(void);
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

// Handling the Control+Z option which stops the signal if it is foreground and calls the main.
int handleCtrlZ(int signo) {
    if (signo == SIGTSTP) {
        if(foreground == 1){
            printf("\nCtrl+Z pressed. Stopping the child process.\n");
            main();
        }
    }
}

// This is an signal handler that resets the signal from the Control Z
void setupSignalHandler() {
    // Action is declared sa and is the handleCtrlZ method
    struct sigaction sa;
    sa.sa_handler = handleCtrlZ;
    // 
    sigemptyset(&sa.sa_mask); // makes a empty signal set
    sa.sa_flags = SA_RESTART; // This is set to restart so that it can use the values for interrupts

    if (sigaction(SIGTSTP, &sa, NULL) == -1) { // if action is -1, then there is an error encountered.
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

// To store the bookmarks we implemented a linked list since the size is unknown.
struct Bookmark{
    char *name; // it stores a name as "command"
    struct Bookmark* next; // points to the next bookmark
};

// This method is to allocate memory and put the data to the struct
struct Bookmark* createBookmark(const char* name) {
    struct Bookmark* newBookmark = (struct Bookmark*)malloc(sizeof(struct Bookmark)); // mem allocation done for the size of struvt
    newBookmark->name = strdup(name); // duplicate the name to the bookmark's naem field
    newBookmark->next = NULL; // next will be empty so it will be null
    return newBookmark;
}

// This method inserts the bookmark
void insertBookmark(struct Bookmark **head, const char *name) {
    struct Bookmark *newBookmark = createBookmark(name); // we create the bookmark

    if (*head == NULL) {
        *head = newBookmark; // if empty, head will be new bookmark we had created
        return;
    }

    struct Bookmark *current = *head;
    while (current->next != NULL) { // if it is not null, it will find the last element
        current = current->next;
    }

    current->next = newBookmark; // the last element's next will be the new bookmark we had created.
}

// This method returns the bookmark by the given index
struct Bookmark *getBookmarkByIndex(struct Bookmark *head, int index) {
    int currentIndex = 0;
    while (head != NULL) { // checks the current struct is null or not 
        if (currentIndex == index) { // if we found it return the current struct
            return head;
        }
        head = head->next; // if not found move to the next one
        currentIndex++; // increment the index
    }
    return NULL; // if not found return null
}

// This method deletes the bookmark usşng the index
void deleteBookmark(struct Bookmark **head, int index) {
    // if list is empty, return nothing
    if (*head == NULL) {
        printf("No bookmarks found.\n");
        return;
    }

    struct Bookmark *current = *head;
    struct Bookmark *prev = NULL;
    int currentIndex = 0;

    // find the struct with the index given
    while (current != NULL && currentIndex < index) {
        prev = current;
        current = current->next;
        currentIndex++;
    }

    // if we cant find it then give error and return
    if (current == NULL) {
        printf("Index %d is out of bounds.\n", index);
        return;
    }

    // if there is a previous then match them
    if (prev != NULL) {
        prev->next = current->next;
    // otherwise, make the head 
    } else {
        *head = current->next;
    }

    // free them
    free(current->name);
    free(current);

    // Return that it is deleted.
    printf("Bookmark at index %d deleted.\n", index);
}

// This method lists the bookmarks
void listBookmarks(struct Bookmark* head) {

    int index = 0;
    // If head is null return
    if (head == NULL) {
        printf("No bookmarks found.\n");
        return;
    }

    printf("Bookmarks:\n");
    // until there is no element, traverse the list from head to tail
    while (head != NULL) {
        printf("%d %s\n", index, head->name);
        head = head->next;
        index++;
    }
}

// This method checks if there is a uppersound symbol in the command given and makes that element null
void argsCheck(int argsLength, char** args){
    for (int i = 0; i < argsLength; i++){
        if(!strcmp(args[i], "&")){
            args[i] = NULL;
            if(argsLength > i + 1) // also this check is for after the uppersound symbol there shouldnt be any arguments
                printf("Please don't put arguments after &");   
        }
    }
}

// Given setup function
void setup(char inputBuffer[], char *args[], int *background, int isBookmark)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
   

    /* read what the user enters on the command line */
    if(isBookmark){
        length = strlen(inputBuffer) + 1; // because of null character
    }
    else{
        length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  
    }   


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

	//printf(">>%s<<",inputBuffer);
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
            case '\0':
            if(isBookmark){ // if bookmark will be executed
                if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
            }
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
    // I only added a check for background
    argsCheck(ct, args);

    

} /* end of setup routine */


// THis function splits the paths from the getenv.
// getenv returns them as  "p1:p2:p3" so we have to split them.
char** split_paths(const char *str, const char *splitter, int *num_of_paths){
    char *copy_all_paths = strdup(str); // copy them
    // we find the length of path here
    int count = 0;
    char *path_token = strtok(copy_all_paths, splitter); // tokenize it using ":"
    while (path_token != NULL) { // until it is null
        count++;
        path_token = strtok(NULL, splitter); // tokenize them
    }
    // mem allocation for paths according to the size
    char **paths = (char**)malloc(count * sizeof(char*));
    // copy them
    strcpy(copy_all_paths, str); 
    path_token = strtok(copy_all_paths, splitter);  // tokenize them
    for (int i = 0; i < count; i++) {
        paths[i] = strdup(path_token); // duplicate the token
        path_token = strtok(NULL, splitter); // token them again
    }
    *num_of_paths = count; // the number of paths are also recored
    free(copy_all_paths); // free the copy variable
    return paths;
}



int createProcess(char **PATH, int number_of_paths ,char **args, int background){
        int execv_return_val = 0;
        pid = fork(); // fork

        if (pid == -1) {
            perror("Failed to fork\n");
        }
        if (pid == 0){ // child process
            char *temp_path = (char *)malloc(strlen("/") + strlen(args[0]) + 1); // give the path to the command
            strcpy(temp_path, "/");
            strcat(temp_path, args[0]);

            for(int i = 0; i < number_of_paths; i++){
                strcat(PATH[i], temp_path);
                args[0] = PATH[i];
                execv_return_val = execv(PATH[i], args); // execute it
            }
        } 
        else { // parent process
            if(!background){
                wait(NULL); // wait for the child process
            }
        }
    return execv_return_val; // return the execution value
}

int redirection(char **paths, int number_of_paths ,char **args, int background){
    int fd; // file descriptor
    int i = 0;
    while(args[i] != NULL){ // until we dont have any argument left
        if(!strcmp(args[i], ">")){ // if args is > 
            int initSTDOUT = dup(1); // save the initial state of the stdout
            fd = open(args[i+1],CREATE_FLAGS_TRUNC, 0777); // open th e file descriptor for truncate, w/ 777 permissions
            if (fd == -1) {
                perror("Failed to open file"); // if fails give error
                return 1;
            }
            dup2(fd,STDOUT_FILENO); // swap them
            if (close(fd) == -1) {
                perror("Failed to close the file"); // close the fd
                return 1;
            }
            args[i] = NULL; // make > null
            createProcess(paths, number_of_paths, args , background); // create the process
            if (dup2(initSTDOUT, 1) == -1) { // switch to original state
                perror("dup2");
            }
            close(initSTDOUT); // close the original state
            return 2; // return with 2
        }
        else if(!strcmp(args[i], ">>")){  // if args is >> 
            int initSTDOUT = dup(1); // save the initial state of the stdout
            fd = open(args[i+1],CREATE_FLAGS_APPEND, 0777); // open th e file descriptor for truncate, w/ 777 permissions
            if (fd == -1) {
                perror("Failed to open file"); // if fails give error
                return 1;
            }
            dup2(fd,STDOUT_FILENO); // swap them
            if (close(fd) == -1) {
                perror("Failed to close the file"); // close the fd
                return 1;
            }
            args[i] = NULL; // make >> null
            createProcess(paths, number_of_paths, args , background); // create the process
            if (dup2(initSTDOUT, 1) == -1) { // switch to original state
                perror("dup2");
            }
            close(initSTDOUT); // close the original state
            return 2; // return with 2
        }
        else if(!strcmp(args[i], "<")){ // if args is <
            int initSTDIN = dup(0); // save the initial state of the stdin
            fd = open(args[i+1], O_RDONLY, 0777); // open th e file descriptor for truncate, w/ 777 permissions
            if (fd == -1) {
                perror("Failed to open file"); // if fails give error
                return 1;
            }
            dup2(fd, STDIN_FILENO); // swap them
            if (close(fd) == -1) {
                perror("Failed to close the file"); // close the fd
                return 1;
            }
            args[i] = NULL; // make < null
            createProcess(paths, number_of_paths, args , background); // create the process
            if (dup2(initSTDIN, 1) == -1) { // switch to original state
                perror("dup2"); 
            }
            close(initSTDIN); // close the original state 
            return 2; // return with 2
        }
        else if(!strcmp(args[i], "2>")){ // if args is 2>
            int initSTDERR = dup(2); // save the initial state of the stderr 
            fd = open(args[i+1],CREATE_FLAGS_APPEND, 0777); // open th e file descriptor for truncate, w/ 777 permissions
            if (fd == -1) {
                perror("Failed to open file"); // if fails give error
                return 1;
            }
            dup2(fd,STDOUT_FILENO); // swap them
            if (close(fd) == -1) {
                perror("Failed to close the file"); // close the fd
                return 1;
            }
            args[i] = NULL;  // make 2> null
            createProcess(paths, number_of_paths, args , background); // create the process
            if (dup2(initSTDERR, 1) == -1) { // switch to original state
                perror("dup2");
            }
            close(initSTDERR); // close the original state
            return 2; // return with 2
        }

        i++;
    }
}


// This function deletes the quotation mark at beginnig and end
char* deleteQuotationMark(char *str) {
    size_t len = strlen(str); // gets the length
    char *newStr = (char *)malloc(len - 2 + 1);  // create a new string
    if (newStr == NULL) {
        fprintf(stderr, "Memory allocation failed.\n"); // if fails
    }
    strncpy(newStr, str + 1, len - 2); // copy from index 1 to index -1 
    newStr[len - 2] = '\0'; // make last index null
    if (str != NULL) {
        free(str); // free the redundant memory
    }
    return newStr;
}

// This method searchs in the file for search implementeation
int searchInFile(const char *filePath, const char *keyword) {

    // GEt the current directory using getcwd
    char currentDir[MAX_PATH_LEN];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL) {
        perror("Error getting current directory");
        return EXIT_FAILURE;
    }
    // mem allocation for path name
    char* newFilePath = (char*)malloc(strlen(filePath) - strlen(currentDir) + 1);
    // instead of the full path name, we used . for the current directory as in the output given from the pdf
    strcpy(newFilePath, "."); //
    int startIndex = strlen(currentDir); 
    strcpy(newFilePath+1, filePath + startIndex); // copy the other parts, now the new file path is ./xx instead of /home/desktop/...

    // we keed if it is found
    int isFound = 0;
    FILE *file = fopen(filePath, "r"); // open the file 
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filePath); // if it fails give error
        return -1;
    }

    char line[MAX_LINE_LEN]; // keep the line
    int lineNumber = 0; // keep the line number

    while (fgets(line, sizeof(line), file) != NULL) {
        lineNumber++; // increment the line number and get the new line

        // search the keyword in the line
        if (strstr(line, keyword) != NULL) {
            printf("\t%d: %s -> %s", lineNumber, newFilePath, line); // print if it exists
            if(isFound == 0){
                isFound = 1; // make found 1
            }
        }
    }
    fclose(file); // close the file
    return isFound; // return the found information
}

// Search in the directory
void searchInDirectory(const char *dirPath, const char *keyword, int recursive) {
    DIR *dir = opendir(dirPath); // open the directory
    int isFound = 0; // store if it is found
    if (dir == NULL) { 
        fprintf(stderr, "Error opening directory: %s\n", dirPath); // if fails give error
        return;
    }

    struct dirent *entry; // it will save the directory entries
    while ((entry = readdir(dir)) != NULL) { // until it is not null
        if (entry->d_type == DT_REG) { // if the type is regular file flag
            char filePath[MAX_PATH_LEN]; // get the file path
            snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name); // this makes the storing of file path using the struct dirent's name

            // we check gere that the ending extension is .c .C .h or .H
            const char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL && (strcmp(ext, ".c") == 0 || strcmp(ext, ".C") == 0 || strcmp(ext, ".h") == 0 || strcmp(ext, ".H") == 0)) {
                if(searchInFile(filePath, keyword)){ // if it is one of these then we call search function
                    isFound = 1; // make the found state 1
                }

            }
        }
        // if it is recursive
        else if (recursive && entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { // check the type if directory and dont have . or ..
            // then we again get the directory and call the function again to find the subdirectories
            char subDirPath[MAX_PATH_LEN];
            snprintf(subDirPath, sizeof(subDirPath), "%s/%s", dirPath, entry->d_name);
            searchInDirectory(subDirPath, keyword, recursive);
        }
        
    }
    // close the directory opened
    closedir(dir);
}

int main(void){
   char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    struct Bookmark* bookmarks = NULL; // bookmarks head

    while (1) {
        printf("myshell: "); // print the my shell
        fflush(stdout); // make sure that it is printed
        char *PATH = getenv("PATH");  // get paths
        strcat(PATH, ":."); // add the current directory
        background = 0; // background is 0 initally
        /*setup() calls exit() when Control-D is entered */ 
        setup(inputBuffer, args, &background, 0); // call setup function
        if(background == 0) // adjust foreground
            foreground = 1;
        if(background == 1)
            foreground = 0;


        // find the paths
        int number_of_paths = 0;
        char splitter = ':'; // split made by :
        char** paths = split_paths(PATH, &splitter, &number_of_paths); // call the method

        setupSignalHandler();

        // ********bookmark*********
        if(!strcmp(args[0], "bookmark")){ // if it is a bookmark
            if(!strcmp(args[1], "-i")){ // check if the second argument is -i
                int value = atoi(args[2]); // then make the value int
                if(!value){
                    if(strcmp(args[2], "0")){
                        printf("You must enter a valid number."); // check if int
                        continue;
                    }
                }
                struct Bookmark *neededBookmark = getBookmarkByIndex(bookmarks, value); // get the data
                // TODO: execution will be added
                char* command = deleteQuotationMark(neededBookmark->name); // delete quotataion marks
                setup(command, args, &background, 1); // call setup to token them aga in
                redirection(paths, number_of_paths, args, background); // call redirection if there is any
                createProcess(paths, number_of_paths, args, background); // call createprocess if there is not any redirection
                continue; // go back to the first state of while
            }
            else if (!strcmp(args[1], "-d")){  // check if the second argument is -i
                // delete
                int value = atoi(args[2]); // then make the value int
                if(!value){
                    if(strcmp(args[2], "0")){
                        printf("You must enter a valid number."); // check if int
                        continue;
                    }
                }
                deleteBookmark(&bookmarks, value); // delete the bookmark
                continue; // go back to the first state of the while
            }
            else if(!strcmp(args[1], "-l")){ // check if the second argument is -l
                // list the bookmarks
                listBookmarks(bookmarks);
                continue; // go back to the first state of while loop
            }
            else if(*args[1] == '"') // if we insert one 
            {
                char *command = args[1]; // get the command
                int count = 2; // store the resulting "command" 
                while(args[count] != NULL){
                    char* result = (char*)malloc(strlen(args[count]) + strlen(command) + 2);
                    strcpy(result, command);
                    strcat(result, " ");
                    strcat(result, args[count]);
                    command = result;
                    count++;
                }
                insertBookmark(&bookmarks, command); // insert the bookmark
                continue; // go back to the first state of while loop
            }
        }

        // *** SEARCH ***
        if(!strcmp(args[0], "search")){ // if the command is search
            char currentDir[MAX_PATH_LEN]; // get the current directory using getcwd method
            if (getcwd(currentDir, sizeof(currentDir)) == NULL) { 
                perror("Error getting current directory"); // if fails
                return EXIT_FAILURE;
            }
            int recursive = 0;
            if(!strcmp(args[1], "-r")){ // if recursive
                recursive = 1;
                args[1] = args[2];
            }

            int keywordLen = strlen(args[1]); // adjust the keyword and delete its quotation marks
            char *keyword = (char*)malloc((keywordLen) * sizeof(char));
            strcpy(keyword, args[1]);
            char *keywordNoQuots = deleteQuotationMark(keyword);
            searchInDirectory(currentDir, keywordNoQuots, recursive); // call the search function with the current directory
            continue; // go back to the first state of while loop 
        }

        // *****EXIT*****
       if(!strcmp(args[0], "exit")){ // if exit is the command
            pid_t childpid; // store the child pid
            int status; // store the status
            char answer; // keep the answer from the user
            childpid = waitpid(-1, &status, WNOHANG); // get the wait pid so that if there is any process are working or completed.
            // if childpid is 0, there are processes are currently working
            if(childpid == 0){
                printf("There are processes still running at background.\nDou you want to terminate them? "); // ask if they want to terminate 
                printf("(y for 'yes', n for 'no' )\n");
                scanf("%c", &answer); // store the answer
            }
            // if the pid is -1 or pid is bigger than 0, there were no background jobs or they are completed
            else if(childpid == -1 || childpid > 0){ 
                printf("There are no processes running. System exits..\n"); // it exits directly
                exit(0);
            }
            if(answer == 'y'){ // if exit is y
                signal(SIGQUIT, SIG_IGN); // signal to quit and ignore the main is called
                if(kill(0, SIGQUIT) == 0){
                    printf("All background processes are terminated. System exits..\n"); // print these and exit
                    exit(0);
                }
            }
            else if(answer == 'n'){ // if no notify and continue back to the first state of while loop
                printf("Shell didn't exit\n");
                continue;
            }
            else{ // invalid input
                printf("Please enter y or n");
            }
       }
        // if none of these happened it may be redirection we call it
        if(redirection(paths, number_of_paths, args, background)==2)
            continue; // if it was a redirection we returned 2 and go back to the first state of while loop
        createProcess(paths, number_of_paths, args, background); // otherwise we directly call the create process
    }  
}