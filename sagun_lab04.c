#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


struct thread_data
{
    int **matrix;
    int *vect;
    // double **matrix;
    // double *vect;
    int x;
    int y;
    double *results;
    int tnum;
    int starting_index;
};

struct message_data
{
    int **matrix;
    int rows;
    int cols;
};

// Function to divide the matrix into t submatrices
int ***divide_matrix(int **X, int m, int n, int t)
{
    printf("dividing matrix...\n");
    int remainingRows = m % t;

    // Allocate memory for the array of matrices
    int ***submatrices = (int ***)malloc(t * sizeof(int **));
    for (int i = 0; i < t; i++)
    {
        submatrices[i] = (int **)malloc(m * sizeof(int *));
        // for(int j = 0; j < m; j++){
        //     submatrices[i][j] = (int*)malloc(n * sizeof(int));
        // }
        if(submatrices[i] == NULL){
            printf("Memory allocation failure in submatrices\n");
        }
    }

    int start = 0;
    int jmp = n / t;
    // for the case where the number of rows is divisible by the number of threads
    if (remainingRows == 0)
    {
        int stop = n / t;
        // printf("start = %d, stop = %d, jmp = %d\n", start, stop, jmp);
        while (stop <= n)
        {
            // printf("start = %d, stop = %d, jmp = %d\n", start, stop, jmp);
            for (int i = 0; i < t; i++)
            {
                // submatrices[i]
                for (int j = 0; j < m; j++)
                { // row of the submatrix
                    // printf("start = %d, stop = %d, jmp = %d\n", start, stop, jmp);
                    submatrices[i][j] = (int *)malloc((stop-start) * sizeof(int));
                    if(submatrices[i][j] == NULL){
                        printf("Memory allocation failure in submatrices' rows (submatrix %d, row %d)\n", i, j);
                    }
                    int colIndex = 0; //this is to separate the submatrix column index from X
                    for (int k = start; k < stop; k++)
                    { // column of the submatrix
                        // submatrices[i][j][colIndex] = (int *)malloc(sizeof(int));
                        submatrices[i][j][colIndex] = X[j][k];
                        colIndex++;
                    }
                }
                // prints the submatrix
                // printf("Submatrix %d:\n", i + 1);
                // for (int j = 0; j < m; j++)
                // {
                //     for (int k = 0; k < jmp; k++)
                //     {
                //         printf("%d ", submatrices[i][j][k]);
                //     }
                //     printf("\n");
                // }
                start = stop;
                stop = stop + jmp;
            }
        }
    }else{
        // int stop = n / t;
        for (int i = 0; i < t; i++)
        {
            // submatrices[i]
            int stop = start + jmp + (i<remainingRows?1:0); //recalculate stop here everytime
            // printf("start = %d, stop = %d, jmp = %d\n", start, stop, jmp);
            for (int j = 0; j < m; j++)
            { // row of the submatrix
                submatrices[i][j] = (int *)malloc((stop-start) * sizeof(int));
                int colIndex = 0; //this is to separate the submatrix column index from X
                for (int k = start; k < stop; k++)
                { // column of the submatrix
                    // submatrices[i][j][colIndex] = (int *)malloc(sizeof(int));
                    submatrices[i][j][colIndex] = X[j][k];
                    colIndex++;
                }
            }
            // prints the submatrix
            // printf("Submatrix %d:\n", i + 1);
            // for (int j = 0; j < m; j++)
            // {
            //     for (int k = 0; k < (stop-start); k++)
            //     {
            //         printf("%d ", submatrices[i][j][k]);
            //     }
            //     printf("\n");
            // }
            start = stop;
        }
    }
    return submatrices;
}

