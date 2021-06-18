#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
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

// Server-only Definition
#define DATA_BUFFER 1024
#define MAX 50000
#define MAX_CONNECTIONS 10 

// Server-Client Definition (harus ada pada source code server dan client)
#define USER_LOGIN "__login"
#define ROOT "__root"
#define LOGIN_SUCCESS "SUCCESS"
#define LOGIN_FAILED "FAILED"

// Server Variables
fd_set read_fd_set;
struct sockaddr_in new_addr;
int server_fd, new_fd, ret_val, i, fd_now;
socklen_t addrlen;
char buf[DATA_BUFFER];
int all_connections[MAX_CONNECTIONS];

/* Ubah server database ke folder server.c disimpan */
char SERVER_DATABASE[] = "/home/prabu/Desktop/sisop/fp/Server";
char DATABASES_FOLDER[] = "databases";
char USER_DATABASE_FOLDER[] = "databases/rootUSERDATABASE";
char USER_TABLE[] = "databases/rootUSERDATABASE/USER.csv";


char userNow[MAX_CONNECTIONS][100];
char usingDatabase[MAX_CONNECTIONS][100];


void __init__server();
void __init__daemon();
int create_tcp_server_socket();
void fill_buf(char message[]);

bool is_root(char user[]);
bool authenticate_user(char user[]);
bool is_database_exist(char database[]);
bool is_user_exist(char user[]);
bool check_permission(char database[], char user[]);
void add_new_user(char user[], char pass[]);
void grant_permission(char database[], char user[]);


