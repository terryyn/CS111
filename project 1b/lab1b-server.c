#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
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
#include "zlib.h"

z_stream stdin_to_shell;
z_stream shell_to_stdout;
int sockfd;
int buffer_size = 2048;
int newsockfd;
unsigned int clilen;
_Bool compress_flag = 0;
int pipe_fd_tochild[2];
int pipe_fd_fromchild[2];
struct pollfd pollfds[2];
pid_t child_pid = -1;
char inflate_buf[2048];

void sigpipehandle()
{
    if(close(pipe_fd_fromchild[0])==-1)
    {
        fprintf(stderr,"error when close: %s \n",strerror(errno));
        close(sockfd);
        close(newsockfd);
        exit(1);
    }
    int status;
    if(waitpid(0,&status,0)==-1)
    {
        fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
        exit(1);
    }
    fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
    close(sockfd);
    close(newsockfd);
    exit(0);
}

void check(int a,char* s)
{
    if(a<0)
    {
        fprintf(stderr,"error when %s: %s\n",s,strerror(errno));
        close(sockfd);
        close(newsockfd);
        if(compress_flag){
            inflateEnd(&shell_to_stdout);
            deflateEnd(&stdin_to_shell);
        }
        exit(1);
    }
}

int main(int argc,char** argv)
{

    int port_number=-1;

    struct sockaddr_in serv_addr,cli_addr;
    static struct option long_options[] =
    {
        {"port",required_argument,0,'p'},
        {"compress",no_argument,0,'c'},
        {0,0,0,0}
    };
    while(1)
    {
        int c = getopt_long(argc,argv,"s:c:",long_options,NULL);
        if(c==-1)
            break;
        if(c=='p')
            port_number = atoi(optarg);
        else if (c=='c')
            compress_flag =1;
        else{
            fprintf(stderr,"invalid argument\n");
            exit(1);
        }
    }
    if(port_number ==-1)
    {
        fprintf(stderr,"missing port argument\n");
        exit(1);
    }
    if(compress_flag)
    {
        stdin_to_shell.zalloc = Z_NULL;
        stdin_to_shell.zfree = Z_NULL;
        stdin_to_shell.opaque = Z_NULL;

        if(deflateInit(&stdin_to_shell,Z_DEFAULT_COMPRESSION)!=Z_OK)
        {
            fprintf(stderr,"error when deflateInit\n");
            exit(1);
        }
        shell_to_stdout.zalloc = Z_NULL;
        shell_to_stdout.zfree = Z_NULL;
        shell_to_stdout.opaque = Z_NULL;

        if(inflateInit(&shell_to_stdout)!=Z_OK)
        {
            fprintf(stderr,"error when inflateInit\n");
            exit(1);
        }
    }
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
    {
        fprintf(stderr,"error when accept:%s\n",strerror(errno));
        exit(1);
    }  
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_number);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr,"error when binding\n");
        close(sockfd);
        exit(1);
    }
    listen(sockfd,5);    
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);   
    if(newsockfd<0)
    {
        fprintf(stderr,"error when accept:%s\n",strerror(errno));
        close(sockfd);
        exit(1);
    }
        /*child process*/
    if(pipe(pipe_fd_tochild)==-1)
    {
        char* error = strerror(errno);
        fprintf(stderr,"error when pipe: %s \n",error);
        close(sockfd);
        close(newsockfd);
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
        signal(SIGPIPE,sigpipehandle);
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
        pollfds[0].fd = newsockfd;
        pollfds[1].fd = pipe_fd_fromchild[0];
        pollfds[0].events = POLLIN;
        pollfds[1].events = POLLIN;  
        while(1)
        {
            int ret = poll(pollfds,2,0);
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
                tempread=read(newsockfd,buf,buffer_size);
                if(tempread<0)
                {
                    fprintf(stderr,"error when read:%s \n",strerror(errno));
                    exit(1);
                }
                if(compress_flag)
                {

                    shell_to_stdout.avail_in = tempread;
                    shell_to_stdout.next_in = (Bytef *)buf;
                    shell_to_stdout.avail_out = (uInt)sizeof(inflate_buf);
                    shell_to_stdout.next_out = (Bytef *)inflate_buf;
                    do{
                        inflate(&shell_to_stdout,Z_SYNC_FLUSH);
                    }while(shell_to_stdout.avail_in>0);
                    int size = buffer_size-shell_to_stdout.avail_out;
                    tempread = size;
                }
                int i=0;
                for(;i<tempread;i++)
                {
                    char temp;
                    if(!compress_flag)
                        temp = buf[i];
                    else
                        temp = inflate_buf[i];
                    if(temp ==0x0D||temp==0x0A){
                        char lf= 0x0A;
                        if(write(pipe_fd_tochild[1],&lf,1)==-1)
                        {
                            fprintf(stderr,"error when write:%s \n",strerror(errno));
                            close(sockfd);
                            close(newsockfd);
                            exit(1);
                        }                   
                    }
                    else if (temp==0x04){
                        /*close the pipe to the shell, but continue processing input from the shell*/
                        if(close(pipe_fd_tochild[1])==-1)
                        {
                            fprintf(stderr,"error when close:%s \n",strerror(errno));
                            close(sockfd);
                            close(newsockfd);
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
                        if(write(pipe_fd_tochild[1],&temp,1)==-1)
                        {
                            fprintf(stderr,"error when write:%s \n",strerror(errno));
                            close(sockfd);
                            close(newsockfd);
                            exit(1);      
                        }
                    }
                }
            }
            if(pollfds[0].revents & POLLERR)
            {
                fprintf(stderr,"error occurred in stdin \n");
                break;
            }
            if(pollfds[0].revents & POLLHUP)
            {
                fprintf(stderr,"error occurred in stdin \n");
                break;
            }
            if(pollfds[1].revents & POLLIN)
            {
                char buf[buffer_size];
                int tempread = read(pipe_fd_fromchild[0],buf,buffer_size);
                if(compress_flag)
                {
                    stdin_to_shell.avail_in = tempread;
                    stdin_to_shell.next_in = (Bytef *)buf;
                    stdin_to_shell.avail_out = buffer_size;
                    char compress_buf[2048];
                    stdin_to_shell.next_out = (Bytef *)compress_buf;
                    do{
                        deflate(&stdin_to_shell,Z_SYNC_FLUSH);
                    }while(stdin_to_shell.avail_in>0);                    
                    int size = 2048-stdin_to_shell.avail_out;
                    write(newsockfd,compress_buf,size);
                }
                else
                {
                    int i=  0;
                    for(;i<tempread;i++)
                    {
                        char temp = buf[i];
                        if(write(newsockfd,&temp,1)==-1)
                        {
                            fprintf(stderr,"error when write: %s \n",strerror(errno));
                            close(sockfd);
                            close(newsockfd);
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
                    close(sockfd);
                    close(newsockfd);
                    exit(1);
                }
                int status;
                if(waitpid(0,&status,0)==-1)
                {
                    fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
                    close(sockfd);
                    close(newsockfd);
                    exit(1);
                }
                fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
                close(sockfd);
                close(newsockfd);
                exit(0);
            }                 
            if(pollfds[1].revents & POLLHUP)
            {
                if(close(pipe_fd_fromchild[0])==-1)
                {
                    fprintf(stderr,"error when close: %s \n",strerror(errno));
                    close(sockfd);
                    close(newsockfd);
                    exit(1);
                }
                int status;
                if(waitpid(0,&status,0)==-1)
                {
                    fprintf(stderr,"error when waitpid: %s \n",strerror(errno));
                    close(sockfd);
                    close(newsockfd);
                    exit(1);
                }
                fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d \n",WTERMSIG(status),WEXITSTATUS(status));
                close(sockfd);
                close(newsockfd);        
                exit(0);
            }
        }
    }
    else if(child_pid==0)
    {
        if(close(pipe_fd_tochild[1])==-1)
        {
            fprintf(stderr,"error when close: %s \n",strerror(errno));
            close(sockfd);
            close(newsockfd);
            exit(1);
        }

        if(close(pipe_fd_fromchild[0])==-1)
        {
            fprintf(stderr,"error when close: %s \n",strerror(errno));
            close(sockfd);
            close(newsockfd);
            exit(1);
        }
        dup2(pipe_fd_tochild[0],STDIN_FILENO);
        dup2(pipe_fd_fromchild[1],STDOUT_FILENO);
        dup2(pipe_fd_fromchild[1],STDERR_FILENO);
        if(close(pipe_fd_tochild[0])==-1)
        {
            fprintf(stderr,"error when close: %s \n",strerror(errno));
            close(sockfd);
            close(newsockfd);
            exit(1);
        }
        if(close(pipe_fd_fromchild[1])==-1)
        {
            fprintf(stderr,"error when close: %s \n",strerror(errno));
            close(sockfd);
            close(newsockfd);
            exit(1);
        }
        char* execvp_argv[2];
        char execvp_filename[] = "/bin/bash";
        execvp_argv[0]  = execvp_filename;
        execvp_argv[1] = NULL;
        if(execvp(execvp_filename,execvp_argv)==-1)
        {
            fprintf(stderr,"execvp() failed \n");
            close(sockfd);
            close(newsockfd);
            exit(1);
        }
    }
    else
    {
        fprintf(stderr,"fork failed \n");
        close(sockfd);
        close(newsockfd);
        exit(1);
    }
    close(sockfd);
    close(newsockfd);
    exit(0);
}