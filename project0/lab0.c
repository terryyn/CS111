#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void sighandler(int);

int main(int argc,char ** argv)
{
  int c;
  _Bool existsegfault = 0;
  _Bool existcatch = 0;
  _Bool existinput = 0;
  _Bool existoutput = 0;
  char* inputfile = NULL;
  char* outputfile =NULL;
  while(1)
    {
      static struct option long_options[]=
        {
          {"input",required_argument,0,'a'},
          {"output",required_argument,0,'b'},
          {"segfault",no_argument,0,'c'},
          {"catch",no_argument,0,'d'},
          {0,0,0,0}
        };
      c = getopt_long(argc,argv,"a:b:c:d:",long_options,NULL);
      if(c==-1)
        break;
      switch(c)
        {
        case 'a':
          existinput = 1;
          inputfile = optarg;
          break;
        case 'b':
          existoutput = 1;
          outputfile = optarg;
          break;
        case 'c':
          existsegfault = 1;
          break;
        case 'd':
          existcatch = 1;
          break;
        default:
          fprintf(stderr,"Usage: lab0 -input=inputfilename -output=outputfilename -segfault -catch \n");
          exit(1);
        }
    }
  if (existinput){
    int inputfile_desc = open(inputfile,O_RDONLY);
    if(inputfile_desc<0){
      char* message = strerror(errno);
      fprintf(stderr,"--input argument wrong, %s cannot be opened, error is %s \n",inputfile,message);
      exit(2);
    }
    close(0); 
    dup(inputfile_desc);
    close(inputfile_desc);
  }
  if (existoutput){
    int outputfile_desc = creat(outputfile,0666);
    if(outputfile_desc<0){
      char* message = strerror(errno);
      fprintf(stderr,"--output argument wrong, %s cannot be created, error is %s \n",outputfile,message);
      exit(3);
    }
    close(1);
    dup(outputfile_desc);
    close(outputfile_desc);
  }
  if (existcatch)
    signal(SIGSEGV,sighandler);
  if (existsegfault){
    char* temp = NULL;
    *temp = 'a';  
  }

  /*if there is no fault or error */
  int tempread= 0;
  char* buf = (char*)malloc(sizeof(char));
  while((tempread = read(0,buf,1))>0){
    write(1,buf,1);
  }
  free(buf);
  exit(0);
}


void sighandler(int a)
{
    fprintf(stderr,"Signal number is %d ,catch segmentation error \n",a);
    exit(4);
}