int main(){
	/* Init server dulu baru daemon */
	__init__server();
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
                            break;
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
                    	strcpy(userNow[i], "");
                    	strcpy(usingDatabase[i], "");
                        close(all_connections[i]);
                        all_connections[i] = -1; /* Connection is now closed */

                        // printf("Byebye\n");
                    } 

                    /* Receive from client */
                    if (ret_val > 0) {
                    	
                    	// Autentikasi User
                    	printf("%s\n",buf);
                    	if(strcmp(buf, USER_LOGIN)==0){
                    		memset(buf, '\0', DATA_BUFFER);
							ret_val =  recv(all_connections[i], buf, DATA_BUFFER, 0);
							
							char user[DATA_BUFFER];
							strcpy(user, buf);
							
                    		if(is_root(user)){
                    			strcpy(userNow[i], ROOT);
                    			printf("%s\n", user);
                    			fill_buf(LOGIN_SUCCESS);
                    		}
                    		else{
                    			if(authenticate_user(user)){
                    				strcpy(userNow[i], user);
                    				fill_buf(LOGIN_SUCCESS);
                    			}
                    			else{
                    				fill_buf(LOGIN_FAILED);
                    			}
                    		}
                    		ret_val = send(all_connections[i], buf, sizeof(buf), 0);
                    	}

                    	// App
                    	else if(strcmp(buf, USER_LOGIN)!=0){

                    		printf("App\n");

                    		// Parse Query
                    		char query[100][1024], *token, temp[DATA_BUFFER];
                    		strcpy(temp, buf);
                    		int iterator = 0;
                    		token = strtok(temp, " ");
                    		while(token != NULL){
								strcpy(query[iterator++], token);
                    			token = strtok(NULL, " ");
                    		}

                    		/**

                    			Super User Only Privilege

                    		**/

                    		/* Create user */
                    		if(strcmp(query[0], "CREATE")==0 && strcmp(query[1], "USER")==0
                    			&& is_root(userNow[i]) ){
                    			char new_user[100], new_pass[100];
                    			strcpy(new_user, query[2]);
                    			if(strcmp(query[3], "IDENTIFIED")==0 && strcmp(query[4], "BY")==0){
                    				strcpy(new_pass, query[5]);
                    				if(is_user_exist(new_user)==false){
                    					add_new_user(new_user, new_pass);
                    					fill_buf("New User added\n");
                    				}
                    				else{
                    					fill_buf("User already exist!\n");
                    				}
                    			}
                    			else{
                    				fill_buf("Error, create user failed\n");
                    			}
                    		}

                    		/* Grant permission */
                    		else if(strcmp(query[0], "GRANT")==0 && strcmp(query[1], "PERMISSION")==0
                    				&& is_root(userNow[i])){
                    			char database[100], user[100];
                    			strcpy(database, query[2]);
                    			if(strcmp(query[3], "INTO")==0){
                    				strcpy(user, query[4]);
                    				if(!is_user_exist(user) || !is_database_exist(database)){
                    					fill_buf("Database or User doesn't exist\n");
                    				}
                    				else if(check_permission(database, user)==false){
                    					grant_permission(database, user);
                    					fill_buf("Grant permission success\n");
                    				}
                    				else{
                    					fill_buf("Permission already granted!\n");
                    				}
                    			}
                    			else{
                    				fill_buf("Error, grant permission failed\n");
                    			}
                    		}

                    		/**

								User and Super User Priviliege

                    		**/
                    		else if(strcmp(query[0], "USE")==0 && strcmp(query[1], "DATABASE")==0){
                    			char database[100];
                    			strcpy(database, query[2]);
                    			if(is_database_exist(database)){
                    				if(is_root(userNow[i])){
                    					strcpy(usingDatabase[i], database);
                    					char mess[DATA_BUFFER];
                    					sprintf(mess, "Using %s\n", database);
                    					fill_buf(mess);
                    				}
                    				else if(check_permission(database,userNow[i])){
                    					strcpy(usingDatabase[i], database);
                    					char mess[DATA_BUFFER];
                    					sprintf(mess, "Using %s\n", database);
                    					fill_buf(mess);
                    				}
                    				else{
                    					fill_buf("User is forbidden from that database\n");
                    				}                    					
                    				
                    			}
                    			else{
                    				fill_buf("Database doesn't exist\n");
                    			}
                    		}

                    		ret_val = send(all_connections[i], buf, sizeof(buf), 0);
                    	}
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
	if((chdir(SERVER_DATABASE)) < 0){
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

void __init__server(){
	mkdir(DATABASES_FOLDER, 0777);
	mkdir(USER_DATABASE_FOLDER, 0777);
	FILE *fp = fopen(USER_TABLE, "a+");
	fclose(fp);
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

void fill_buf(char *message){
	memset(buf, '\0', DATA_BUFFER);
	sprintf(buf, "%s", message);
}

/* Mengecek apakah user adalah root */
bool is_root(char user[]){
	if(strcmp(user, ROOT)==0) return true;
	return false;
}

/* Mengecek user untuk keperluan login */
bool authenticate_user(char user[]){
	char data[DATA_BUFFER], akun[DATA_BUFFER];
	bool flag = false;

	strcpy(data, user);
	FILE *fp = fopen(USER_TABLE, "r");
	while(fgets(akun, sizeof(akun), fp)){
		if(strcmp(data, akun)==0){
			flag = true;
			break;
		}
	}
	fclose(fp);

	if(flag){
		char *token;
		token = strtok(data, "\t");
		strcpy(user, token);
		return true;
	}
	return false;
}

/* Mengecek apakah user sudah ada atau belum */
bool is_user_exist(char user[]){
	char data[100], *token;
	bool flag = false;
	FILE *fp = fopen(USER_TABLE, "r");
	while(fgets(data, sizeof(data), fp)){
		token = strtok(data, "\t");
		if(strcmp(token, user)==0) flag = true;
	}
	fclose(fp);
	return flag;
}

/* Mengecek apakah dataabase ada */
bool is_database_exist(char database[]){
	char check_database[1024];
	sprintf(check_database, "%s/%s", DATABASES_FOLDER, database);
	DIR *dir = opendir(check_database);
	if(dir) return true;
	else return false;
}

/* Tambah user baru dan menambahkan tabel user */
void add_new_user(char user[], char pass[]){
	FILE *fp = fopen(USER_TABLE, "a");
	fprintf(fp, "%s\t%s\n", user, pass);
	fclose(fp);

	char new_user_table[1024];
	sprintf(new_user_table, "%s/%s.csv", USER_DATABASE_FOLDER, user);
	FILE *fp1 = fopen(new_user_table, "a+");
	fclose(fp1);
}

/* Check permission user terhadap suatu database */
bool check_permission(char database[], char user[]){
	char user_table[1024], data[1024], *token;
	bool flag = false;
	sprintf(user_table, "%s/%s.csv", USER_DATABASE_FOLDER, user);
	FILE *fp = fopen(user_table, "r");
	while(fgets(data, sizeof(data), fp)){
		token = strtok(data, "\n");
		if(strcmp(token, database)==0){
			flag = true;
			break;
		}
	}
	fclose(fp);
	return flag;
}

/* Grant permission user terhadap suatu database */
void grant_permission(char database[], char user[]){
	char user_table[1024];
	sprintf(user_table, "%s/%s.csv", USER_DATABASE_FOLDER, user);
	FILE *fp = fopen(user_table, "a+");
	fprintf(fp, "%s\n", database);
	fclose(fp);
}

