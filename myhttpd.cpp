
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <string>
#include <pthread.h>


int QueueLength = 5;

// Processes time request
void processRequest( int socket );
void processRequestThread( int socket );
bool endsWith(char* string, char* ending);
bool isCGI(char* docPath);

const char* htmlhead = "HTTP/1.0 Document follows\r\nServer: IanWhitlockServer\r\nContent-type: text/html\r\n\r\n";
const char* gifhead = "HTTP/1.0 Document follows\r\nServer: IanWhitlockServer\r\nContent-type: image/gif\r\n\r\n";
const char* jpghead = "HTTP/1.0 Document follows\r\nServer: IanWhitlockServer\r\nContent-type: image/jpg\r\n\r\n";
const char* texthead = "HTTP/1.0 Document follows\r\nServer: IanWhitlockServer\r\nContent-type: text/plain\r\n\r\n";



int
main( int argc, char ** argv )
{
  // Print usage if not enough arguments


  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }

  int mode = 0;
	int port;
  
  // Get the port from the arguments

  if ( argv[1][0] != '-' ) {
  	port = atoi( argv[1] );
  } else {
		port = atoi(argv[2]);
		if ( argv[1][1] == 'f' ) mode = 1;
		else if ( argv[1][1] == 't' ) mode = 2;
		else if ( argv[1][1] == 'p') mode = 3;
		else {
			fprintf( stderr, "%s", usage);
			exit(-1);
		}
  }

  
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

	int pid;
  while ( 1 ) {

    // Accept incoming connections
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);
		
    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }
		
		switch (mode) {
    // Process request.
			case(0): //no threading
    		processRequest(slaveSocket);
				close( slaveSocket);
				break;
			case(1): //processes
				(void) signal(SIGCHLD, SIG_IGN);
				pid = fork();
				if ( pid < 0 ) {
					perror( "fork" );
				} else if ( pid == 0 ) {
					processRequest(slaveSocket);
    			close( slaveSocket );
					exit(0);
				}
				close(slaveSocket);
				break;
			case(2): //threads
				pthread_t thr;
				pthread_attr_t attr;

				pthread_attr_init( &attr );
				pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

				pthread_create( &thr, &attr, (void * (*)(void *)) processRequestThread, (void *) slaveSocket );
				
				
				break;
			case(3): //pool of threads
				break;

		}
//    processTimeRequest( slaveSocket );

    // Close socket
  }
  
}


void
processRequestThread(int socket) {
	processRequest(socket);
	close(socket);
	
}

