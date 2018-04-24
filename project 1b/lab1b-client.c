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
#include <signal.h>
#include <arpa/inet.h>
#include "zlib.h"

struct termios saved_attributes;
struct pollfd pollfds[2];
int buffer_size = 2048;
_Bool compressflag= 0;
_Bool log_flag = 0;
int sockfd;
int logfd;
char inflate_buf[2048];
z_stream stdin_to_shell;
z_stream shell_to_stdout;



void reset_terminal_mode()
{
    tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
    close(sockfd);
}

void check(int a,char* s)
{
    if(a<0)
    {
        reset_terminal_mode();
        fprintf(stderr,"error when %s: %s\n",s,strerror(errno));
        if(compressflag){
            inflateEnd(&shell_to_stdout);
            deflateEnd(&stdin_to_shell);
        }
        exit(1);
    }
}

int main(int argc,char** argv)
{
    int port_number = -1;
    char* log_name ;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    static struct option long_options[] = 
    {
        {"port",required_argument,0,'p'},
        {"log",required_argument,0,'l'},
        {"compress",no_argument,0,'c'},
        {0,0,0,0}
    };
    while(1)
    {
        int c = getopt_long(argc,argv,"p:l:c:",long_options,NULL);
        if(c==-1)
            break;
        switch(c)
        {
            case'p':
                port_number = atoi(optarg);
                break;
            case 'l':
                log_name = optarg;
                log_flag = 1;
                logfd = creat(log_name,0666);
                if(logfd<0)
                {
                    fprintf(stderr,"error:%s \n",strerror(errno));
                    exit(1);
                }
                break;
            case 'c':
                compressflag = 1;
                break;
            default:
                fprintf(stderr,"invalid argument");
                exit(1);
        }
    }
    if(port_number ==-1)
    {
        fprintf(stderr,"missing port argument \n");
        exit(1);
    }
    if(compressflag)
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


    struct termios current_mode;
    tcgetattr(0,&current_mode);
    tcgetattr(0,&saved_attributes);
    current_mode.c_iflag = ISTRIP;
    current_mode.c_oflag = 0;
    current_mode.c_lflag = 0;
    tcsetattr(STDIN_FILENO,TCSANOW,&current_mode);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    check(sockfd,"socket");
    server= gethostbyname("localhost"); 
    memset((char *) &serv_addr, 0,sizeof(serv_addr));
    serv_addr.sin_port = htons(port_number);
    memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);    
    serv_addr.sin_family = AF_INET;
    if (connect(sockfd , (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        reset_terminal_mode();
        fprintf(stderr,"error:connect failed");
        exit(1);
    }

    pollfds[0].fd=STDIN_FILENO;
    pollfds[1].fd = sockfd;
    pollfds[0].events = POLLIN;
    pollfds[1].events = POLLIN;
    while(1)
    {
        int ret;
        ret = poll(pollfds,2,0);
        check(ret,"poll");
        if(ret==0)
            continue;
        if(pollfds[0].revents&POLLIN)
        {
            int tempread= 0 ;
            char buf[buffer_size];
            tempread=read(STDIN_FILENO,buf,buffer_size);
            check(tempread,"read from stdin");
            if(compressflag)
            { 
                stdin_to_shell.avail_in = tempread;
                stdin_to_shell.next_in = (Bytef *)buf;
                stdin_to_shell.avail_out = buffer_size;
                char compress_buf[2048];
                stdin_to_shell.next_out = (Bytef *)compress_buf;
                do{
                    deflate(&stdin_to_shell,Z_SYNC_FLUSH);
                }while(stdin_to_shell.avail_in>0);                    
                int size = buffer_size-stdin_to_shell.avail_out;
                write(sockfd,compress_buf,size);
                if(log_flag)
                {
                    char message[100];
                    int sprintf_result1 = sprintf(message,"SENT %d bytes: ",size);
                    check(sprintf_result1,"sprintf");
                    int write_temp = write(logfd,message,sprintf_result1);
                    check(write_temp,"write to log");
                    write(logfd,compress_buf,size);
                    char *lf = "\n";
                    write(logfd,lf,1);
                } 
            }
            int i=0;
            for(;i<tempread;i++)
            {
                char temp = buf[i];
                if(temp==0x0A||temp==0x0D)
                {
                    char crlf[2]= {0x0D,0x0A};
                    int write_result = write(STDOUT_FILENO,crlf,2);
                    check(write_result,"write");
                }
                else
                {
                    int write_result = write(STDOUT_FILENO,&temp,1);
                    check(write_result,"write");
                }
                if(!compressflag)
                {
                    int send_result = write(sockfd,buf+i,1);
                    check(send_result,"send");
                }
                if(log_flag && !compressflag)
                {
                    char* message = "SENT 1 bytes: ";
                    int write1= write(logfd,message,14);
                    check(write1,"write");
                    char messageend[2] = {temp,'\n'};
                    int write2 = write(logfd,messageend,2);
                    check(write2,"write");
                }
            }
        }
        if(pollfds[0].revents&POLLERR)
        {
            fprintf(stderr,"error in stdin\n");
            break;
        }
        if(pollfds[0].revents&POLLHUP)
        {
            fprintf(stderr,"error in stdin\n");
            break;
        }
        if(pollfds[1].revents&POLLIN)
        {
            int tempread= 0 ;
            char buf[buffer_size];
            tempread=read(sockfd,buf,buffer_size);
            check(tempread,"read from server");
            if(tempread==0)
                break;
/*            if(log_flag&&!compressflag)
            {
                char* message;
                int sprintf_result = sprintf(message,"RECEIVED %d bytes: ",tempread);
                check(sprintf_result,"sprintf");
                int write_temp = write(logfd,message,sprintf_result);
                check(write_temp,"write to log");
                write(logfd,buf,tempread);
                char* lf ="\n";
                write(logfd,lf,1);
            }
*/
            if(log_flag&&compressflag)
            {
                char message[100];
                int sprintf_result = sprintf(message,"RECEIVED %d bytes: ",tempread);
                check(sprintf_result,"sprintf");
                int write_temp = write(logfd,message,sprintf_result);
                check(write_temp,"write to log");
                write(logfd,buf,tempread);
                char* lf ="\n";
                write(logfd,lf,1);

            }
            if(compressflag)
            {
                shell_to_stdout.avail_in = tempread;
                shell_to_stdout.next_in = (Bytef *)buf;
                shell_to_stdout.avail_out = buffer_size;

                shell_to_stdout.next_out = (Bytef *)inflate_buf;
                do{
                    inflate(&shell_to_stdout,Z_SYNC_FLUSH);
                }while(shell_to_stdout.avail_in>0);
                int size = buffer_size-shell_to_stdout.avail_out;
                /*int write_temp = write(STDOUT_FILENO,inflate_buf,size);
                check(write_temp,"write to stdout");*/
                tempread=size;
            }
            int i=0;
            for(;i<tempread;i++)
            {
                char temp;
                if(compressflag)
                    temp = inflate_buf[i];
                else
                    temp = buf[i];
                if(temp==0x0A||temp==0x0D)
                {
                    char crlf[2]= {0x0A,0x0D};
                    write(STDOUT_FILENO,crlf,2);
                }
                else
                {
                    write(STDOUT_FILENO,&temp,1);
                    if(log_flag&&!compressflag)
                    {
                        char message[100];
                        int sprintf_result = sprintf(message,"RECEIVED 1 bytes: %c\n",temp);
                        write(logfd,message,sprintf_result);
                    }
                }
            }
        }
        if(pollfds[1].revents&POLLHUP)
        {
            reset_terminal_mode();
            fprintf(stderr,"error in server\n");
            exit(0);
        }
        if(pollfds[1].revents&POLLERR)
        {
            reset_terminal_mode();
            fprintf(stderr,"error in server\n");
            exit(0);
        }

    }
    reset_terminal_mode();
    exit(0);
}


