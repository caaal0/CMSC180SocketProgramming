#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct thread_args
{
    // int **matrix;
    // int *y;
    double **matrix;
    double *y;
    int rows;
    int cols;
    int port;
    char *master_address;
    char *slave_address;
    int starting_index;
    double *result;
};

struct matrix_details
{
    int rows;
    int cols;
};

// Function to divide the matrix into t submatrices
double ***divide_matrix(double **X, int m, int n, int t)
{
    printf("dividing matrix...\n");
    int remainingRows = m % t;

    // Allocate memory for the array of matrices
    double ***submatrices = (double ***)malloc(t * sizeof(double **));
    for (int i = 0; i < t; i++)
    {
        submatrices[i] = (double **)malloc(m * sizeof(double *));
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
                    submatrices[i][j] = (double *)malloc((stop-start) * sizeof(double));
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
                //         printf("%f ", submatrices[i][j][k]);
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
                submatrices[i][j] = (double *)malloc((stop-start) * sizeof(double));
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

void *sendMatrix(void *args){
    struct thread_args *my_data = (struct thread_args *)args;
    double **submatrix = my_data->matrix;
    int rows = my_data->rows;
    int cols = my_data->cols;
    int port = my_data->port;
    double *y = my_data->y;
    double *result = my_data->result;
    int starting_index = my_data->starting_index;
    char *master_address = my_data->master_address;
    char *slave_address = my_data->slave_address;
    //open port p, distribute the submatrices to slaves
    int socket_desc;
    struct sockaddr_in server_addr;
    char server_message[2000], client_message[2000];

    // Clean buffers:
    memset(server_message,'\0',sizeof(server_message));
    memset(client_message,'\0',sizeof(client_message));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Unable to create socket\n");
        pthread_exit(NULL);
    }
    
    printf("Socket created successfully\n");
    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(slave_address);
    
    // Send a connection request to the server, which is waiting at accept():
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        pthread_exit(NULL);
    }
    printf("Connected with slave successfully\n");

    //send the matrix details first
    struct matrix_details *matrix_details = (struct matrix_details *)malloc(sizeof(struct matrix_details));
    // struct matrix_details matrix_details;
    matrix_details->rows = rows;
    matrix_details->cols = cols;

    if(send(socket_desc, matrix_details, sizeof(matrix_details), 0) < 0){
        printf("Unable to send matrix details\n");
        pthread_exit(NULL);
    }
    
    //for serializing
    double buffer[rows*cols];
    // Serialize the 2D array into a 1D buffer
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            buffer[i * cols + j] = submatrix[i][j];
        }
    }
    printf("Sending to port %d\n", port);

    // Send the message to slave:
    if(send(socket_desc, buffer, sizeof(buffer), 0) < 0){
        printf("Unable to send message\n");
        pthread_exit(NULL);
    }

    double vector[rows];
    for(int i = 0; i < rows; i++){
        vector[i] = y[i];
        // printf("%d ", vector[i]);
    }

    //send the vector as well
    if(send(socket_desc, vector, sizeof(vector), 0) < 0){
        printf("Unable to send message\n");
        pthread_exit(NULL);
    }

    //receive the result from slave
    double r[cols];
    if(recv(socket_desc, r, sizeof(r), 0) < 0){
        printf("Error while receiving server's msg\n");pthread_exit(NULL);
        pthread_exit(NULL);
    }

    for(int i = 0; i < cols; i++){
        result[starting_index + i] = r[i];
    }
    
    // Receive the server's response:
    if(recv(socket_desc, server_message, sizeof(server_message), 0) < 0){
        printf("Error while receiving server's msg\n");pthread_exit(NULL);
        pthread_exit(NULL);
    }
    
    printf("Slave from port %d: %s\n",port, server_message);
    
    // Close the socket:
    close(socket_desc);
    //free allocated resources
    // free(matrix_details);
    // free(my_data);
    //exit thread
    pthread_exit(NULL);
}

