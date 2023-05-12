#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<errno.h>
#include<fcntl.h>

#define MAXCOM 1000 // max number of letters to be supported
#define MAXLIST 100 // max number of commands to be supported
  
// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")


char cwd[1024];
int redirtype;
int prompttype;

// sigint handler
void int_handler(int status) {
    rl_clear_signals();
    rl_reset_line_state();      // Resets the display state to a clean state
    rl_cleanup_after_signal();  // Resets the terminal to the state before readline() was called
    rl_replace_line("",0);      // Clears the current prompt
    rl_crlf();                  // Moves the cursor to the next line
    rl_redisplay();             // Redisplays the prompt
}


// necessary env and history function calls
void init_shell()
{
    setenv("SHELL", "/bin/bosh", 1);
    read_history(".bosh_history");
}

void doExec(char **parsed)
{
    if(parsed[1] != NULL){
        execvp(parsed[1], parsed + 1);
	switch (errno) {
	    case EACCES: fprintf(stderr, "Error: permission denied\n");
	        break;
	    case ENOENT: fprintf(stderr, "Error: file does not exist\n");
                break;
	    default:     fprintf(stderr, "Error: unable to exec\n");
	}
    }
}
  
// Function to take input + display prompt
int takeInput(char* str)
{
    char* buf;
    getcwd(cwd, sizeof(cwd)); // construct working directory variable for readline prompt
    char* username = getenv("USER"); // construct username variable for readline prompt
    int promptSize = 2048; 
    char prompt[promptSize];
    
    if(strcmp(username, "root") == 0)
        snprintf(prompt, promptSize, "%s@:%s# ", username, cwd);
    else
        snprintf(prompt, promptSize, "%s@:%s$ ", username, cwd);
    if(prompttype)
    	buf = readline("");
    else
    	buf = readline(prompt);
    if (strlen(buf) != 0) {
        add_history(buf);
	write_history(".bosh_history");
        strcpy(str, buf);
        return 0;
    } else {
        return 1;
    }
}
  
// Function where the system command is executed
void execArgs(char** parsed)
{
    // Forking a child
    pid_t pid = fork(); 
  
    if (pid == -1) {
        printf("Failed forking child..\n");
        return;
    } else if (pid == 0) {
        if (execvp(parsed[0], parsed) < 0) {
            printf("Error: %s is not a valid command\n", parsed[0]);
        }
        exit(0);
    } else {
        // waiting for child to terminate
	signal(SIGINT, SIG_IGN);
        wait(NULL); 
	signal(SIGINT, int_handler);
        return;
    }
}

// Function that reads what type of redirection is being used (indicated by the redirtype variable) 
// and uses freopen() to open a file with the appropriate redirection operation
// When execvp executes the parsed command the stdout with be redirected to the file.
void execArgsRedir(char** parsed, char** parsedredir)
{
    pid_t p1;

    p1 = fork();
    if (p1 < 0){
        printf("Could not fork\n");
	return;
    }
    if(p1 == 0) {	
	
	// When redirtype == 0, the truncating redirection operator is used. (>)
	// When redirtype == 1, the appending redirection operator is used. (>>)

	if(redirtype == 0){
	    freopen(parsedredir[0], "w", stdout); 
            if (execvp(parsed[0], parsed) < 0) {
                printf("Could not execute command ..\n");
		freopen("/dev/tty", "w", stdout);
                exit(0);
            }
	    freopen("/dev/tty", "w", stdout);
	}
	else if(redirtype == 1){
	    freopen(parsedredir[0], "a", stdout);
	    if (execvp(parsed[0], parsed) < 0) {
                printf("Could not execute command ..\n");
		freopen("/dev/tty", "w", stdout);
		exit(0);
	    }
	    freopen("/dev/tty", "w", stdout);
	}
	
    } 
    else
        wait(NULL);
}

// Function where the piped system commands is executed
void execArgsPiped(char** parsed, char** parsedpipe)
{
    // 0 is read end, 1 is write end
    int pipefd[2]; 
    pid_t p1, p2;
  
    if (pipe(pipefd) < 0) {
        printf("Pipe could not be initialized\n");
        return;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("Could not fork\n");
        return;
    }
  
    if (p1 == 0) {
        // Child 1 executing..
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
  
        if (execvp(parsed[0], parsed) < 0) {
            printf("Could not execute command 1..\n");
            exit(0);
        }
    } else {
        // Parent executing
        p2 = fork();
  
        if (p2 < 0) {
            printf("Could not fork\n");
            return;
        }
  
        // Child 2 executing..
        // It only needs to read at the read end
        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                printf("Could not execute command 2..\n");
                exit(0);
            }
        } else {
            // parent executing, waiting for two children
            wait(NULL);
            wait(NULL);
        }
    }
}
  
// Help command builtin
void openHelp()
{
    puts("\n--- Welcome to BOSH, the Barebones Operating-system SHell ---"
         "\n A minimal and simple shell that is accepting contributions. "
         "\n"
	);
  
    return;
}

