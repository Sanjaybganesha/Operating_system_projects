#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include<fcntl.h>
#include<signal.h>



//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100
#define MAX_BG_TASKS 100
#define WRITE 1
size_t MAX_LINE_LEN = 10000;


// commands
#define EXIT_STR "exit"
#define EXIT_CMD 1
#define FG_STR "fg"
#define FG_CMD 2
#define LISTJOBS_STR "listjobs"
#define LISTJOBS_CMD 3

// process states
#define RUNNING 0
#define TERMINATED 1
#define STOPPED 2
static const char *status_string[] = {"RUNNING", "TERMINATED", "STOPPED"};

// misc
#define ERROR -1
#define FREE -1
static int fd;
static fpos_t pos;
FILE *fp; // file struct for stdin
char **tokens, **redir, **redir_in, **pip2;
int token_count = 0,redir_count=0,redirin_count=0, pipe_count=0;
char *line;
int currentPid = 0;
struct background_task {
	pid_t pid;
	int status;
} bgtask[MAX_BG_TASKS];
int bg_count = 0;

void initialize()
{

	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual token pointers
	assert( (tokens = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);
	assert( (redir = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);
assert( (redir_in = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);
assert( (pip2 = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);
	// open stdin as a file pointer 
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

	// cleanup background task structs
	bg_count = 0;
	for(int i=0; i<MAX_BG_TASKS; i++) 
		bgtask[i].pid = FREE; 
}

int tokenize (char * string)
{
	int size = MAX_TOKENS;int i,out,in;
	char *this_token, *this_redir, *this_redir_in, *this_pipe;
	char *redirStr = NULL, *pipe1= NULL , *redirIn =NULL;char try;
	pid_t pid;	
	// cleanup old token pointers
	for(int i=0; i<token_count; i++) tokens[i] = NULL;
		redirStr = strchr(line, '>');
		pipe1= strchr(line, '|');
		redirIn= strchr(line, '<');
					if(redirStr)
				{ //redirStr++;
				//spid = fork();
				//if(pid==0)
				while ( (this_redir = strsep( &string, " \t\v\f\n\r")) != NULL) {
					redir[redir_count] = this_redir;
					redir_count++;
					}
				//printf("%s",redir[0]);
				//char *source = "prettyCoolString";
				//char *find = "CoolString";

					/*char dest[5],x[5];
				char *p = strstr(line, redirStr);
				strncpy(dest, line, p - line);int n=2;
					 printf("%.*s\n", n, dest);
					char r=dest[1];
					printf("%c",r);*/
						
					out = open(redir[2],O_WRONLY|O_CREAT,0666); 
					//dup2(out, STDOUT_FILENO);
					if(dup2(out,STDOUT_FILENO)==-1)
					{
					  perror(" ");
					}(execlp(redir[0],redir[0],(char*) NULL)==-1);
					close(out);
				//printf("yes working");
	
				} 
			if(redirIn)
	{ //redirIn++;
		while ( (this_redir_in = strsep( &string, " \t\v\f\n\r")) != NULL) {
					redir_in[redirin_count] = this_redir_in;
					redirin_count++;
					}	
				in =open(redir_in[2],O_RDONLY);			
					if(dup2(in,STDIN_FILENO)==-1)
					{
					  perror(" ");
					}
					(execlp(redir_in[0],redir_in[0],(char*) NULL)==-1);
					perror("exec faied:");
						exit(1);	
					close(in);
					return 0;
		 }
if(pipe1)
{  int a[2]; 
  pipe1++;
      pipe(a); 
  while ( (this_pipe = strsep( &string, " \t\v\f\n\r")) != NULL) {
					pip2[pipe_count] = this_pipe;
					pipe_count++;
					}
    if(!fork()) 
    { 
        // closing normal stdout 
        close(1); 
          
        // making stdout same as a[1] 
        dup(a[1]); 
          
        // closing reading part of pipe 
        // we don't need of it at this time 
        close(a[0]); 
          
        // executing ls  
        (execlp(pip2[0],pip2[0],(char*) NULL)==-1); 
    } 
    else
    { 
        // closing normal stdin 
        close(0); 
          
        // making stdin same as a[0] 
        dup(a[0]); 
          
        // closing writing part in parent, 
        // we don't need of it at this time 
        close(a[1]); 
          
        // executing wc 
        (execlp(pip2[2],pip2[2],(char*) NULL)==-1); 
    } 
} 
	
token_count = 0;
	while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL) {

		if (*this_token == '\0') continue;
		//if (*this_token == '>') continue;
		//if (*this_token == '<') continue;
		//if (*this_token == '|') continue;
		tokens[token_count] = this_token;

		//printf("Token %d: %s\n", token_count, tokens[token_count]);

		token_count++;
			
		// if there are more tokens than space ,reallocate more space
		if(token_count >= size){
			size*=2;

			assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
		}
	}
	//printf("token count = %d\n", token_count);
}

void read_command() 
{

	// getline will reallocate if input exceeds max length
	assert( getline(&line, &MAX_LINE_LEN, fp) > -1); 

	//printf("Shell read this line: %s\n", line);

	tokenize(line);
}

void handle_exit() 
{
	//printf("Exit command\n");
	
	// cleanup terminated tasks
	for(int i=0; i<MAX_BG_TASKS; i++) {
		if( bgtask[i].pid == FREE) continue;

		if (waitpid( bgtask[i].pid, NULL, WNOHANG) < 0)
			perror("error cleaning up background task:");
	}
}

void handle_listjobs() 
{

	int ret, status;

	for(int i=0; i<MAX_BG_TASKS; i++) {
		if( bgtask[i].pid == FREE) continue;

		//update status
		ret = waitpid( bgtask[i].pid, &status, WNOHANG);

		if (ret > 0) {

			if(WIFEXITED(status) || WIFSIGNALED(status)) 
				bgtask[i].status = TERMINATED;

			if(WIFSTOPPED(status)) 
				bgtask[i].status = STOPPED;
		}

		printf("PID: %d Status: %s\n", bgtask[i].pid, status_string[bgtask[i].status] );

		if(bgtask[i].status == TERMINATED) {
			bgtask[i].pid = FREE;
			bg_count--;
		}
	}
	
}

void handle_fg() 
{
	int i;
	pid_t  pid;
	int status;

	if(tokens[1] == NULL) {
		printf("Usage: fg <pid>\n");
		return;
	}

	pid = atoi(tokens[1]);

	for(i=0; i<MAX_BG_TASKS; i++) {
		if( bgtask[i].pid == pid ) break;
	}

	if(i == MAX_BG_TASKS) {
		printf("PID %d: No such background task\n", pid);
		return;
	}

	// free the entry
	bgtask[i].pid = FREE;

	//handle_listjobs();

	// block
	if( waitpid( pid, &status, 0) < 0) {
		perror("error waiting on forgrounded process:");
	}

}


int next_free_bg() 
{
	for(int i=0; i<MAX_BG_TASKS; i++) 
		if(bgtask[i].pid == FREE) return i;

	return -1;
}

int is_background() 
{
	return (strcmp(tokens[token_count-1], "&") == 0);
}
int run_command() 
{
	pid_t pid;
	int ret = 0, status;
	int out; // handle builtin commands first

	// exit
	if (strcmp( tokens[0], EXIT_STR ) == 0) {
		handle_exit();
		return EXIT_CMD;
	}

	// fg
	if (strcmp( tokens[0], FG_STR ) == 0) {
		handle_fg();
		return FG_CMD;
	}

	// listjobs
	if (strcmp( tokens[0], LISTJOBS_STR) == 0) {
		handle_listjobs();
		return FG_CMD;
	}

	// disallow more than MAX_BG_TASKS
	if (is_background() && (bg_count == MAX_BG_TASKS) ) {
		printf("No more background tasks allowed\n");
		return ERROR;
	}

	// fork child
	pid = fork();
	if( pid < 0) {
		perror("fork failed:");
		return ERROR;
	}

	if(pid==0) {
		execvp(tokens[0], tokens);
		perror("exec faied:");
		exit(1);
		if(is_background()) tokens[token_count-1] = NULL; 

		}
		else {
			if(is_background()) {
			// record bg task
			int next_free = next_free_bg();
			assert(next_free != -1); // we should have checked earlier
			bgtask[next_free].pid = pid;
			bgtask[next_free].status = RUNNING;
			bg_count++;
						} 
				else {
			// parent waits on foreground command
			currentPid = pid;
			ret = waitpid(pid, &status, 0);
					if( ret < 0) 	{
				perror("error waiting for child:");
				return ERROR;
							}
				     }
			}

	return 0;
}

void hand(int sig){
	if(currentPid!=0) kill(currentPid, sig);
}

int main()
{

	signal(SIGINT, hand);
	int ret;

	initialize();

	do {
		ret = 0;
		currentPid = 0;
		printf("sh550> ");

		read_command();

		if(token_count > 0) ret = run_command();

	} while( (ret != EXIT_CMD) && (ret != ERROR) );

	return 0;
}

