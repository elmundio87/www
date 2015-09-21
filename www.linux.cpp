#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> //for cgi
#include <pthread.h>
#include <iostream>
#include <fcntl.h> 
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

using namespace std;


extern void *serve(void *);

int main(int argc, char *argv[]){
  
  
  int port = htons(atoi(argv[1]));
  char buffer[4096];
  int ssock, connsock, stat, r;

  struct sockaddr_in myaddr;
  ssock = socket(AF_INET, SOCK_STREAM, 0);
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = port;
  stat=bind(ssock,(struct sockaddr *)&myaddr,sizeof(myaddr));
  if(stat < 0) { cerr << "cant bind\n"; exit(1);}
  listen(ssock,5); 

  pthread_t th; 
  while(true) {
    connsock=accept(ssock,0,0);
    r=pthread_create(&th,0,serve,(void *)connsock);
  }
}


void *serve(void *s) {
  int sock = (int)(size_t)s, rc;
  char buffer[2048];
  
   struct stat statbuf; //for returning file information
  time_t timeinsecs, mtime;
  
  char rep[2048]; //a string used when replying
  char fullpath[2048] = "pages" ;//the filepath of server files
  
  long fsize;
  
  int n, fd;
  char req[16], resource[128], proto[16], *exts;  //identifying parts of the request
   
  pthread_detach(pthread_self());
  rc = read(sock,buffer,2048);

  n=sscanf(buffer,"%s %s %s", req, resource, proto); //split the request into request type, resource and protocol
 
  strcpy(rep, proto); //insert protocol into the reply header
 
  if(strcmp(resource,"/")==0)  //if resource requested is "/", use index.html
	{
	strcpy(resource,"/index.html"); //replace resource name with index.html
	strcat(fullpath,resource);  //add the resource name to the filepath
	}
	else
	{
	strcat(fullpath,resource); //add  the resource name to the filepath
	}
	
  
  
   if (!strcmp(req,"GET")==0) //if the request type is not GET, send back 501 not implemented
 {
 strcat(rep, " 501 Not implemented");
 write(sock,rep,strlen(rep));
 write(sock, "\n\rConnection: close\r\n", strlen("\n\rConnection: close\r\n")); //send the "Connection: close" string
 write(sock, "\r\n", strlen("\r\n"));
 write(sock, "<TITLE>501 Not implemented</TITLE>", strlen("<TITLE>501 Not implemented</TITLE>"));
 write(sock, "<H1>Request invalid</H1>", strlen("<H1>Request invalid</H1>"));
 goto logfile; //skip to logfile creation
 }
  
  
  if(access(fullpath,F_OK) < 0 ) { //if the file doesn't exist, print a 404 error, else 
	
	strcat(rep, " 404 File not found\r\n\r\n");
	strcat(rep, "<TITLE>404 Not Found</TITLE>");
	strcat(rep, "<H1>Not Found</H1>");
	strcat(rep, "The requested URL ");
	strcat(rep, resource);
	strcat(rep, " was not found on this server.<P>");
	strcat(rep, "<body><img src='404.jpg'></img></body>");
	write(sock,rep,strlen(rep));
	goto logfile; //skip to logfile creation;
  	} 
	else 
	{
 
  
	strcat(rep, " 200 OK\r\n");  //else print a normal 200 OK
    write(sock,rep,strlen(rep));
 

	

	if(strrchr(fullpath, '.') > 0) 	//if the file has an extension -.-
	{
	exts = strrchr(fullpath, '.'); //get the file extension of the resource
	}	


		
write(sock, "Connection: close\r\n", strlen("Connection: close\r\n")); //send the "Connection: close" string
	
if((strcmp(exts,".cgi")==0)) //if a cgi script
{

int pid = fork(); 

if(pid == 0){
close(1);
dup2(sock,1); //redirect outstream to socket
execlp(fullpath, NULL); //execute the script

}

goto logfile; //skip to logfile creation

}	
	
		  
stat(fullpath,&statbuf); //return the time that the resource was last modified
mtime = statbuf.st_mtime;

char *temptime;
temptime = ctime(&mtime);
  
write(sock, "Last-Modified: ", strlen("Last-Modified: ")); //print that time as part of a "Last-Modified" string
write(sock, temptime,strlen(temptime));
  

stat(fullpath, &statbuf); //return the size of the file as Long
fsize = statbuf.st_size; 

char tempsize[10];

sprintf(tempsize, "%i", fsize); //convert the Long to char

write(sock, "Content-Length: ", strlen("Content-Length: ")); //print the "Content-Length" string 
write(sock, tempsize, strlen(tempsize));
write(sock, "\r\n", strlen("\r\n"));
  
//send a Content-Type string, with the correct content type		
if((strcmp(exts,".jpg")==0) ||  (strcmp(exts,".jpeg")==0) || (strcmp(exts,".gif")==0) || (strcmp(exts,".bmp")==0) || (strcmp(exts,".png")==0))
{
write(sock, "Content-Type: image/jpeg\r\n", strlen("Content-Type: image/jpeg\r\n"));
}
else if((strcmp(exts,".txt")==0))
{
write(sock, "Content-Type: text/plain\r\n", strlen("Content-Type: text/plain\r\n"));
}  
else 
{
write(sock, "Content-Type: text/html\r\n", strlen("Content-Type: text/html\r\n"));
}
  
  
  
  
  
  
  
  
write(sock, "\r\n", strlen("\r\n"));  //Send a gap to the socket, anything below it will be data
  
int fd, readf;
char line[2048];

fd = open(fullpath,O_RDONLY); //open the resource file
readf = read(fd,line,2048);
while(readf > 0) //while the file is readble
{
write(sock,line, 2048); //read the file 1 bits at a time, and write to the socket
readf = read(fd,line,2048);
}

logfile:

 write(1,buffer,rc); //write buffer to the window
close(fd);

char dates[64];
struct tm tmbuf;

  time_t timeinsecs = time(0);
  gmtime_r(&timeinsecs, &tmbuf);
strftime(dates,64,"%H:%M, %d/%m/%y",&tmbuf);



char logtext[1024] ; //a char array to hold the line to be written to the log file

  int s = (int)sock, rc, r;
socklen_t cliaddrlen;
 struct sockaddr_in cliaddr;
char *hostip;
unsigned short port;

  cliaddrlen = sizeof(struct sockaddr_in);
  r=getpeername(s,(struct sockaddr *) &cliaddr,&cliaddrlen);

    hostip = inet_ntoa(cliaddr.sin_addr);
    port = ntohs(cliaddr.sin_port);
 




strcpy(logtext, dates); //add the current date to the log string
strcat(logtext,": <");

strcat(logtext,hostip);

strcat(logtext,"> ");
strcat(logtext,req);//add the request type to the log string
strcat(logtext," ");
strcat(logtext,resource); //add the resource to the log string
strcat(logtext," ");
strcat(logtext,proto); //add the protocol to the log string
strcat(logtext,"\r\n");




fd = open("pages/log.txt",O_WRONLY|O_APPEND); //open the log file
write(fd,logtext,strlen(logtext)); //write the log string to the log file
close(fd);



close(sock);pthread_exit(0); //exit the thread, closing any open files and sockets
}
}