void
processRequest ( int socket ) {

	unsigned char newChar;
	unsigned char oldChar = 0;
	const int MaxName = 2048;
	char curr_string  [ MaxName + 1 ];
	int length = 0;
	int gotGet = 0; 
	int n = 0;


	while(n = read(socket, &newChar, sizeof(newChar))) {
		if (newChar == '\n' && oldChar == '\r') break;
		oldChar = newChar;
		length++;
		curr_string[length-1] = newChar;
	}

	if ( curr_string[0] == '\0' ) return;

	char* currToken = strtok(curr_string, " ");
	if ( strcmp(currToken,"GET") ) {
		write(socket, "400 Bad Request\n", 16);
		return;
	}

	char* docpath = strtok(NULL, " ");	



//	char* cwd = (char*)malloc(256);
	char fullpath [ 512 ]  = {0};
	
	getcwd(fullpath,256);	
//	strcat(fullpath, "/http-root-dir/htdocs");
//	strcat(fullpath, docpath);

	if ( isCGI(docpath) ) {
		const char* head = "HTTP/1.0 Document follows\r\nServer: IanWhitlockServer\r\n";
		strcpy(fullpath, (std::string(fullpath)+std::string("/http-root-dir")+std::string(docpath)).c_str());
		int savestdout = dup(1);
		char* args[256] = {0};
		int cnt = 0;
		args[0] = strtok(fullpath, "?");
		while ( args[cnt] != NULL ) {
			cnt++;
			args[cnt] = strtok(NULL, "&");
		} 
		printf("%s, %d\n",args[0], cnt);
		args[cnt] = NULL;

		//std::string qry = "QUERY_STRING=";
		if (cnt > 1) {
			putenv("REQUEST_METHOD=GET");
	//		for ( int i = 1; i < cnt-1; i++ ) {
		//		qry = qry + std::string("&") + std::string(args[cnt]);
		//	}
		//	char* qrystring = strcpy(qrystring, qry.c_str());	
		//	putenv(qrystring);
		}
		int ret;
		(void) signal(SIGCHLD, SIG_IGN);
		ret = fork();
		if ( ret == 0 ) { 
			write(socket, head, strlen(head));
			dup2(socket,1);	
			execvp(args[0],args);
		} 
		else if (ret <= 0 ) { 
			perror( "fork" );
			exit(0);

		} else return;
	} else {

		printf("%s\n",fullpath);
		strcpy(fullpath, (std::string(fullpath)+std::string("/http-root-dir/htdocs")+std::string(docpath)).c_str());
		if ( fullpath[strlen(fullpath)-1] == '/' ) 
			strcpy(fullpath, (std::string(fullpath)+std::string("index.html")).c_str());
		//strcat(fullpath, "index.html");

		printf("%s\n",docpath);

//	if ( endsWith("this.html",".htm") ) printf("true\n");
		int file = open(fullpath,O_RDONLY);
		if ( file  == -1 ) {
			perror("open");
			write(socket,"404\n",4);
		}
		else {
			if (endsWith(fullpath, (const char*)"gif")) write(socket, gifhead, strlen(gifhead));
			else write(socket, htmlhead, strlen(htmlhead));

			int count;
			void* buffer = malloc(1);
			while(count = read(file, buffer, 1)) {
				if ( write(socket, buffer, 1) != count ) {
					perror("write");
					break;
				}
			}
			free(buffer);

		}

	free(currToken);
	close(file);
	}
	
//	write(socket, "<html><body bgcolor=black>", 26);
	free(fullpath);
	free(curr_string);
	free(docpath);

}


bool
endsWith(char* str, char* ending) {
	int stringlen = strlen(str);
	int endlen = strlen(ending);
	
	if ( endlen > stringlen ) return false;
	
	int i;
	for ( i = 0; i < endlen; i++ ) {
		if ( str[stringlen-i-1] != ending[endlen-i-1] ) return false; 
	}	
	return true;


}


bool
isCGI (char* docPath) {
	
	if (&docPath == '\0' ) return false;
	const char* cgibin = "/cgi-bin/";
	if ( strlen(docPath) < 9 ) return false;
	int i;
	for ( i = 0; i < 9; i++) {
		if (docPath[i] != cgibin[i]) return false;
	}

	return true;
}




/*void
processTimeRequest( int fd )
{
  // Buffer used to store the name received from the client
  const int MaxName = 1024;
  char name[ MaxName + 1 ];
  int nameLength = 0;
  int n;

  // Send prompt
  const char * prompt = "\nType your name:";
  write( fd, prompt, strlen( prompt ) );

  // Currently character read
  unsigned char newChar;

  // Last character read
  unsigned char lastChar = 0;

  //
  // The client should send <name><cr><lf>
  // Read the name of the client character by character until a
  // <CR><LF> is found.
  //
    
  while ( nameLength < MaxName &&
	  ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

    if ( lastChar == '\015' && newChar == '\012' ) {
      // Discard previous <CR> from name
      nameLength--;
      break;
    }

    name[ nameLength ] = newChar;
    nameLength++;

    lastChar = newChar;
  }

  // Add null character at the end of the string
  name[ nameLength ] = 0;

  printf( "name=%s\n", name );

  // Get time of day
  time_t now;
  time(&now);
  char	*timeString = ctime(&now);

  // Send name and greetings
  const char * hi = "\nHi ";
  const char * timeIs = " the time is:\n";
  write( fd, hi, strlen( hi ) );
  write( fd, name, strlen( name ) );
  write( fd, timeIs, strlen( timeIs ) );
  
  // Send the time of day 
  write(fd, timeString, strlen(timeString));

  // Send last newline
  const char * newline="\n";
  write(fd, newline, strlen(newline));
  
}*/