int main(){

    int s;
    printf("Enter status of this instance (0 = master, 1 = slave): ");
    scanf("%d", &s);

    //then the number of slave ports (for same pc instances)
    FILE *config = fopen("sagun_config.txt", "r");
    int numSlaves;
    fscanf(config, "%d", &numSlaves);

    char ** ip_slaves = (char **)malloc(numSlaves * sizeof(char*));

    //the first address is always the master
    char *addr_master = (char *)malloc(16 * sizeof(char));
    int port_master;
    for(int i = 0; i<numSlaves;i++){
        ip_slaves[i] = (char *)malloc(16 * sizeof(char));
    }
    fscanf(config, "%s %d", addr_master, &port_master);
    printf("Master: %s\n", addr_master);
    //then the ports themselves
    int port_slaves[numSlaves];
    for(int i = 0; i<numSlaves;i++){
        fscanf(config, "%s %d", ip_slaves[i], &port_slaves[i]);
        printf("Slave[%d] = %s %d\n", i, ip_slaves[i], port_slaves[i]);
    }

    // return 0;

    //check if the status is master or slave
    if(s == 0){
        //dummy data
        // Define and initialize the weight array
        int n = 10;
        double **weight = (double **)malloc(10 * sizeof(double *));
        for(int i = 0; i < 10; i++) {
            weight[i] = (double *)malloc(10 * sizeof(double));
            for(int j = 0; j < 10; j++) {
                switch(i){
                    case 0:
                        weight[i][j] = 3.63;
                        break;
                    case 1:
                        weight[i][j] = 3.02;
                        break;
                    case 2:
                        weight[i][j] = 3.82;
                        break;
                    case 3:
                        weight[i][j] = 3.42;
                        break;
                    case 4:
                        weight[i][j] = 3.59;
                        break;
                    case 5:
                        weight[i][j] = 2.87;
                        break;
                    case 6:
                        weight[i][j] = 3.03;
                        break;
                    case 7:
                        weight[i][j] = 3.46;
                        break;
                    case 8:
                        weight[i][j] = 3.36;
                        break;
                    case 9:
                        weight[i][j] = 3.3;
                        break;
                }
            }
        }
        
        // Define and initialize the height array
        double height[10] = {53.1, 49.7, 48.4, 54.2, 54.9, 43.7, 47.2, 45.2, 54.4, 50.4};

        //make the results array
        double *result = (double *)malloc(10 * sizeof(double));
        // printf("\n");
        //master
        //divide the submatrices
        double ***submatrices = divide_matrix(weight, n, n, numSlaves);
        pthread_t tid[numSlaves];

        srand(time(NULL));
        clock_t start = clock();
        //make a thread for each slave
        int starting_index = 0;
        for(int i = 0; i < numSlaves;i++)
        {
            struct thread_args *args = (struct thread_args *)malloc(sizeof(struct thread_args));
            args->matrix = submatrices[i];
            args->y = height;
            args->rows = n;
            args->cols = n/numSlaves;
            if(n < numSlaves && i < n){
                args->cols = 1;
            }
            args->port = port_slaves[i];
            args->master_address = addr_master;
            args->slave_address = ip_slaves[i];
            args->starting_index = starting_index;
            args->result = result;
            //create the thread
            pthread_create(&tid[i], NULL, sendMatrix, args);
            starting_index+=args->cols;
        }
        //join the threads
        for(int i = 0; i < numSlaves;i++)
        {
            pthread_join(tid[i], NULL);
        }
        clock_t end = clock();
        printf("Time taken: %f\n", (double)(end - start) / CLOCKS_PER_SEC);
        printf("Results:\n");
        for(int i = 0; i < n; i++){
            printf("%f ", result[i]);
        }
        printf("\n");
        for (int i = 0; i < n; i++){
            // free(X[i]);
            free(weight[i]);
        }
        // free(X);
        // free(y);
        free(weight);
    }else if(s == 1){
        //slave
        //slave - server
        printf("Slave\n");
        //ask the slave number of this instance
        int slaveNum;
        printf("Enter the slave number of this instance: ");
        scanf("%d", &slaveNum);
        printf("Slave %d's port: %d\n", slaveNum, port_slaves[slaveNum]);
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
        server_addr.sin_port = htons(port_slaves[slaveNum]);
        server_addr.sin_addr.s_addr = inet_addr(ip_slaves[slaveNum]);
        
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
        printf("\nWaiting for master.....\n");
        
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
        // printf("client connected at ip: %s and port %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //start time when connected
        srand(time(NULL));
        clock_t start = clock();
        //receive matrix details first
        struct matrix_details *submatrix = (struct matrix_details *)malloc(sizeof(struct matrix_details));
        if(recv(client_sock, submatrix, sizeof(submatrix), 0) < 0){
            printf("Couldn't receive matrix details\n");
            return -1;
        }
        printf("Matrix details received: %d rows, %d cols\n", submatrix->rows, submatrix->cols);
        // Receive client's message:
        double buffer[submatrix->rows*submatrix->cols];

        if (recv(client_sock, buffer, sizeof(buffer), 0) < 0){
            printf("Couldn't receive\n");
            return -1;
        }
        // Reconstruct the 2D array
        double x[submatrix->rows][submatrix->cols];
        for (int i = 0; i < submatrix->rows; ++i) {
            for (int j = 0; j < submatrix->cols; ++j) {
                x[i][j] = buffer[i * submatrix->cols + j];
            }
        }

        printf("Matrix received:\n");
        for (int i = 0; i < submatrix->rows; i++)
        {
            for (int j = 0; j < submatrix->cols; j++)
            {
                printf("%f ", x[i][j]);
            }
            printf("\n");
        }

        //receive the vector
        double y[submatrix->rows];
        if(recv(client_sock, y, sizeof(y), 0) < 0){
            printf("Couldn't receive\n");
            return -1;
        }
        printf("Vector received\n");
        //print vector
        for(int i = 0; i < submatrix->rows; i++){
            printf("%f ", y[i]);
        }
        printf("\n");

        // r will hold the coefficients
        double r[submatrix->cols];

        //solve for the pearson coefficient
        double sum_y = 0.0;
        double sum_ysquare = 0.0;

        for (int i = 0; i < submatrix->rows; i++) {
            sum_y += y[i];
            sum_ysquare += pow(y[i], 2);
        }

        for (int col = 0; col < submatrix->cols; col++) {
            double sum_x = 0.0;
            double sum_xsquare = 0.0;
            double sum_xy = 0.0;

            for (int row = 0; row < submatrix->rows; row++) {
                sum_x += x[row][col];
                sum_xsquare += pow(x[row][col], 2);
                sum_xy += x[row][col] * y[row];
            }

            // printf("sum_x = %f, sum_xsquare = %f, sum_y = %f, sum_ysquare = %f, sum_xy = %f\n", sum_x, sum_xsquare, sum_y, sum_ysquare, sum_xy);

            double upper = submatrix->rows * sum_xy - sum_x * sum_y;
            double lower = sqrt((submatrix->rows * sum_xsquare - pow(sum_x, 2)) * (submatrix->rows * sum_ysquare - pow(sum_y, 2)));
            r[col] = upper / lower;
        }

        //print the results
        for(int i = 0; i < submatrix->cols; i++){
            printf("r[%d] = %f\n", i, r[i]);
        }

        //send the results to master
        if (send(client_sock, r, sizeof(r), 0) < 0){
            printf("Can't send results\n");
            return -1;
        }
        
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
        // free(y);
    }else{
        printf("Invalid status!\n");
    }
    
    return 0;
}