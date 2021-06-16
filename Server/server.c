#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <wait.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h> 
#include <stdbool.h>
#include <limits.h>

#define DATA_BUFFER 1000
#define MAX 50000
#define MAX_CONNECTIONS 10 

// Server Variables
fd_set read_fd_set;
struct sockaddr_in new_addr;
int server_fd, new_fd, ret_val, i, fd_now;
socklen_t addrlen;
char buf[DATA_BUFFER];
int all_connections[MAX_CONNECTIONS];


void __init__daemon();
int create_tcp_server_socket();


int main(){
	__init__daemon();

	/* Get the socket server fd */
    server_fd = create_tcp_server_socket(); 
    if (server_fd == -1) {
        fprintf(stderr, "Failed to create a server\n");
        return -1; 
    }   

    /* Initialize all_connections and set the first entry to server fd */
    for (i=0;i < MAX_CONNECTIONS;i++) {
        all_connections[i] = -1;
    }
    all_connections[0] = server_fd;

        while (1) {
        FD_ZERO(&read_fd_set);
        /* Set the fd_set before passing it to the select call */
        for (i=0;i < MAX_CONNECTIONS;i++) {
            if (all_connections[i] >= 0) {
                FD_SET(all_connections[i], &read_fd_set);
            }
        }

        /* Invoke select() and then wait! */
        ret_val = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

        /* select() woke up. Identify the fd that has events */
        if (ret_val >= 0 ) {
            /* Check if the fd with event is the server fd */
            if (FD_ISSET(server_fd, &read_fd_set)) { 
                // printf("Returned fd is %d (server's fd)\n", server_fd);
                /* accept the new connection */
                new_fd = accept(server_fd, (struct sockaddr*)&new_addr, &addrlen);
                if (new_fd >= 0) {
                    for (i=0;i < MAX_CONNECTIONS;i++) {
                        if (all_connections[i] < 0) {
                            all_connections[i] = new_fd;
                        }
                    }
                } else {
                    fprintf(stderr, "accept failed [%s]\n", strerror(errno));
                }
                ret_val--;
                if (!ret_val) continue;
            } 

            /* Check if the fd with event is a non-server fd */
            for (i=1;i < MAX_CONNECTIONS;i++) {
                if ((all_connections[i] > 0) &&
                    (FD_ISSET(all_connections[i], &read_fd_set))) {

                    /* read incoming data */
                    memset(buf, '\0', DATA_BUFFER);
                    ret_val =  recv(all_connections[i], buf, DATA_BUFFER, 0);

                    /* Close connection with client */
                    if (ret_val == 0) {
                        close(all_connections[i]);
                        all_connections[i] = -1; /* Connection is now closed */
                    } 

                    /* Receive from client */
                    if (ret_val > 0) {
                    	char mess[DATA_BUFFER/2];
                    	strcpy(mess, buf);
                    	sprintf(buf, "from server: %s", mess);
                    	ret_val = send(all_connections[i], buf, sizeof(buf), 0);
                    } 

                    /*  Receive failed from client */
                    if (ret_val == -1) {
                        // Error message
                        break;
                    }
                }
                ret_val--;
                if (!ret_val) continue;
            } /* for-loop */
        } /* (ret_val >= 0) */
    } /* while(1) */

    /* Last step: Close all the sockets */
    for (i=0;i < MAX_CONNECTIONS;i++) {
        if (all_connections[i] > 0) {
            close(all_connections[i]);
        }
    }
    return 0;
}

void __init__daemon(){
	pid_t pid, sid;
	pid = fork();

	if(pid < 0){
		exit(EXIT_FAILURE);
	}

	if(pid > 0){
		exit(EXIT_SUCCESS);
	}

	umask(0);

	sid = setsid();
	if(sid < 0){
		exit(EXIT_FAILURE);
	}
	if((chdir("/")) < 0){
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}


int create_tcp_server_socket() {
    struct sockaddr_in saddr;
    int fd, ret_val;

    /* Step1: create a TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    if (fd == -1) {
        fprintf(stderr, "socket failed [%s]\n", strerror(errno));
        return -1;
    }
    printf("Created a socket with fd: %d\n", fd);

    /* Initialize the socket address structure */
    saddr.sin_family = AF_INET;         
    saddr.sin_port = htons(7000);     
    saddr.sin_addr.s_addr = INADDR_ANY; 

    /* Step2: bind the socket to port 7000 on the local host */
    ret_val = bind(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (ret_val != 0) {
        fprintf(stderr, "bind failed [%s]\n", strerror(errno));
        close(fd);
        return -1;
    }

    /* Step3: listen for incoming connections */
    ret_val = listen(fd, 5);
    if (ret_val != 0) {
        fprintf(stderr, "listen failed [%s]\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}
