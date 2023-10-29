#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int size;
void error(const char *msg)
{
    perror(msg);
    printf("Either the server shut down or the other player disconnected.\nGame over.\n");
    
    exit(0);
}

void recv_msg(int sockfd, char * msg)
{
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3)
     error("ERROR reading message from server socket.");

    printf(" Received message: %s\n", msg);
   }

int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR");
    
    printf("Received int: %d\n", msg);
    
    return msg;
}

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
    
    printf(" Wrote int to server: %d\n", msg);
    }

int connect_to_server(char * hostname, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sockfd < 0) 
        error("ERROR opening socket for server.");
	
    server = gethostbyname(hostname);
	
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
	
	memset(&serv_addr, 0, sizeof(serv_addr));

   serv_addr.sin_family = AF_INET;
    memmove(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

   if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting to server");

    printf("Connected to server.\n");
     return sockfd;
}

void draw_board(char board[size][size])
{
    printf("\n");
    for(int i = 0;i<size;i++){
        for(int j = 0;j<size;j++){
            if(j==0) printf("    %c ",board[i][j]);
            else printf("| %c ",board[i][j]);
        }
        printf("\n");
        if(i!=size-1){
            for(int j = 0;j<size;j++){
            if(j==0) printf("   ---");
            else printf("+---");
            }
            printf("\n");
        }
    }
    printf("\n");
}


void take_turn(int sockfd)
{
    char buffer[10];
    char input[3];
    while (1) { 
        printf("Enter 0 TO %d  make a move, or %d for number of active players: ",(size*size-1),(size*size));
	    fgets(buffer, 10, stdin);
        input[0] = buffer[0];
        input[1] = buffer[1];
        input[2] = '\0';
	    int move = atoi(input);
        int valid_move = size*size;
        if (move <= valid_move && move >= 0){
            printf("\n");
             write_server_int(sockfd, move);   
            break;
        } 
        else
            printf("\nInvalid input. Try again.\n");
    }
}

void get_update(int sockfd, char board[size][size])
{
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);
    board[move/size][move%size] = player_id ? 'X' : 'O';    
}
void instructions(){
    int index = 0;
    printf("\n");
    for(int i = 0;i<size;i++){
        for(int j = 0;j<size;j++){
            if(j==0) printf("    %2d ",index);
            else printf("| %2d ",index);
            index++;
        }
        printf("\n");
        if(i!=size-1){
            for(int j = 0;j<size;j++){
                if(j==0) printf("   ----");
                else printf("+----");
            }   
        printf("\n");
        }
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    size = 3;
    int n = atoi(argv[3]);
    size = n;

    int sockfd = connect_to_server(argv[1], atoi(argv[2]));
    int id = recv_int(sockfd);


    char msg[4];
    char board[size][size];
    for(int i = 0;i<size;i++){
        for(int j = 0;j<size;j++){
            board[i][j] = ' ';
        }
    }
    do {
        recv_msg(sockfd, msg);
        if (!strcmp(msg, "HLD"))
            printf("Waiting for a second player...\n");
    } while ( strcmp(msg, "SRT") );
    /* The game has begun. */
    printf("Game on!\n");
    printf("Your are %c's\n", id ? 'X' : 'O');
    instructions();
    while(1) {
        recv_msg(sockfd, msg);

        if (!strcmp(msg, "TRN")) { 
	        printf("Your move...\n");
	        take_turn(sockfd);
        }
        else if (!strcmp(msg, "INV")) { 
            printf("That position has already been played. Try again.\n"); 
        }
        else if (!strcmp(msg, "CNT")) { 
            int num_players = recv_int(sockfd);
            printf("There are currently %d active players.\n", num_players); 
        }
        else if (!strcmp(msg, "UPD")) { 
            get_update(sockfd, board);
            draw_board(board);
        }
        else if (!strcmp(msg, "WAT")) { 
            printf("Waiting for other players move...\n");
        }
        else if (!strcmp(msg, "WIN")) { 
            printf("You won....Congratulations!\n");
            break;
        }
        else if (!strcmp(msg, "LSE")) { 
            printf("You lost...but well played\n");
            break;
        }
        else if (!strcmp(msg, "DRW")) { 
            printf("Draw....that was a great game \n");
            break;
        }
        else 
            error("Unknown message.");
    }
    
    printf("Game over.\n");
    close(sockfd);
    return 0;
}
