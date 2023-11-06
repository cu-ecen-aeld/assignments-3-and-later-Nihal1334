#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <errno.h>

#define BACKLOG 5
#define MAX_BUF_LEN 4096 
#define FAILURE -1
#define SUCCESS 0
int sockfd = 0, client_fd = 0, filedesc = 0;
char* filename = "/var/tmp/aesdsocketdata";
volatile sig_atomic_t exit_condition = 0;

void close_connection();

void sig_handle(int sig)
{
    if(sig == SIGINT || sig == SIGTERM)
    {
        syslog(LOG_WARNING, "Caught signal, exiting");
        exit_condition = true;
        close_connection();
    }
}
void close_connection()
{
    close(client_fd);
    /* deletes the file */
    if (FAILURE == unlink(filename))
    {
       syslog(LOG_PERROR, "unlink %s: %s", filename, strerror(errno));
    }
    close(sockfd);
    if (FAILURE == shutdown(sockfd, 2))
    {
        syslog(LOG_PERROR, "shutdown: %s", strerror(errno));
    }
    closelog();
}

int daemonize()
{
    fflush(stdout);
    pid_t pid = fork();
    if (FAILURE == pid)
    {
        syslog(LOG_PERROR, "creating a child process:%s\n", strerror(errno));
        return FAILURE;
    }
    else if (0 == pid)
    {
        printf("Child process created successfully\n");
        umask(0);
        pid_t id = setsid();
        if (FAILURE == id)
        {
            syslog(LOG_PERROR, "setsid:%s\n", strerror(errno));
            return FAILURE;
        }
        /* change current directory to root */
        if (FAILURE == chdir("/"))
        {
            syslog(LOG_PERROR, "chdir:%s\n", strerror(errno));
            return FAILURE;
        }
        /* close standard files of process */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        /* redirect standard files to /dev/null */
        int fd = open("/dev/null", O_RDWR);
        if (FAILURE == fd)
        {
            syslog(LOG_PERROR, "open:%s\n", strerror(errno));
            return FAILURE;
        }
        if (FAILURE == dup2(fd, STDIN_FILENO))
        {
            syslog(LOG_PERROR, "dup2:%s\n", strerror(errno));
            return FAILURE;
        }
        if (FAILURE == dup2(fd, STDOUT_FILENO))
        {
            syslog(LOG_PERROR, "dup2:%s\n", strerror(errno));
            return FAILURE;
        }
        if (FAILURE == dup2(fd, STDERR_FILENO))
        {
            syslog(LOG_PERROR, "dup2:%s\n", strerror(errno));
            return FAILURE;
        }
        close(fd);
    }
    else
    {
        syslog(LOG_PERROR, "Terminating Parent process");
        exit(0);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    struct addrinfo hints;
    struct addrinfo* res;
    char clientIP[INET6_ADDRSTRLEN];
    int opt = 1; bool recv_flag = false;
    bool start_as_daemon = false; 
    int recv_size = 0, written_bytes = 0, send_bytes = 0;
    char buf[MAX_BUF_LEN] = {'\0'};
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr);

    openlog(NULL, 0, LOG_USER);
    if ((2 == argc) && (strcmp(argv[1], "-d") == 0))
    {
        syslog(LOG_INFO, "Starting aesdsocket as a daemon");       
        start_as_daemon = true;
    }

    signal(SIGINT, sig_handle);
    signal(SIGTERM, sig_handle);
    //create a socket and get socket filedesc
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //socket configurations
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    if (SUCCESS != getaddrinfo(NULL, "9000", &hints, &res))
    {
        syslog(LOG_PERROR, "getaddrinfo: %s", strerror(errno));
        if (NULL != res)
        {
            freeaddrinfo(res);
        }
        close(sockfd);
        return FAILURE;
    }

    if (SUCCESS != setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        syslog(LOG_ERR, "Failed to setsockopt");
        if (NULL != res)
        {
            freeaddrinfo(res);
        }
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    //bind addr to sockfd
    if( SUCCESS != bind(sockfd, res->ai_addr, res->ai_addrlen))
    {
        syslog(LOG_ERR, "Failed to bind connection");
        if (NULL != res)
        {
            freeaddrinfo(res);
        }
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (NULL != res)
    {
        freeaddrinfo(res);
    }
    //check if program should be deamonized
    if (start_as_daemon)
    {
        syslog(LOG_DEBUG, "Turning into a daemon");
        if( FAILURE == daemonize())
        {
            close(sockfd);
            return FAILURE;
        }
    }

    //listen to socket fd
    if( SUCCESS != listen(sockfd, BACKLOG))
    {
        syslog(LOG_ERR, "Failed to listen connection");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while( 1 )
    {
        //accept the connection and get client fd
        if(( client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen)) == FAILURE )
        {
            syslog(LOG_ERR, "Failed to accept connection");
            close_connection();
            return FAILURE;
        }
        printf("Connection accepted");
        // Get the client's IP address from the clientAddr structure
        if (NULL == inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, INET_ADDRSTRLEN))
        {
            syslog(LOG_PERROR, "inet_ntop: %s", strerror(errno));
        }
        syslog(LOG_INFO, "Accepted connection from %s",clientIP);

        //open the file with Append or create it if it doesn't exist
        filedesc = open(filename, O_CREAT | O_RDWR | O_APPEND, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
        if( filedesc == FAILURE)
        {
            syslog(LOG_ERR, "Failed to create file");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        do
        {
            memset(buf, 0, MAX_BUF_LEN);
            recv_size = recv(client_fd, buf, MAX_BUF_LEN-1, 0);
            if( FAILURE == recv_size)
            {
                syslog(LOG_PERROR, "recv: %s", strerror(errno));
                close(filedesc);
                close(client_fd);
                close(sockfd);
                return FAILURE;
            }

            //write to file
            written_bytes =  write(filedesc, buf, recv_size);
 	    if (written_bytes != recv_size)
            {
                syslog(LOG_ERR, "Error writing %s to %s file: %s", buf, filename,
                strerror(errno));
                close(filedesc);
                close(client_fd);
                close(sockfd);
                return FAILURE;
            }
           
            if( NULL != memchr(buf, '\n', recv_size))
            {
                recv_flag = true;
                syslog(LOG_DEBUG, "End receive");
            }
        }
        while( !recv_flag );
        recv_flag = false;

        lseek(filedesc, 0, SEEK_SET);
        int byte_read = 0;
      
        do
        {
            memset(buf, 0, MAX_BUF_LEN);
            byte_read = read(filedesc, buf, MAX_BUF_LEN);
            if( byte_read > 0)
            {
                send_bytes =  send(client_fd, buf, byte_read, 0);
                if( send_bytes != byte_read )
                {
                  //  syslog(LOG_PERROR, "send: %s", strerror(errno));
                    close(filedesc);
                    close(client_fd);
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                //syslog(LOG_DEBUG, "Sending %d bytes: %s",byte_read,buf);
            }
        }
        while(byte_read > 0);

        close(filedesc);
        if (SUCCESS == close(client_fd))
        {
            syslog(LOG_INFO, "Closed connection from %s", clientIP);
        }
    }

    close_connection();

    return EXIT_SUCCESS;
}

