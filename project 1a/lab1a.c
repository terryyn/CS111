/*Name: Terry Ye
  Email: yeyunong@hotmail.com
  ID:004757414
*/


/* reference :  https://stackoverflow.com/questions/12444679/how-does-the-poll-c-linux-function-work    for poll
                https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html                for noncanonical
*/    


#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>

int buffer_size = 2048;
struct termios saved_attributes;
int pipe_fd_tochild[2];
int pipe_fd_fromchild[2];
struct pollfd pollfds[2];
pid_t child_pid = -1;


void reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void sigpipehandle()
{
    printf("received sigpipe \n");
    if(close(pipe_fd_fromchild[0])==-1)
    {
        fprintf(stderr,"error when close: %s \n",strerror(errno));
        exit(1);
    }
    reset_input_mode();
    int status;
    if(waitpid(0,&status,0)==-1)
    {
        fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
        exit(1);
    }
    fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
    exit(0);
}





int main(int argc,char** argv)
{
    _Bool existshell = 0;
    static struct option long_options[]=
    {
      {"shell",no_argument,0,'s'},
      {0,0,0,0}
    };
    while(1)
    {
        int c = getopt_long(argc,argv,"s:",long_options,NULL);
        if(c== -1)
            break;
        if(c == 's')
            existshell = 1;
        else{
            fprintf(stderr,"unrecognized argument \n");
            exit(1);
        }
    }
    struct termios current_mode;
    tcgetattr(0,&current_mode);
    tcgetattr(0,&saved_attributes);
    current_mode.c_iflag = ISTRIP;
    current_mode.c_oflag = 0;
    current_mode.c_lflag = 0;
    tcsetattr(STDIN_FILENO,TCSANOW,&current_mode);
    if(!existshell){
        int tempread= 0 ;
        char buf[buffer_size];
        while(1)
        {
            tempread=read(0,buf,buffer_size);
            if (tempread< 0)
            {
                fprintf(stderr,"error when read:%s \n",strerror(errno));
                exit(1);
            }
            int i=0;
            for(;i<tempread;i++)
            {
                char temp = buf[i];
                if(temp ==0x0D||temp==0x0A){
                    char crlf[2]= {0x0D,0x0A};
                    if(write(1,crlf,2)<0)
                    {
                        fprintf(stderr,"error when write:%s \n",strerror(errno));
                        exit(1);
                    }
                }
                else if (temp==0x04){
                    reset_input_mode();
                    exit(1);
                }
                else{
                    if(write(1,&temp,1)<0)
                    {
                        fprintf(stderr,"error when write:%s \n",strerror(errno));
                        exit(1);
                    }
                }
            }
        }
        reset_input_mode();
        exit(0);
    }
    else
    {
        signal(SIGPIPE,sigpipehandle);
        if(pipe(pipe_fd_tochild)==-1)
        {
            char* error = strerror(errno);
            fprintf(stderr,"error when pipe: %s \n",error);
            exit(1);
        }
        if(pipe(pipe_fd_fromchild)==-1)
        {
            char* error = strerror(errno);
            fprintf(stderr,"error when pipe: %s \n",error);
            exit(1);
        }

        child_pid = fork();
        if(child_pid > 0) /*parent */
        {
            if(close(pipe_fd_tochild[0])==-1)
            {
                fprintf(stderr,"error when close:%s \n",strerror(errno));
                exit(1);
            }
            if(close(pipe_fd_fromchild[1])==-1)
            {
                fprintf(stderr,"error when close:%s \n",strerror(errno));
                exit(1);
            }
            pollfds[0].fd = STDIN_FILENO;
            pollfds[1].fd = pipe_fd_fromchild[0];
            pollfds[0].events = POLLIN;
            pollfds[1].events = POLLIN;
            while(1)
            {
                int ret;
                ret = poll(pollfds,2,0);
                if (ret< 0)
                {
                    char* error  = strerror(errno);
                    fprintf(stderr,"error when poll: %s \n",error);
                    exit(1);
                }
                if(ret==0)
                    continue;
                if(pollfds[0].revents & POLLIN)
                {
                    int tempread= 0 ;
                    char buf[buffer_size];
                    tempread=read(STDIN_FILENO,buf,buffer_size);
                    if(tempread<0)
                    {
                        fprintf(stderr,"error when read:%s \n",strerror(errno));
                        exit(1);
                    }
                    int i=0;
                    for(;i<tempread;i++)
                    {
                        char temp = buf[i];
                        if(temp ==0x0D||temp==0x0A){
                            char lf= 0x0A;
                            if(write(pipe_fd_tochild[1],&lf,1)==-1)
                            {
                                fprintf(stderr,"error when write:%s \n",strerror(errno));
                                exit(1);
                            }
                            char crlf[2]= {0x0D,0x0A};
                            if(write(STDOUT_FILENO,crlf,2)==-1)
                            {
                                fprintf(stderr,"error when write:%s \n",strerror(errno));
                                exit(1);
                            }                       
                        }
                        else if (temp==0x04){
                            /*close the pipe to the shell, but continue processing input from the shell*/
                            if(close(pipe_fd_tochild[1])==-1)
                            {
                                fprintf(stderr,"error when close:%s \n",strerror(errno));
                                exit(1);
                            }
                        }
                        else if(temp == 0x03)
                        {
                            if(kill(child_pid,SIGINT)==-1)
                            { 
                                char* error = strerror(errno);
                                fprintf(stderr,"error when killed %s \n",error); 
                            }
                        }
                        else{
                            if(write(pipe_fd_tochild[1],buf+i,1)==-1)
                            {
                                fprintf(stderr,"error when write:%s \n",strerror(errno));
                                exit(1);      
                            }
                            if(write(STDOUT_FILENO,&temp,1)==-1)
                            {
                                fprintf(stderr,"error when write:%s \n",strerror(errno));
                                exit(1);                                    
                            }

                        }
                    }
                }
                if(pollfds[0].revents & POLLERR)
                {
                    fprintf(stderr,"error occurred in stdin \n");
                    exit(1);
                }
                if(pollfds[0].revents & POLLHUP)
                {
                    fprintf(stderr,"error occurred in stdin \n");
                    exit(1);
                }
                if(pollfds[1].revents & POLLIN)
                {
                    char buf[buffer_size];
                    int tempread = read(pipe_fd_fromchild[0],buf,buffer_size);
                    int i=  0;
                    for(;i<tempread;i++)
                    {
                        char temp = buf[i];
                        if(temp==0x0A)
                        {
                            char crlf[2] = {0x0D,0x0A};
                            if(write(STDOUT_FILENO,crlf,2)==-1)
                            {
                                fprintf(stderr,"error when write: %s \n",strerror(errno));
                                exit(1);
                            }
                        } 
                        else if(temp==0x04)
                        {
                            if(close(pipe_fd_fromchild[0])==-1)
                            {
                                fprintf(stderr,"error when close: %s \n",strerror(errno));
                                exit(1);
                            }
                            reset_input_mode();
                            int status;
                            if(waitpid(0,&status,0)==-1)
                            {
                                fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
                                exit(1);
                            }
                            fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
                            exit(0);
                        }
                        else{
                            if(write(STDOUT_FILENO,&temp,1)==-1)
                            {
                                fprintf(stderr,"error when write: %s \n",strerror(errno));
                                exit(1);
                            }
                        }
                    }
                }
                if(pollfds[1].revents & POLLERR)
                {
                    if(close(pipe_fd_fromchild[0])==-1)
                    {
                        fprintf(stderr,"error when close: %s \n",strerror(errno));
                        exit(1);
                    }
                    reset_input_mode();
                    int status;
                    if(waitpid(0,&status,0)==-1)
                    {
                        fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
                        exit(1);
                    }
                    fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
                    exit(0);
                }                 
                if(pollfds[1].revents & POLLHUP)
                {
                    if(close(pipe_fd_fromchild[0])==-1)
                    {
                        fprintf(stderr,"error when close: %s \n",strerror(errno));
                        exit(1);
                    }
                    reset_input_mode();
                    int status;
                    if(waitpid(0,&status,0)==-1)
                    {
                        fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
                        exit(1);
                    }
                    fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
                    exit(0);
                }
            }
        }
        else if(child_pid==0) /* child process */
        {
            if(close(pipe_fd_tochild[1])==-1)
            {
                fprintf(stderr,"error when close: %s \n",strerror(errno));
                exit(1);
            }

            if(close(pipe_fd_fromchild[0])==-1)
            {
                fprintf(stderr,"error when close: %s \n",strerror(errno));
                exit(1);
            }
            dup2(pipe_fd_tochild[0],STDIN_FILENO);
            dup2(pipe_fd_fromchild[1],STDOUT_FILENO);
            dup2(pipe_fd_fromchild[1],STDERR_FILENO);
            if(close(pipe_fd_tochild[0])==-1)
            {
                fprintf(stderr,"error when close: %s \n",strerror(errno));
                exit(1);
            }
            if(close(pipe_fd_fromchild[1])==-1)
            {
                fprintf(stderr,"error when close: %s \n",strerror(errno));
                exit(1);
            }
            char* execvp_argv[2];
            char execvp_filename[] = "/bin/bash";
            execvp_argv[0]  = execvp_filename;
            execvp_argv[1] = NULL;
            if(execvp(execvp_filename,execvp_argv)==-1)
            {
                fprintf(stderr,"execvp() failed \n");
                exit(1);
            }
        }
        else
        {
            fprintf(stderr,"fork failed \n");
            exit(1);
        }
        reset_input_mode();
        int finalstatus;
        if(waitpid(0,&finalstatus,0)==-1)
        {
            fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
            exit(1);
        }
        fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(finalstatus),WEXITSTATUS(finalstatus));
        exit(0);
    }
}

