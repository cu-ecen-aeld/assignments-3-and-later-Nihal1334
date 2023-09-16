#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

void log_error( char* filename );

int main(int argc, char **argv)
{
    openlog(NULL, 0, LOG_USER);
    if( argc < 3 )
    {
	
	syslog(LOG_ERR, "Incorrect Number of arguments:%d", argc);
    	exit(1);
    }

    char* filename = argv[1];
    char* str_name = argv[2];

    int fd = 0;
    fd = creat(filename, 0664);

    if( fd == -1 )
    {
	log_error(filename);
	exit(1);
    }

    
    syslog( LOG_DEBUG, "Writing: %s to %s",argv[2], argv[1]);
    
    int size = write( fd, str_name, strlen(str_name)+1);

    if( size == -1 )
    {
	log_error(filename);
	exit(1);
    }
    
    if( close(fd) == -1 )
    {
        log_error(filename);
	exit(1);
    }
    
    return 0;
}

void log_error( char* filename )
{
	syslog( LOG_ERR, "Error in opening file %s:%s\n", filename, strerror(errno));
	closelog();
}
