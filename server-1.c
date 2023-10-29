#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int player_count = 0;
pthread_mutex_t mutexcount;
int size;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
}

void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
}


void write_clients_msg(int * cli_sockfd, char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

void write_clients_int(int * cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
}

int setup_listener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening listener socket.");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;	
    serv_addr.sin_addr.s_addr = INADDR_ANY;	
    serv_addr.sin_port = htons(portno);		

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR binding listener socket.");


    return sockfd;
}
int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int))  return -1;

    printf(" Received int: %d\n", msg);
    
    return msg;
}

void get_clients(int lis_sockfd, int * cli_sockfd)
{
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
     
    int num_conn = 0;
    while(num_conn < 2)
    {
	    listen(lis_sockfd, 253 - player_count);
        
        memset(&cli_addr, 0, sizeof(cli_addr));

        clilen = sizeof(cli_addr);
	
        cli_sockfd[num_conn] = accept(lis_sockfd, (struct sockaddr *) &cli_addr, &clilen);
            
        if (cli_sockfd[num_conn] < 0)
            error("ERROR accepting a connection from a client.");
        
        write(cli_sockfd[num_conn], &num_conn, sizeof(int));
        
        
        pthread_mutex_lock(&mutexcount);
        player_count++;
        printf("Number of players is now %d.\n", player_count);
        pthread_mutex_unlock(&mutexcount);

        if (num_conn == 0) {
            write_client_msg(cli_sockfd[0],"HLD");
            
        }

        num_conn++;
    }
}

int get_player_move(int cli_sockfd)
{
    
    
    write_client_msg(cli_sockfd, "TRN");

    return recv_int(cli_sockfd);
}

int check_move(char board[size][size], int move, int player_id)
{
    if ((move == size*size) || (board[move/size][move%size] == ' ')) { 
        return 1;
   }
   else {      
       printf("Player %d's move was invalid.\n", player_id);
       return 0;
   }
}

void update_board(char board[size][size], int move, int player_id)
{
    board[move/size][move%size] = player_id ? 'X' : 'O';
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


void send_update(int * cli_sockfd, int move, int player_id)
{   
    write_clients_msg(cli_sockfd, "UPD");

    write_clients_int(cli_sockfd, player_id);
    
    write_clients_int(cli_sockfd, move);
    
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);
}

int check_board(char board[size][size], int last_move)
{
    
    // if you won then return 1;
   
    int row = last_move/size;
    int col = last_move%size;

    bool won = true;
    // row wise check
    char ch = board[row][0];
    for(int j = 1;j<size;j++){
        if(ch!=board[row][j] || board[row][j]==' ') {
            won = false;
            break;
        };
    }
    if(won) return 1;
    won = true;
    // column wise check
    ch = board[0][col];
    for(int j = 1;j<size;j++){
        if(ch!=board[j][col] || board[j][col]==' ') {
            won = false;
            break;
        };
    }
    if(won) return 1;
    // left diagonal check
    if(!(last_move || last_move%(size+1))){
        won = true;
        ch = board[0][0];
        for(int i = 1;i<size;i++){
            if(ch!=board[i][i] || board[i][i]==' ') {
                won = false;
                break;
            };
        }
        if(won) return 1;
    }
    // right diagonal check 
    if(!(last_move%(size-1))){
        won = true;
        ch = board[0][size-1];
        for(int i = 1;i<size;i++){
            if(ch!=board[i][size-1-i] || board[i][size-1-i]==' ') {
                won = false;
                break;
            };
        }
        if(won) return 1;
    }
    
    return 0;
}

void *run_game(void *thread_data) 
{
    int *cli_sockfd = (int*)thread_data; 
    char board[size][size];
    for(int i = 0;i<size;i++){
        for(int j = 0;j<size;j++){
            board[i][j] = ' ';
        }
    }

    printf("Game on!\n");
    
       write_clients_msg(cli_sockfd, "SRT");

    draw_board(board);
    
    int prev_player_turn = 1;
    int player_turn = 0;
    int game_over = 0;
    int turn_count = 0;
    while(!game_over) {
        
        if (prev_player_turn != player_turn)
            write_client_msg(cli_sockfd[(player_turn + 1) % 2], "WAT");

        int valid = 0;
        int move = 0;
        while(!valid) {             move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1) break; 
            printf("Player %d played position %d\n", player_turn, move);
                
            valid = check_move(board, move, player_turn);
            if (!valid) { 
                printf("Move was invalid. Let's try this again...\n");
                write_client_msg(cli_sockfd[player_turn], "INV");
            }
        }

	    if (move == -1) { 
            printf("Player disconnected.\n");
            break;
        }
        else if (move == (size*size)) {
            prev_player_turn = player_turn;
            send_player_count(cli_sockfd[player_turn]);
        }
        else {
                     update_board(board, move, player_turn);
            send_update( cli_sockfd, move, player_turn );
                
         
            draw_board(board);

                        game_over = check_board(board, move);
            
            if (game_over == 1) {
                write_client_msg(cli_sockfd[player_turn], "WIN");
                write_client_msg(cli_sockfd[(player_turn + 1) % 2], "LSE");
                printf("Player %d won.\n", player_turn);
            }
            else if (turn_count == size*size-1) {                printf("Draw.\n");
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
            }

            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
        }
    }

    printf("Game over.\n");

	close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    printf("Number of players is now %d.", player_count);
    player_count--;
    printf("Number of players is now %d.", player_count);
    pthread_mutex_unlock(&mutexcount);
    
    free(cli_sockfd);

    pthread_exit(NULL);
}
void instructions(){
    int index = 0;
    for(int i = 0;i<size;i++){
        for(int j = 0;j<size;j++){
            if(j==0) printf(" %2d ",index);
            else printf("| %2d ",index);
            index++;
        }
        printf("\n");
        if(i!=size-1){
            for(int j = 0;j<size;j++){
                if(j==0) printf("----");
                else printf("+----");
            }   
        printf("\n");
        }
    }
}

int main(int argc, char *argv[])
{   
    if (argc < 3) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    size = 3;
    int n = atoi(argv[2]);
    size = n;

    int lis_sockfd = setup_listener(atoi(argv[1])); 
    pthread_mutex_init(&mutexcount, NULL);
    while (1) {
        if (player_count <= 252) {   
            int *cli_sockfd = (int*)malloc(2*sizeof(int)); 
            memset(cli_sockfd, 0, 2*sizeof(int));
            
            get_clients(lis_sockfd, cli_sockfd);
            

            pthread_t thread;
            int result = pthread_create(&thread, NULL, run_game, (void *)cli_sockfd);
            if (result){
                printf("Thread creation failed with return code %d\n", result);
                exit(-1);
            }
            
            printf(" New game thread started.\n");
            }
    }

    close(lis_sockfd);

    pthread_mutex_destroy(&mutexcount);
pthread_exit(NULL);
}
