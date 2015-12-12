//Basic functionality was created by following Beej's Guide to Network Programming

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>


#include "utils.h"

#define BACKLOG 10  
#define PAGE 1048576
#define intList ((int*)list) 
#define pg (PAGE/sizeof(int)-1) 

 
//Gonna use mapping after a stackoverflow discussion on malloc and fork()
static account* list;


int findAccount(char* accountName, account* list, int numAccounts){
    int x = 0;
    while(x<numAccounts){
        if(strcmp(accountName, list[x].acctName) ==0){
            return x;
        }
        x++;
    }
    return -1;
}



void printBankInfo(){
    char* nm = "Account Name:   \n";
    char* bal = "The current balance is: \n";
    while(1){
        printf("*****************************************************************\n");
        printf("CURRENT BANK STATE\n");
        int x=0;
        while(x < intList[pg]){
            
            if(list[x].isf == 1){ //in finder
                printf(list[x].acctName);
                printf("   IS CURRENTLY IN SESSION\n");
                printf("------------------\n");
                continue;     
            }
            printf("%s\t\t%.2f\n", list[x].acctName, list[x].balance);
            x++;
        }
        printf("****************************************************\n");
        sleep(20);
    }
}


char* spaceCheck(char* input){
    int space = -1;
    int x= 0;
    while(x<strlen(input)){
		if(input[x] == ' ') {
			if(space == -1) {
				space = x;
			} else {
				printf("Commands are at most two words\n");
				return NULL;
			}
		}
        x++;
	}

	// one word
	if(space == -1) {
		return NULL;
	}

	// two words
	else {
		input[space] = '\0';
		return &(input[space + 1]);
	}        
}



