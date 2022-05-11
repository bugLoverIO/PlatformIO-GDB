#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define BUFF_SIZE   4096
#define MAX_PARAMS  20

const char* break_key       = "break-insert -f main\n";
const char* connect_key     = "^connected";
const char* continue_key    = "cont\n";

int aStdinPipe[2];
int aStdoutPipe[2];

typedef struct {
    char*   strGDB;
    char*   func;
    int     gdbStubInit;
    char**  params;
} tContext;

int runGDB(tContext* pCtx)
{
    int nResult = 0;
    
    // redirect stdin
    if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1) {
        exit(errno);
    }
    
    // redirect stdout
    if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
        exit(errno);
    }
    
    // redirect stderr
    if (dup2(aStdoutPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
        exit(errno);
    }
    
    // all these are for use by parent only
    close(aStdinPipe[PIPE_READ]);
    close(aStdinPipe[PIPE_WRITE]);
    close(aStdoutPipe[PIPE_READ]);
    close(aStdoutPipe[PIPE_WRITE]);
    
 
    pCtx->params[0] = pCtx->strGDB;
    
    // run GDB process
    nResult = execv(pCtx->strGDB, pCtx->params);
    
    // if we get here, an error occurred
    printf("GDB EXIT %d\n",nResult);
    exit(nResult);
}


int pipeGDB(int pid, tContext* pCtx)
{

    char*   buffer;
    struct  timeval timeout;
    fd_set  readFD,writeFD;
    
    // close unused file descriptors, these are for child only
    close(aStdinPipe[PIPE_READ]);
    close(aStdoutPipe[PIPE_WRITE]);
    
    buffer = malloc(BUFF_SIZE*sizeof(char));
    
    // Set timeout for select()
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    
    
    while (waitpid (pid, NULL, WNOHANG) != pid) {
        int nbRead = 0;
        
        // Set on wich file descriptor listen
        FD_ZERO(&readFD);
        FD_SET(aStdoutPipe[PIPE_READ],&readFD);
        
        if (select(aStdoutPipe[PIPE_READ]+1,&readFD, NULL, NULL,&timeout)>0) {
            nbRead = read(aStdoutPipe[PIPE_READ],buffer,BUFF_SIZE);
            if (nbRead > 0) {
                
                // once connected to the ESP8266, force GDB to continue allowing inserting next break-points IF gdbStubInit is true
                if ( (pCtx->gdbStubInit) && (strnstr(buffer,connect_key,nbRead)) ) {
                    write(aStdinPipe[PIPE_WRITE],continue_key,strlen(continue_key));
                }
                write(STDOUT_FILENO, buffer, nbRead);
            }
        }
        
        FD_ZERO(&writeFD);
        FD_SET(STDIN_FILENO,&writeFD);
        
        if (select(STDIN_FILENO+1,&writeFD, NULL, NULL,&timeout)>0) {
            nbRead = read(STDIN_FILENO,buffer,BUFF_SIZE);
            if (nbRead<0)
                continue;
            
            // if "break-insert -f main" is added, replaces by break-insert -f <function>
            if (strnstr(buffer,break_key,nbRead)) {
                char* pos = strnstr(buffer,"main",nbRead);
                sprintf(pos, "%s\n",pCtx->func);
            }
            
            if (NULL != buffer) {
                write(aStdinPipe[PIPE_WRITE], buffer, nbRead);
            }
        }
    }
    // done with these in this example program, you would normally keep these
    // open of course as long as you want to talk to the child
    close(aStdinPipe[PIPE_WRITE]);
    close(aStdoutPipe[PIPE_READ]);
    
    return 0;
}


int createGDB(tContext* pCtx) {
    int nChild = 0;
    
    if (pipe(aStdinPipe) < 0) {
        perror("allocating pipe for child input redirect");
        return -1;
    }
    if (pipe(aStdoutPipe) < 0) {
        close(aStdinPipe[PIPE_READ]);
        close(aStdinPipe[PIPE_WRITE]);
        perror("allocating pipe for child output redirect");
        return -1;
    }
    
    nChild = fork();
    if (0 == nChild)        return runGDB(pCtx);
    else if (nChild > 0)    return pipeGDB(nChild, pCtx);
    
    // failed to create child
    close(aStdinPipe[PIPE_READ]);
    close(aStdinPipe[PIPE_WRITE]);
    close(aStdoutPipe[PIPE_READ]);
    close(aStdoutPipe[PIPE_WRITE]);

    printf("ERROR creating GDB process \n");
    return (-1);
}


void usage() {
    printf ("usage: gdbpipe [--help] --gdb path [--func name] [--init true|false] [extra GDB parametes] \n");
    printf ("options\n");
    printf ("  -h, --help               : This message\n");
    printf ("  -g, --gdb  <path>        : GDB path\n");
    printf ("  -f, --func <string>      : Function name (in C code) to be used as 'main' subtitute (default is 'loop')\n");
    printf ("  -i, --init true|false    : Enable/disable 'continue' to cope with GDBSTUB_BREAK_ON_INIT pragma (default is true)\n");
    printf ("  [extra GDB parameters]   : Can be anything up to 19 parameters \n");
    printf("\n");
    abort ();
}


int parseParam (int argc, char **argv, tContext *myCtx)
{
    int     c;
    char    nbParams = 1;
    int     value;
    
    opterr = 0;
    while (1)
    {
        
        const struct option long_options[] =
        {
            {"help",   	    no_argument,        0, 'h'},
            {"gdb",         required_argument,  0, 'g'},
            {"init",        required_argument,  0, 'i'},
            {"func",        required_argument,  0, 'f'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long (argc, argv, "hg:f:i:", long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        switch (c)
        {
            case 'h':
                usage();
                break;
                                
            case 'g':
                myCtx->strGDB = optarg;
                break;

            case 'f':
                myCtx->func = optarg;
                break;

            case 'i':
                if ( (atoi(optarg) > 0) || (strcmp(optarg,"true") == 0) )  myCtx->gdbStubInit=1;
                else myCtx->gdbStubInit=0;
                break;

                
            case '?':
                myCtx->params[nbParams++] = argv[optind-1];
                if ((!argv[optind]) || (*(argv[optind]) == '-')) break;
                myCtx->params[nbParams++] = argv[optind];
                break;
                
            default:
                abort ();
        }
    }
    
    return 0;
}


int main (int argc, char **argv) {

    tContext ctx;

    ctx.strGDB      = NULL;
    ctx.gdbStubInit = 1;
    ctx.func        = "loop";
    ctx.params      = malloc (sizeof(char*) * MAX_PARAMS);
    memset(ctx.params, 0, sizeof(char*)*MAX_PARAMS);
    parseParam (argc, argv, &ctx);
    
    if (!ctx.strGDB) usage();
    
    createGDB(&ctx);
}
