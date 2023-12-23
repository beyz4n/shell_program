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


#define CREATE_FLAGS_TRUNC (O_WRONLY | O_CREAT | O_TRUNC)
#define CREATE_FLAGS_APPEND (O_WRONLY | O_CREAT | O_APPEND)
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_PATH_LEN 256
#define MAX_LINE_LEN 1024
pid_t pid;

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


void insertBookmark(struct Bookmark **head, const char *name) {
    struct Bookmark *newBookmark = createBookmark(name);

    if (*head == NULL) {
        *head = newBookmark;
        return;
    }

    struct Bookmark *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = newBookmark;
}

struct Bookmark *getBookmarkByIndex(struct Bookmark *head, int index) {
    int currentIndex = 0;
    while (head != NULL) {
        if (currentIndex == index) {
            return head;
        }
        head = head->next;
        currentIndex++;
    }
    return NULL; // Index out of bounds
}


void deleteBookmark(struct Bookmark **head, int index) {
    if (*head == NULL) {
        printf("No bookmarks found.\n");
        return;
    }

    struct Bookmark *current = *head;
    struct Bookmark *prev = NULL;
    int currentIndex = 0;

    while (current != NULL && currentIndex < index) {
        prev = current;
        current = current->next;
        currentIndex++;
    }

    if (current == NULL) {
        printf("Index %d is out of bounds.\n", index);
        return;
    }

    if (prev != NULL) {
        prev->next = current->next;
    } else {
        *head = current->next;
    }

    free(current->name);
    free(current);

    printf("Bookmark at index %d deleted.\n", index);
}

void listBookmarks(struct Bookmark* head) {
    int index = 0;
    if (head == NULL) {
        printf("No bookmarks found.\n");
        return;
    }

    printf("Bookmarks:\n");
    while (head != NULL) {
        printf("%d %s\n", index, head->name);
        head = head->next;
        index++;
    }
}

void argsCheck(int argsLength, char** args){
    for (int i = 0; i < argsLength; i++){
        if(!strcmp(args[i], "&")){
            args[i] = NULL;
            if(argsLength > i + 1)
                printf("Please don't put arguments after &");   
        }
    }
}

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
            case '\0':
            if(isBookmark){
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

    argsCheck(ct, args);

    if(!isBookmark){
        for (i = 0; i <= ct; i++)
		    printf("args %d = %s\n",i,args[i]);
    }

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



int createProcess(char **PATH, int number_of_paths ,char **args, int background){
        int execv_return_val = 0;
        pid = fork();

        if (pid == -1) {
            perror("Failed to fork\n");
        }
        if (pid == 0){ // child process
            char *temp_path = (char *)malloc(strlen("/") + strlen(args[0]) + 1);
            printf("Child %d %d\n", background,getpid());
            strcpy(temp_path, "/");
            strcat(temp_path, args[0]);

            for(int i = 0; i < number_of_paths; i++){
                strcat(PATH[i], temp_path);
                args[0] = PATH[i];
                execv_return_val = execv(PATH[i], args);
            }
        } 
        else { // parent process
            if(!background){
                wait(NULL); // wait child process
            }
        }
    return execv_return_val;
}

int redirection(char **paths, int number_of_paths ,char **args, int background){
    int fd;
    int i = 0;
    while(args[i] != NULL){
        if(!strcmp(args[i], ">")){
            int initSTDOUT = dup(1);
            fd = open(args[i+1],CREATE_FLAGS_TRUNC, 0777);
            if (fd == -1) {
                perror("Failed to open file");
                return 1;
            }
            dup2(fd,STDOUT_FILENO);
            if (close(fd) == -1) {
                perror("Failed to close the file");
                return 1;
            }
            args[i] = NULL;
            createProcess(paths, number_of_paths, args , background);
            if (dup2(initSTDOUT, 1) == -1) {
                perror("dup2");
            }
            close(initSTDOUT);
            return 2;
        }
        else if(!strcmp(args[i], ">>")){
            int initSTDOUT = dup(1);
            fd = open(args[i+1],CREATE_FLAGS_APPEND, 0777);
            if (fd == -1) {
                perror("Failed to open file");
                return 1;
            }
            dup2(fd,STDOUT_FILENO);
            if (close(fd) == -1) {
                perror("Failed to close the file");
                return 1;
            }
            args[i] = NULL;
            createProcess(paths, number_of_paths, args , background);
            if (dup2(initSTDOUT, 1) == -1) {
                perror("dup2");
            }
            close(initSTDOUT);
            return 2;
        }
        else if(!strcmp(args[i], "<")){
            int initSTDIN = dup(0);
            fd = open(args[i+1], O_RDONLY, 0777);
            if (fd == -1) {
                perror("Failed to open file");
                return 1;
            }
            dup2(fd, STDIN_FILENO);
            if (close(fd) == -1) {
                perror("Failed to close the file");
                return 1;
            }
            args[i] = NULL;
            createProcess(paths, number_of_paths, args , background);
            if (dup2(initSTDIN, 1) == -1) {
                perror("dup2");
            }
            close(initSTDIN);
            return 2;
        }
        else if(!strcmp(args[i], "2>")){
            int initSTDERR = dup(2);
            fd = open(args[i+1],CREATE_FLAGS_APPEND, 0777);
            if (fd == -1) {
                perror("Failed to open file");
                return 1;
            }
            dup2(fd,STDOUT_FILENO);
            if (close(fd) == -1) {
                perror("Failed to close the file");
                return 1;
            }
            args[i] = NULL;
            createProcess(paths, number_of_paths, args , background);
            if (dup2(initSTDERR, 1) == -1) {
                perror("dup2");
            }
            close(initSTDERR);
            return 2;
        }

        i++;
    }
}

char* deleteQuotationMark(char *str) {
    // Control bookmark string
    size_t len = strlen(str);
    
    if (len <= 2) {
        // TODO: eror bastıralım
        return;
    }

    // Bellekte yeni bir string oluştur
    char *newStr = (char *)malloc(len - 2 + 1); 
    if (newStr == NULL) {
        // Bellek tahsisi başarısız oldu
        fprintf(stderr, "Memory allocation failed.\n");
    }

    // İlk karakteri kopyala (atla)
    strncpy(newStr, str + 1, len - 2);

    // Son karakteri ekle
    newStr[len - 2] = '\0';

    // Orijinal string'in belleğini serbest bırak
   if (str != NULL) {
     free(str); 
    }

    return newStr;
}

int searchInFile(const char *filePath, const char *keyword) {
    int isFound = 0;
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filePath);
        return;
    }

    char line[MAX_LINE_LEN];
    int lineNumber = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        lineNumber++;

        // Search for the keyword in the line
        if (strstr(line, keyword) != NULL) {
            printf("%s:%d: %s", filePath, lineNumber, line);
            if(isFound == 0){
                isFound = 1;
            }
        }
    }

    fclose(file);
    return isFound;
}