void sigchld_handler(int s)
{
    
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



//Methods for commands
//kinda got tired of doing this...realized it was easier to do it within main....however it's easier to read modularized

char* openCmd(char *in, char* out, char* arg, int finder){
    
                    arg = spaceCheck(in);
                    if(finder != -1) {
                        strcpy(out, "You cannot open an account with an session started.");
                    } else if((intList[pg]) == 20) {
                    	strcpy(out, "There cannot be more than 20 accounts.");
                	} else {
                        // look for the account with that name via linear search
                        finder = findAccount(arg, list, ((intList)[pg]));
                        // if the account was found...
                        if(finder != -1) {
                            strcpy(out, "That account already exists");
                        } else {
                            list[((intList)[pg])].isf = 0;
                            strcpy(list[((intList)[pg])].acctName, arg);
                            list[((intList)[pg])].balance = 0;
                            ((intList)[pg])++;
                            printf("Created account with name '%s'.\n", arg);
                            strcpy(out, "Success.");
                        }
                    }
    return out;
}

char* startCmd(char *in, char* out, char* arg, int finder){
    
    return out;
}

char* creditCmd(char *in, char* out, char* arg, int finder){
    
    arg = spaceCheck(in);
                    if(finder == -1) {
                        strcpy(out, "You must start a session to use credit.");
                    } else {
                        list[finder].balance += atof(arg);
                        printf("Credited '%f' to current account.\n", atof(arg));
                        strcpy(out, "Success.");
                    }
    
    return out;
}

char* debitCmd(char *in, char* out, char* arg, int finder){
    
    arg = spaceCheck(in);
                    if(finder == -1) {
                        strcpy(out, "You must start a session to use debit.");
                    } else {
                        if ((list[finder].balance - atof(arg)) >= 0 ) {
                        list[finder].balance -= atof(arg);
                        printf("Debited '%f' from current account.\n", atof(arg));
                        strcpy(out, "Success.");
                        }
                        
                        else if ((list[finder].balance - atof(arg)) < 0 ) {
                            printf("Insufficient balance. Unable to debit");
                            strcpy(out, "Insufficient balance. Unable to debit");
                        }
                    }
    
    return out;
}

char* balanceCmd(char* in, char* out, char* arg, int finder){
    if(finder == -1) {
                        strcpy(out, "You must start a session to get your balance.");
                    } 
                    else {
                        char balancestr[50];
                        sprintf(balancestr, "%f", list[finder].balance);
                        strcpy(out, "Your balance is ");
                        strcpy(out + strlen("Your balance is "), balancestr);
                    }
    return out;
}

char* finishCmd(char* in, char* out, char* arg, int finder){
    

    return out;
}


int main(void)
{
    int sockfd, new_fd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; 
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int *sockPtr;
    sockPtr = (int*)malloc(sizeof(int));
    *sockPtr = sockfd; //Point to file desc
    
    

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); 

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    //mmap info learned from stack overflow and random tutorials on the internet and linux man page
    list = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_SHARED| MAP_ANON, -1, 0);
    intList[pg] = 0;
    
    
    //Sigaction timer not working......try using threads
    //struct sigaction sap;

	/* Install timer_handler as the signal handler for SIGVTALRM. */
	//sigemptyset(&sap.sa_mask);
	//sap.sa_flags = 0;
	//sap.sa_handler = printBankInfo;
	//sigaction (SIGALRM, &sap, NULL);
    
    pthread_attr_t attr;
    pthread_t printer;
	
    if(pthread_attr_init(&attr) != 0){
        printf("ERROR: attr_init failure\n");
        exit(EXIT_FAILURE);
    }
    
    if(pthread_create(&printer, &attr, printBankInfo, sockPtr) != 0){
        printf("ERROR: creating thread\n");
        exit(EXIT_FAILURE);
    }
    
    
    
    

    printf("server: waiting for connections...\n");

    while(1) {  
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { 
            close(sockfd); 
            
            
            char in[64];
            char out[64];
            char out2[64];
            int numB;
            char *arg;
            int commandArg;
            int finder = -1;
            
                
               while(1) {
				for(int i = 0; i < 64; i++){
					in[i] = 0;
				}
                for(int x=0; x<64; x++){
                    out2[x] = 0;
                }
				// recieve in
				if((numB = recv(new_fd, in, 64-1, 0)) == -1) {
					perror("recv");
					exit(1);
				}

				if(numB == 0) {
					if(finder != -1) {
						list[finder].isf = 0;
                        printf("Closed session.\n");
					}
					printf("Connection closed by client.\n");
					exit(1);
				}
                
                if (strncmp(in, "open ", 5) == 0) {
                    strcpy(out2, openCmd(in, out, arg, finder));
                }
                
                else if (strncmp(in, "start ", 6) == 0) {
                    arg = spaceCheck(in);
                    // find the account with that name via linear search
                    if(finder == -1) {
                        finder = findAccount(arg, list, (((int*)list)[pg]));
                        // if the account was not found
                        if(finder == -1) {
                            strcpy(out, "There are no accounts with that name");
                        }
                        else if(list[finder].isf != 1) {
                            list[finder].isf = 1;
                            printf("Started session for account '%s'.\n", arg);
                            strcpy(out, "Success.");
                        } 
                        else {
                            strcpy(out, "That account is currently being accessed.");
                        }
                    }
                    else if(finder != -1) {
                        strcpy(out, "You already have a session active.");
                    }
                }
                
                else if (strncmp(in, "credit ", 7) == 0) {
                    strcpy(out2, creditCmd(in, out, arg, finder));
                }
                
                else if (strncmp(in, "debit ", 6) == 0) {
                    strcpy(out2, debitCmd(in, out, arg, finder));
                }
                
                else if (strncmp(in, "balance", 7) == 0) {
                    strcpy(out2, balanceCmd(in, out, arg, finder));
                }
                                   
                else if (strncmp(in, "finish", 6) == 0) {
                    if(finder == -1) {
                        strcpy(out, "You are not in a session.");
                    } else {
                        list[finder].isf = 0;
                        finder = -1;
                        printf("Session closed.\n");
                        strcpy(out, "Success.");
                    }
                }
                
                else if (strncmp(in, "exit", 4) == 0) {
                	printf("Exiting process.\n");
                    exit(0);
                }

                else {
                    strcpy(out, "Invalid command.");
                }

            	// send output
            	if(send(new_fd, out, strlen(out), 0) == -1) {
              		perror("send");
              	}
			}
			close(new_fd);
			printf("Exiting process.\n");
			exit(0);
		}
		// parent doesn't need this
		close(new_fd);
		int status;
	}
	
	pthread_join(printer, NULL);

	return 0;
}

                        
                        
                        
    

                        