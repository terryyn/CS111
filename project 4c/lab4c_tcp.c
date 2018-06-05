#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mraa.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <poll.h>
#include <sys/socket.h>
#include <netdb.h>

#define buf_size 128
_Bool cdegree = 0;
_Bool fdegree = 1;
int period_time = 1;
char* log_file = "";
FILE* logfd;
_Bool logflag = 0;
mraa_aio_context sensor;
struct pollfd pollfds[2];
_Bool button_flag = 0;
_Bool stop_flag = 0;

char* id = "";
int port_number = -1;
char* host = "";
int sockfd;
struct sockaddr_in serv_addr;
struct hostent *server;

void quit()
{
    if(logflag)
        fclose(logfd);
    mraa_aio_close(sensor);
    close(sockfd);
    return;
}

float gettemperature()
{
    float a;
    a = mraa_aio_read(sensor);
    float R0 = 100000.0;
    float R = 1023.0/a-1.0;
    R = R0*R;
    int B=  4275;
    float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
    if(cdegree)
        return temperature;
    else if(fdegree)
    {
        temperature = temperature*1.8+32;
    }
    return temperature;
}


void parse(const char* command)
{
    if(strcmp(command,"OFF")==0)
    {
        button_flag  =1;
    }
    else if(strcmp(command,"SCALE=F")==0)
    {
        fdegree  =1;
        cdegree = 0;
    }
    else if(strcmp(command,"SCALE=C")==0)
    {
        fdegree  = 0;
        cdegree = 1;   
    }
    else if(command[0]=='S'&&command[1]=='T'&&command[2]=='O'&&command[3]=='P')
    {
        stop_flag = 1;
    }
    else if(strcmp(command,"START")==0)
    {   
        stop_flag = 0;
    }
    else if(command[0]=='L'&&command[1]=='O'&&command[2]=='G')
    {
        ;
    }
    else if(command[0] == 'P' && command[1] == 'E' && command[2] == 'R' && command[3] == 'I' && command[4] == 'O' && command[5] == 'D'&&command[6]=='=')
    {
        int new_period = atoi(command+7);
        if(new_period<0)
        {
            fprintf(stderr,"invalid period\n");
            quit();
            exit(1);
        }
        period_time  =new_period;
    }
    else
    {
        fprintf(stderr,"invalid command\n");
        quit();
        exit(1);
    }
}
void wait_button()
{
    const time_t sample_time  = time(NULL);
    struct tm* local_time = localtime(&sample_time);
    dprintf(sockfd,"%02d:%02d:%02d SHUTDOWN",local_time->tm_hour,local_time->tm_min,local_time->tm_sec);
    if(logflag)
    {
        fprintf(logfd,"%02d:%02d:%02d SHUTDOWN",local_time->tm_hour,local_time->tm_min,local_time->tm_sec);
    }
    quit();
    exit(0);
}

void sample()
{ 
    if(button_flag)
    {
        ;
    }
    if(!stop_flag){
        float temperature = gettemperature();
        const time_t sample_time  = time(NULL);
        struct tm* local_time = localtime(&sample_time);
        dprintf(sockfd,"%02d:%02d:%02d %.1f\n",local_time->tm_hour,local_time->tm_min,local_time->tm_sec,temperature);
        if(logflag)
        {
            fprintf(logfd,"%02d:%02d:%02d %.1f\n",local_time->tm_hour,local_time->tm_min,local_time->tm_sec,temperature);
        }
    }

    return ;     
}
int main(int argc,char**argv)
{

    static struct option long_options[] = 
    {
        {"scale",required_argument,0,'s'},
        {"period",required_argument,0,'p'},
        {"log",required_argument,0,'l'},
        {"id",required_argument,0,'i'},
        {"host",required_argument,0,'h'},
        {0,0,0,0}
    };
    while(1)
    {
        int c = getopt_long(argc,argv,"s:p:l:i:h:",long_options,NULL);
        if(c==-1)
            break;
        else if(c=='s'){
            if(optarg[0]=='C')
                cdegree = 1;
            else if(optarg[0]=='F')
                fdegree =1;
            else{
                fprintf(stderr,"error:invalid scale\n");
                exit(1);
            }
        }
        else if(c=='p')
        {
            period_time  = atoi(optarg);
        }
        else if(c=='l')
        {
            logflag = 1;
            log_file = optarg;
            logfd = fopen(log_file,"w");
            if(logfd==NULL)
            {
                fprintf(stderr,"error when create log file\n");
                exit(1);
            }
        }
        else if(c=='i')
        {
            id = optarg;
        }
        else if(c=='h')
        {
            host = optarg;
        }
        else
        {
            fprintf(stderr,"error:invalid argument\n");
            exit(1);
        }
    }
    if(strcmp(id,"")==0)
    {
        fprintf(stderr,"no id argument\n");
        exit(1);
    }
    if(strcmp(host,"")==0)
    {
        fprintf(stderr,"no host argument\n");
        exit(1);
    }
    if(strcmp(log_file,"")==0)
    {
        fprintf(stderr,"no log argument\n");
        exit(1);
    }
    port_number = atoi(argv[argc-1]);
    if(port_number == -1)
    {
        fprintf(stderr,"no port argument\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd<0)
    {
        fprintf(stderr,"socket error\n");
        exit(2);
    }
    server= gethostbyname(host);
    if(server==NULL)
    {
        fprintf(stderr,"server error\n");
        exit(2);
    } 
    memset((char *) &serv_addr, 0,sizeof(serv_addr));
    memcpy((char *)&serv_addr.sin_addr.s_addr,(char *)server->h_addr, server->h_length); 
    serv_addr.sin_port = htons(port_number); 
    serv_addr.sin_family = AF_INET;
    if (connect(sockfd , (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr,"error:connect failed\n");
        exit(2);
    }


  
    sensor = mraa_aio_init(1);
    pollfds[0].fd = sockfd;
    pollfds[0].events = POLLIN;
    time_t new_time = time(NULL);
    time_t old_time = time(NULL);
    char temp[20] = "";
    int index = 0;
    dprintf(sockfd,"ID=%s\n",id);
    fprintf(logfd,"ID=%s\n",id);
    while(1)
    {
        int ret = poll(pollfds,1,0);
        if(ret<0)
        {
            fprintf(stderr,"error when poll\n");
            exit(1);
        }
        if(button_flag)
        {
            wait_button();
        }
        new_time = time(NULL);
        if(new_time-old_time>=period_time)
        {
            old_time = new_time;    
            sample();
        }
        if(pollfds[0].revents&POLLIN)
        {
            char buf[buf_size];
            int read_size=read(sockfd,buf,buf_size);
            if(read_size<0)
            {
                fprintf(stderr,"error when read\n");
                quit();
                exit(1);
            }
            int i=0;
            for(;i<read_size;i++)
            {
                if(buf[i]!='\n')
                {
                    temp[index]=buf[i];
                    index++;
                }
                else
                {
                    char command[20];
                    strcpy(command,temp);
                    if(logflag)
                    {
                        fprintf(logfd,"%s\n",command);
                    }
                    parse(command);
                    memset(temp,0,20);
                    index = 0;
                }
            }
        }
    }
    quit();
    exit(0);
}