// Function to execute bosh builtin commands
int ownCmdHandler(char** parsed)
{
    int NoOfOwnCmds = 4, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;
  
    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";
    ListOfOwnCmds[2] = "help";
    ListOfOwnCmds[3] = "exec";

    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }
  
    switch (switchOwnArg) {
    case 1:
        exit(0);
    case 2:
        chdir(parsed[1]);
	getcwd(cwd, sizeof(cwd));
	setenv("PWD", cwd, 1);
        return 1;
    case 3:
        openHelp();
        return 1;
    case 4:
	doExec(parsed);
	return 1;
    default:
        break;
    }
  
    return 0;
}

// Function to find redirection operator.
// This function uses strstr() to locate the first instance of the redirection operator.
// Then the function inserts a null byte character '\0' into the string, effectively splitting the string in two

int parseRedirect(char* str, char** strredir)
{
    char* redirfinder = strstr(str, ">>");
    if(redirfinder != NULL){
	redirfinder[0] = '\0';
	redirfinder += 2; //iterates the pointer to the character by two to store the split string without '>>' into strredir[1]
	redirtype = 1;
	strredir[0] = str;
	strredir[1] = redirfinder;
        return 1;

    }
    else if(redirfinder == NULL){
	redirfinder = strstr(str, ">");
	if(redirfinder != NULL){
	    redirfinder[0] = '\0';
	    redirfinder += 1;
	    redirtype = 0;
	    strredir[0] = str;
	    strredir[1] = redirfinder;
	    return 1;
	}
    }
    return 0; // if no redirection operator is found, return zero
}

// This function finds the pipe
int parsePipe(char* str, char** strpiped)
{
    int i;
    for (i = 0; i < 2; i++) {
        strpiped[i] = strsep(&str, "|");
        if (strpiped[i] == NULL)
            break;
    }
  
    if (strpiped[1] == NULL)
        return 0; // returns zero if no pipe is found.
    else {
        return 1;
    }
}
  
// function for parsing command words
void parseSpace(char* str, char** parsed)
{
    int i;
    char* ptr;

    ptr = strchr(str, '#'); // This will delete any data after a comment symbol
    if (ptr != NULL) {
	*ptr = '\0';
    }    

    for (i = 0; i < MAXLIST; i++) {
        parsed[i] = strsep(&str, " ");
  
        if (parsed[i] == NULL)
            break;
        if (strlen(parsed[i]) == 0)
            i--;
    }

    // For loop for expanding env variables
    char* envvar;
    for(i = 0; i < MAXLIST; i++){
	if(parsed[i] == NULL)
	    break;
	if(parsed[i][0] == '$'){
	    parsed[i] = getenv(parsed[i]+1);
	}
    }

    
}
  
int processString(char* str, char** parsed, char** parsedpipe, char** parsedredir)
{
  
    char* strpiped[2];
    char* strredir[2];
    int piped = 0;
    int redirected = 0;
  
    piped = parsePipe(str, strpiped);
    redirected = parseRedirect(str, strredir);
  
    if (piped) {
        parseSpace(strpiped[0], parsed);
        parseSpace(strpiped[1], parsedpipe);
    } 
    else if(redirected) {
	parseSpace(strredir[0], parsed);
        parseSpace(strredir[1], parsedredir);
    } 
    else {
  
        parseSpace(str, parsed);
    }
  
    if (ownCmdHandler(parsed)) //Tests if the parsed command is a shell built in
        return 0;
    else {
	if(piped)
            return 2;
	else if(redirected)
	    return 3;
	else
	    return 1;
    }
}
  
int main(int argc, char** argv)
{

    char inputString[MAXCOM], *parsedArgs[MAXLIST];
    char* parsedArgsPiped[MAXLIST];
    char* parsedArgsRedir[MAXLIST];
    int execFlag = 0;
    FILE* fp;


    init_shell();

    // Spawns signal handler to prevent sigints
    signal(SIGINT, SIG_IGN);
    wait(NULL);
    if(signal(SIGINT, int_handler) == SIG_ERR){
	printf("failed to register interrupts with kernel\n");
    }

    if(argc > 1 && argv[1] != NULL){
    
	prompttype = 1;
        fp = fopen(argv[1], "r");
	if (fp == NULL)
            exit(EXIT_FAILURE);	
	
    } 

    while (1) {

	// take input + prompt	
	if(prompttype == 0)
            if (takeInput(inputString))
                continue;
	
	if(prompttype == 1){
	    if(fgets(inputString, MAXCOM, fp) == NULL){
	         exit(0);
	    }
	    inputString[strcspn(inputString, "\n")] = 0; // Remove newline from fgets()
	}
	    
        if(inputString[0] != '#')
	    execFlag = processString(inputString, parsedArgs, parsedArgsPiped, parsedArgsRedir);
	else
	    execFlag = 0;
        // execflag returns zero if there is no command
        // or it is a builtin command,
        // 1 if it is a simple command
        // 2 if it is including a pipe.
	// 3 if it is including a redirection operator
  
	if (execFlag == 1)
            execArgs(parsedArgs);
  
	else if (execFlag == 2)
	    execArgsPiped(parsedArgs, parsedArgsPiped);
	

	else if (execFlag == 3)
	    execArgsRedir(parsedArgs, parsedArgsRedir);
    }
    return 0;
}