int main(){
    //read inputs from users
    int n, p, s;

    printf("Enter status of this instance (0 = master, 1 = slave): ");
    scanf("%d", &s);

    //check if status is master or slave
    if (s == 0)
    {
        //master - client
        printf("Enter the size of the matrix: ");
        scanf("%d", &n);
        // printf("Enter the port number: ");
        // scanf("%d", &p);
        //make matrix
        int **X = (int **)malloc(n * sizeof(int *));
        for (int i = 0; i < n; i++)
        {
            X[i] = (int *)malloc(n * sizeof(int));
            if (X[i] == NULL)
            {
                printf("Memory allocation failed\n");
                return 1; //exits program
            }
        }

        // Make random matrix
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                X[i][j] = rand() % 100 + 1;
                // printf("%d ", X[i][j]);
            }
            // printf("\n");
        }
        ////////////////////////////
        // FOR TESTING IN SAME PC //
        ////////////////////////////

        //check config file for number of slaves
        FILE *config = fopen("sagun_config.txt", "r");
        //read how many slaves are there
        int numSlaves;
        //store the first line as the number of slaves
        fscanf(config, "%d", &numSlaves);
        printf("Number of slaves: %d\n", numSlaves);
        //set the first address as the master always
        char *addr_master = (char *)malloc(16 * sizeof(char));
        int port_master;
        fscanf(config, "%s %d", addr_master, &port_master);
        printf("Master: %s\n", addr_master);
        //read the ip addr and port number for each slave
        char **addr_slaves = (char **)malloc(numSlaves * sizeof(char*));
        for(int i=0;i<numSlaves;i++)
        {
            addr_slaves[i] = (char *)malloc(16 * sizeof(char));
        }
        int *ports_slaves = (int *)malloc(numSlaves * sizeof(int));
        for (int i = 0; i < numSlaves; i++)
        {
            fscanf(config, "%s %d",addr_slaves[i], &ports_slaves[i]);
        }
        printf("address and port of slaves:\n");
        for (int i = 0; i < numSlaves; i++)
        {
            printf("Slave %d: %s %d\n",i+1,addr_slaves[i],ports_slaves[i]);
        }

        //divide the submatrix into number of slaves
        int ***submatrices = divide_matrix(X, n, n, numSlaves);
        printf("Matrix divided\n");
        
        srand(time(NULL));
        clock_t start = clock();
        //open port p, distribute the submatrices to slaves
        int socket_desc;
        struct sockaddr_in server_addr;
        char server_message[2000], client_message[2000];

        //make a socket and send message per slave
        for(int i = 0; i < numSlaves;i++)
        {    
            // Clean buffers:
            memset(server_message,'\0',sizeof(server_message));
            memset(client_message,'\0',sizeof(client_message));
            
            // Create socket:
            socket_desc = socket(AF_INET, SOCK_STREAM, 0);
            
            if(socket_desc < 0){
                printf("Unable to create socket\n");
                return -1;
            }
            
            printf("Socket created successfully\n");
            // Set port and IP the same as server-side:
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(ports_slaves[i]);
            server_addr.sin_addr.s_addr = inet_addr(addr_slaves[i]);
            
            // Send a connection request to the server, which is waiting at accept():
            if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
                printf("Unable to connect\n");
                return -1;
            }
            printf("Connected with server successfully\n");
            
            // Send the message to server:
            struct message_data *submatrix = (struct message_data *)malloc(sizeof(struct message_data));
            submatrix->matrix = submatrices[i];
            // for(int j = 0;j<n;j++){
            //     for(int k=0;k<numSlaves;k++){
            //         printf("%d ", submatrix->matrix[j][k]);
            //     }
            //     printf("\n");
            // }
            submatrix->rows = n;
            submatrix->cols = n/numSlaves;
            if(send(socket_desc, submatrix, sizeof(struct message_data), 0) < 0){
                printf("Unable to send message\n");
                return -1;
            }
            
            // Receive the server's response:
            if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
                printf("Error while receiving server's msg\n");
                return -1;
            }
            
            printf("Server's response: %s\n",server_message);
            
            // Close the socket:
            close(socket_desc);
        }

        clock_t end = clock();
        printf("Time taken: %f\n", (double)(end - start) / CLOCKS_PER_SEC);

    }
    else if (s == 1)
    {
        //slave - server
        printf("Slave\n");
        //read from the file the first line (ip address of the master)
        FILE *config = fopen("sagun_config.txt", "r");
        int numSlaves;
        fscanf(config, "%d", &numSlaves);
        //the first address is always the master
        char *addr_master = (char *)malloc(16 * sizeof(char));
        int port_master;
        fscanf(config, "%s %d", addr_master, &port_master);
        printf("Master: %s %d\n", addr_master, port_master);
        //read the ip addr and port number for each slave
        char **addr_slaves = (char **)malloc(numSlaves * sizeof(char*));
        for(int i=0;i<numSlaves;i++)
        {
            addr_slaves[i] = (char *)malloc(16 * sizeof(char));
        }
        int *ports_slaves = (int *)malloc(numSlaves * sizeof(int));
        for (int i = 0; i < numSlaves; i++)
        {
            fscanf(config, "%s %d",addr_slaves[i], &ports_slaves[i]);
        }
        //ask the slave number of this instance
        // int slaveNum;
        // printf("Enter the slave number of this instance: ");
        // scanf("%d", &slaveNum);
        // printf("Slave %d's port: %d\n", slaveNum, ports[slaveNum]);
        //socket details declaration
        int socket_desc, client_sock, client_size;
        struct sockaddr_in server_addr, client_addr;
        char server_message[2000], client_message[2000];
        // Clean buffers:
        memset(server_message, '\0', sizeof(server_message));
        memset(client_message, '\0', sizeof(client_message));
        //creation of socket
        socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
        if(socket_desc < 0){
            printf("Error while creating socket\n");
            return -1;
        }
        printf("Socket created successfully\n");
        // Initialize the server address by the port and IP:
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(ports_slaves[0]);
        server_addr.sin_addr.s_addr = inet_addr(addr_slaves[0]);
        
        // Bind the socket descriptor to the server address (the port and IP):
        if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
            printf("Couldn't bind to the port\n");
            return -1;
        }
        printf("Done with binding\n");
        
        // Turn on the socket to listen for incoming connections:
        if(listen(socket_desc, 1) < 0){
            printf("Error while listening\n");
            return -1;
        }
        printf("\nListening for incoming connections.....\n");
        
        /* Store the client’s address and socket descriptor by accepting an
        incoming connection. The server-side code stops and waits at accept()
        until a client calls connect().
        */
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, (socklen_t *)&client_size);
        
        if (client_sock < 0){
            printf("Can't accept\n");
            return -1;
        }
        printf("client connected at ip: %s and port %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //start time when connected
        srand(time(NULL));
        clock_t start = clock();
        // Receive client's message:
        struct message_data *submatrix = (struct message_data *)malloc(sizeof(struct message_data));
        // submatrix->matrix = (int **)malloc(sizeof(int *));
        // for(int i = 0; i < 10; i++){
        //     submatrix->matrix[i] = (int *)malloc(sizeof(int));
        // }

        if (recv(client_sock, submatrix, sizeof(struct message_data), 0) < 0){
            printf("Couldn't receive\n");
            return -1;
        }
        // printf("Msg from client: %s\n", client_message);
        printf("number of rows: %d\n", submatrix->rows);
        printf("number of cols: %d\n", submatrix->cols);
        //print the contents of the matrix from the struct data
        // printf("Matrix received:\n");
        // for (int i = 0; i < submatrix->rows; i++)
        // {
        //     for (int j = 0; j < submatrix->cols; j++)
        //     {
        //         printf("%d ", submatrix->matrix[i][j]);
        //     }
        //     printf("\n");
        // }
        
        // Respond to client:
        strcpy(server_message, "ack\n");
        
        if (send(client_sock, server_message, strlen(server_message), 0) < 0){
            printf("Can't send\n");
            return -1;
        }
        clock_t end = clock();

        printf("Time taken: %f\n", (double)(end - start) / CLOCKS_PER_SEC);
        
        // Closing the socket:
        close(client_sock);
        close(socket_desc);
        }
    else
    {
        printf("Invalid status\n");
        return 1;
    }

    return 0;
}