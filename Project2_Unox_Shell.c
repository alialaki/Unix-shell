//Unix Shell 2021

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
#define READ_END 0 
#define WRITE_END 1

/**
   @brief Split a line into tokens.
   @param line The line.
   @return redirect.
 */
char** split_line(char* line, char* chargs[]) {
    
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;
    
     if (!tokens) {
    fprintf(stderr, "sh: allocation error\n");
    exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
      chargs[0] = token;
  
    char** redirect = malloc(2 * sizeof(char*));
    for(int i =0; i < 2; ++i)
        redirect[i] = malloc(BUFSIZ * sizeof(char));
    //I/O/P
    redirect[0] = "";
    //path
    redirect[1] = "";

    while(token != NULL){
      //put token into char args vector
      token = strtok(NULL, LSH_TOK_DELIM);
      if(token == NULL)
          break;
    tokens_backup = tokens;
    if (!strncmp(token, ">", 1)){
          token = strtok(NULL, LSH_TOK_DELIM);
          redirect[0] = "o";
          redirect[1] = token;
        return redirect;
      }else
        //determine pipe
        if (!strncmp(token, "|", 1)){
            redirect[0] = "p";
          }
      else
        if (!strncmp(token, "<", 1)){
          token = strtok(NULL, LSH_TOK_DELIM);
          redirect[0] = "i";
          redirect[1] = token;
          return redirect;
      }
      if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
      chargs[++position]  = token;
    }    
    return redirect;
}

/**
 * @brief find pipe.
 * @param chargs 
 * @return int 
 */

int pipe_lsh(char** chargs){

    int position = 0;

    while(chargs[position] != '\0'){
        //try to find pipe
        if(!strncmp(chargs[position], "|", 1)){
            // start offset
            return position;
        }
        ++position;
    }
    return -1;
}

/**
   @brief Loop getting input.
 */
int shell_llp(void){
    //declare local varibles
    char line [BUFSIZ];
    char last_command [BUFSIZ];
    char* lefty[BUFSIZ];
    char *righty[BUFSIZ];
    int status; 
    //use for file descriptor 
    int pipeFileDescp[2];
    bool finished = false;

    //wipe out buffers
    memset(line, 0, BUFSIZ * sizeof(char));
    memset(last_command, 0, BUFSIZ * sizeof(char));

    while(!finished)
    {
        // command line 
        printf("osh>a ");
        // force flush to console  
        fflush(stdout);   
        // read into input from stdin
        fgets(line, BUFSIZ, stdin);
        //set newline with null
        line[strlen(line) - 1] = '\0'; 
        // superfacial cases
        if(strncmp(line, "exit", 4) == 0)// quit
            return 0;
        // history (last_command)
        if(strncmp(line, "!!", 2)) 
            strcpy(last_command, line);      

        pid_t pid = fork();
        // failed to create child process
        if(pid < 0){   
            fprintf(stderr, "unable to create fork ...\n");
            return -1; 
        }
         // parent process
        if(pid != 0){ 
            //Force parent to wait 
            wait(NULL);
            wait(NULL);
            
        }
        // child process
        else{          
            char* chargs[BUFSIZ];    
            memset(chargs, 0, BUFSIZ * sizeof(char));

            int history = 0;
            // read from cache (last command)
            if(!strncmp(line, "!!", 2)) history = 1;
            // check for correct buffer
            char** redirect = split_line( (history ? last_command : line), chargs);
            //check for previous command 
            if(history && last_command[0] == '\0'){
                printf("command not found.\n");
                exit(0);
            } 
            // redir input & output to file descriptor
            if(!strncmp(redirect[0], "o", 1)){
                printf("output saved to ./%s\n", redirect[1]);
                int fD = open(redirect[1], O_TRUNC | O_CREAT | O_RDWR);
                // redirect stdout to file descriptor 
                dup2(fD, STDOUT_FILENO);
                // input redir 
            }else if(!strncmp(redirect[0], "i", 1)){
                printf("reading from file: ./%s\n", redirect[1]);
                // read-only
                int fD = open(redirect[1], O_RDONLY);
                memset(line, 0, BUFSIZ * sizeof(char));
                read(fD, line,  BUFSIZ * sizeof(char));
                memset(chargs, 0, BUFSIZ * sizeof(char));
                line[strlen(line) - 1]  = '\0';
                split_line(line, chargs);
            // Look for a pipe
            }else if(!strncmp(redirect[0], "p", 1)){
                //child process
                pid_t pidchild;
                int p_r_offset = pipe_lsh(chargs);
                chargs[p_r_offset] = "\0";
                int pep = pipe(pipeFileDescp);
                if(pep < 0){
                    fprintf(stderr, "Unable to create pipe...\n");
                    return 1;
                }

                memset(lefty, 0, BUFSIZ*sizeof(char));
                memset(righty, 0, BUFSIZ*sizeof(char));

                for(int x = 0; x < p_r_offset; ++x){
                    lefty[x] = chargs[x];
                }
                for(int x = 0; x < BUFSIZ; ++x){
                    int position = x + p_r_offset + 1;
                    if(chargs[position] == 0)
                    break;
                    righty[x] = chargs[position];
                }
               
                // create child to handle pipe's rhs
                pidchild = fork();
                if(pidchild < 0){
                    fprintf(stderr, "fork not succesfull...\n");
                    return 1;
                }
                // parent process
                if(pidchild != 0){ 
                    // duplicate stdout to write end of file descriptor
                    dup2(pipeFileDescp[WRITE_END], STDOUT_FILENO);
                    close(pipeFileDescp[WRITE_END]);
                    execvp(lefty[0], lefty);
                    exit(0); 
                }else
                {
                    // child process
                    dup2(pipeFileDescp[READ_END], STDIN_FILENO);
                    close(pipeFileDescp[READ_END]);
                    execvp(righty[0], righty);
                    exit(0); 
                }
                wait(NULL);
            }
            execvp(chargs[0], chargs);
            exit(0);
        }
        finished = false;
    }
    return 0;
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument array.
 */

int main(int argc, const char *argv[]) {

    //run command loop
    shell_llp();

  return EXIT_SUCCESS;
}


/**************************************************************************************/
//
//
//                                  End of program
//
//
/**************************************************************************************/