void searchInDirectory(const char *dirPath, const char *keyword, int recursive) {
    DIR *dir = opendir(dirPath);
    int isFound = 0;
    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", dirPath);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Regular file
            char filePath[MAX_PATH_LEN];
            snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, entry->d_name);

            // Check file extension
            const char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL && (strcmp(ext, ".c") == 0 || strcmp(ext, ".C") == 0 || strcmp(ext, ".h") == 0 || strcmp(ext, ".H") == 0)) {
                if(searchInFile(filePath, keyword)){
                    isFound = 1;
                }

            }
        } else if (recursive && entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Recursive search in subdirectories
            char subDirPath[MAX_PATH_LEN];
            snprintf(subDirPath, sizeof(subDirPath), "%s/%s", dirPath, entry->d_name);
            searchInDirectory(subDirPath, keyword, recursive);
        }
        
    }

    closedir(dir);
    if(isFound == 0){
        printf("No result found.\n");
    }
}


int main(void){
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    struct Bookmark* bookmarks = NULL;

    while (1) {  
        printf("myshell: ");
        char *PATH = getenv("PATH");
        strcat(PATH, ":.");
        background = 0;
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background, 0);

        // find the paths
        int number_of_paths = 0;
        char splitter = ':';
        char** paths = split_paths(PATH, &splitter, &number_of_paths);

        // ********bookmark*********
        if(!strcmp(args[0], "bookmark")){
            if(!strcmp(args[1], "-i")){
                int value = atoi(args[2]);
                if(!value){
                    if(strcmp(args[2], "0")){
                        printf("You must enter a valid number.");
                        continue;
                    }
                }
                struct Bookmark *neededBookmark = getBookmarkByIndex(bookmarks, value);
                // TODO: execution will be added
                char* command = deleteQuotationMark(neededBookmark->name);
                setup(command, args, &background, 1);
                printf("args after setup %s\n", args[0]);
                redirection(paths, number_of_paths, args, background);
                createProcess(paths, number_of_paths, args, background);
                // TODO: setupa girmiyor debug at

                continue;
            }
            else if (!strcmp(args[1], "-d")){
                // delete
                int value = atoi(args[2]);
                if(!value){
                    if(strcmp(args[2], "0")){
                        printf("You must enter a valid number.");
                        continue;
                    }
                }
                deleteBookmark(&bookmarks, value);
                continue;
            }
            else if(!strcmp(args[1], "-l")){
                // list
                listBookmarks(bookmarks);
                continue;
            }
            else if(*args[1] == '"')
            {
                char *command = args[1];
                int count = 2;
                while(args[count] != NULL){
                    char* result = (char*)malloc(strlen(args[count]) + strlen(command) + 2);
                    strcpy(result, command);
                    strcat(result, " ");
                    strcat(result, args[count]);

                    command = result;
                    count++;
                }
                insertBookmark(&bookmarks, command);
                continue;
            }
        }

        // *** SEARCH ***
        if(!strcmp(args[0], "search")){
            char currentDir[MAX_PATH_LEN];
            if (getcwd(currentDir, sizeof(currentDir)) == NULL) {
                perror("Error getting current directory");
                return EXIT_FAILURE;
            }
            int recursive = 0;
            if(!strcmp(args[1], "-r")){
                recursive = 1;
                args[1] = args[2];
            }

            int keywordLen = strlen(args[1]);
            char *keyword = (char*)malloc((keywordLen) * sizeof(char));
            strcpy(keyword, args[1]);

            char *keywordNoQuots = deleteQuotationMark(keyword);

            searchInDirectory(currentDir, keywordNoQuots, recursive);
            continue;
        }

        // *****EXIT*****
       if(!strcmp(args[0], "exit")){
            pid_t childpid;
            int status;
            char* input;
            childpid = waitpid(-1, &status, WNOHANG);
            if(childpid == 0){
                printf("There are processes still running at background.\nDou you want to terminate them? ");
                printf("(y for 'yes', n for 'no' )\n");

                scanf("%s", input);
            }
            else if(childpid == -1 || childpid > 0){
                printf("There are no processes running. System exits..\n");
                exit(0);
            }
            if(!strcmp(input, "y")){
                signal(SIGQUIT, SIG_IGN);
                if(kill(0, SIGQUIT) == 0){
                    printf("All background processes are done. System exits..\n");
                    exit(0);
                }
            }
            if(!strcmp(input, "n")){
                printf("Shell didn't exit\n");
                continue;
            }
       }

        

        if(redirection(paths, number_of_paths, args, background)==2)
            continue;
        createProcess(paths, number_of_paths, args, background);
    }               
}