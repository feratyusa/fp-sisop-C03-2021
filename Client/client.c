#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h> 
#include <netdb.h> 
#include <string.h>
#include <ctype.h>

// Client olny definition
#define BUFFER_SIZE 1024
#define MAX 50000

// Server-Client Definition (harus ada pada source code server dan client)
#define USER_LOGIN "__login"
#define ROOT "__root"
#define LOGIN_SUCCESS "SUCCESS"
#define LOGIN_FAILED "FAILED"

// Client Variable
struct sockaddr_in saddr;
int fd, ret_val;
struct hostent *local_host; /* need netdb.h for this */
char message[BUFFER_SIZE];
char USERNOW[100];


void fill_message(char *mess);
bool check_user(int argc, char *argv[]);
bool is_sudo();


int main(int argc, char *argv[]){

	/* Step1: create a TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    if (fd == -1) {
        fprintf(stderr, "socket failed [%s]\n", hstrerror(errno));
        return -1;
    }
    printf("Created a socket with fd: %d\n", fd);

    /* Let us initialize the server address structure */
    saddr.sin_family = AF_INET;         
    saddr.sin_port = htons(7000);     
    local_host = gethostbyname("127.0.0.1");
    saddr.sin_addr = *((struct in_addr *)local_host->h_addr);

    /* Step2: connect to the TCP server socket */
    ret_val = connect(fd, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (ret_val == -1) {
        fprintf(stderr, "connect failed [%s]\n", hstrerror(errno));
        close(fd);
        return -1;
    }

    // Autentikasi
	fill_message(USER_LOGIN);
	ret_val = send(fd, message, sizeof(message), 0);

	if(check_user(argc, argv)){
		if(strcmp(USERNOW, ROOT)==0)
			printf("Welcome to nothing, super user!\n");
		else
			printf("\nWelcome to nothing, %s!\n", USERNOW);
	}
	else{
		printf("\nUsername and Password are wrong!\n");
		return -1;
	}

	// Masuk ke app
	while (1)
    {
        printf("client: ");
        char query[BUFFER_SIZE];
        scanf("%[^\n]", message);
        ret_val = send(fd, message, sizeof(message), 0);
        ret_val = recv(fd, message, sizeof(message), 0);
        getchar();
        printf("%s\n", message);
    }

    /* Last step: close the socket */
    close(fd);
    return 0;
}

void fill_message(char *mess){
	memset(message, '\0', BUFFER_SIZE);
	sprintf(message, "%s", mess);
}

bool check_user(int argc, char *argv[]){
	if(is_sudo() && argc == 1){
		fill_message(ROOT);
		strcpy(USERNOW, ROOT);
	}
	else if(argc > 1){
		char userPass[1000];
		if(strcmp(argv[1], "-u")==0 && strcmp(argv[3], "-p")==0){
			printf("Checking user\n");
			sprintf(userPass, "%s\t%s\n", argv[2], argv[4]);
			fill_message(userPass);
			strcpy(USERNOW, argv[2]);
		}
		else{
			fill_message("Error");
		}
	}
	else{
		fill_message("Error");
	}
	ret_val = send(fd, message, sizeof(message), 0);

	ret_val = recv(fd, message, sizeof(message), 0);
	if(strcmp(message, LOGIN_SUCCESS)==0) return true;
	else return false;
}

bool is_sudo(){
	if(geteuid()) return false;
	else return true;
}