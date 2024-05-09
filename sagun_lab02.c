#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <windows.h>
#include <process.h>

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

// pthread_mutex_t lock;
HANDLE lock;
double *sum_x;
double *sum_xsquare;
double *sum_xy;

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
                        submatrices[i][j][colIndex] = (int)malloc(sizeof(int));
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
                    submatrices[i][j][colIndex] = (int)malloc(sizeof(int));
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

// Function to free the memory allocated for submatrices
void free_submatrices(int ***submatrices, int m, int t)
{
    //if the matrices were divided by column
    // for (int i = 0; i < t; i++)
    // {
    //     for (int j = 0; j < m; j++)
    //     {
    //         free(submatrices[i][j]);
    //     }
    //     free(submatrices[i]);
    // }
    // free(submatrices);
    //if the matrices were divided by row
    for (int i = 0; i < t; i++)
    {
        for (int j = 0; j < m/t; j++)
        {
            free(submatrices[i][j]);
        }
        free(submatrices[i]);
    }
    free(submatrices);
}

void normal_pearson_cor(int **x, int *y, int m, double *r) {
    double sum_y = 0.0;
    double sum_ysquare = 0.0;

    for (int i = 0; i < m; i++) {
        sum_y += y[i];
        sum_ysquare += pow(y[i], 2);
    }

    for (int col = 0; col < m; col++) {
        double sum_x = 0.0;
        double sum_xsquare = 0.0;
        double sum_xy = 0.0;

        for (int row = 0; row < m; row++) {
            sum_x += x[row][col];
            sum_xsquare += pow(x[row][col], 2);
            sum_xy += x[row][col] * y[row];
        }

        double upper = m * sum_xy - sum_x * sum_y;
        double lower = sqrt((m * sum_xsquare - pow(sum_x, 2)) * (m * sum_ysquare - pow(sum_y, 2)));
        r[col] = upper / lower;
    }
}

void *pearson_cor(void *threadarg)
{
    struct thread_data *my_data;
    my_data = (struct thread_data *)threadarg;
    int **x, *y, m, n, tnum;
    double *r;
    x = my_data->matrix;
    y = my_data->vect;
    m = my_data->x;
    n = my_data->y;
    r = my_data->results;
    tnum = my_data->tnum;
    int starting_index = my_data->starting_index;
    // printf("thread %d is running\n", tnum);

    double sum_y = 0.0;
    double sum_ysquare = 0.0;

    for (int i = 0; i < m; i++)
    {
        sum_y += y[i];
        sum_ysquare += pow(y[i], 2);
    }

    for (int col = 0; col < n; col++)
    {
        double sum_x = 0.0;
        double sum_xsquare = 0.0;
        double sum_xy = 0.0;

        for (int row = 0; row < m; row++)
        {
            sum_x += x[row][col];
            sum_xsquare += pow(x[row][col], 2);
            sum_xy += x[row][col] * y[row];
        }

        double upper = m * sum_xy - sum_x * sum_y;
        double lower = sqrt((m * sum_xsquare - pow(sum_x, 2)) * (m * sum_ysquare - pow(sum_y, 2)));
        r[col + starting_index] = upper / lower; //only works with equally divided submatrices
        // printf("r[%d] = %lf\n", col, r[col]);
    }
    // printf("thread %d exiting...\n", tnum);
    free(threadarg);
    pthread_exit(NULL);
}

unsigned int __stdcall pearson_cor_windows(void *threadarg)
{
    struct thread_data *my_data;
    my_data = (struct thread_data *)threadarg;
    int **x, *y, m, n, tnum;
    double *r;
    x = my_data->matrix;
    y = my_data->vect;
    m = my_data->x;
    n = my_data->y;
    r = my_data->results;
    tnum = my_data->tnum;
    int starting_index = my_data->starting_index;
    // printf("thread %d is running\n", tnum);

    double sum_y = 0.0;
    double sum_ysquare = 0.0;

    for (int i = 0; i < m; i++)
    {
        sum_y += y[i];
        sum_ysquare += pow(y[i], 2);
    }

    for (int col = 0; col < n; col++)
    {
        double sum_x = 0.0;
        double sum_xsquare = 0.0;
        double sum_xy = 0.0;

        for (int row = 0; row < m; row++)
        {
            sum_x += x[row][col];
            sum_xsquare += pow(x[row][col], 2);
            sum_xy += x[row][col] * y[row];
        }

        double upper = m * sum_xy - sum_x * sum_y;
        double lower = sqrt((m * sum_xsquare - pow(sum_x, 2)) * (m * sum_ysquare - pow(sum_y, 2)));
        r[col + starting_index] = upper / lower; //only works with equally divided submatrices
        // printf("r[%d] = %lf\n", col, r[col]);
    }
    // printf("thread %d exiting...\n", tnum);
    free(threadarg);
    // pthread_exit(NULL);
    _endthreadex(0);
    return 0;
}

int*** divide_matrix_row(int** matrix, int rows, int cols, int t) {
    int remaining_rows = rows % t;
    int rows_per_thread = rows / t;
    int*** submatrices = (int***)malloc(t * sizeof(int**));

    int start_row = 0;
    for (int i = 0; i < t; i++) {
        int end_row = start_row + rows_per_thread + (i < remaining_rows ? 1 : 0);
        submatrices[i] = (int**)malloc((end_row - start_row) * sizeof(int*));

        for (int j = start_row; j < end_row; j++) {
            submatrices[i][j - start_row] = (int*)malloc(cols * sizeof(int));
            for (int k = 0; k < cols; k++) {
                submatrices[i][j - start_row][k] = matrix[j][k];
            }
        }

        start_row = end_row;
    }

    return submatrices;
}

unsigned int __stdcall pearson_cor_windows_row(void *threadarg)
{
    // store the sums in the row first? so that means, using mutex
    struct thread_data *my_data;
    my_data = (struct thread_data *)threadarg;
    //for int data
    int **x, *y, m, n, tnum;

    //for dummy data
    // int m, n, tnum;
    // double **x, *y;

    double *r;
    x = my_data->matrix;
    y = my_data->vect;
    m = my_data->x;
    n = my_data->y;
    r = my_data->results;
    tnum = my_data->tnum;
    int starting_index = my_data->starting_index;
    // printf("thread %d is running\n", tnum);
    DWORD dwWaitResult;
    // double sum_y = 0.0;
    // double sum_ysquare = 0.0;

    // for (int i = 0; i < m; i++)
    // {
    //     sum_y += y[i];
    //     sum_ysquare += pow(y[i], 2);
    // }

    //the threads can only access the sum arrays one at a time
    // pthread_mutex_lock(&lock);

    // pthread_mutex_unlock(&lock);
    dwWaitResult = WaitForSingleObject(lock, INFINITE); //wait for the mutex to be free

    switch (dwWaitResult){
        case WAIT_OBJECT_0:
            // printf("Thread %d got the lock\n", tnum);
            for(int row = 0; row < m; row++)
            {
                for(int col = 0; col < n; col++)
                {
                    sum_x[col] += x[row][col];
                    sum_xsquare[col] += pow(x[row][col], 2);
                    sum_xy[col] += x[row][col] * y[starting_index+row];
                    // if(col == 5){
                    //     printf("sum_xy[%d] is %f\n", col, sum_xy[col]);
                    //     printf("%f x %f = %f\n",x[row][col], y[row], x[row][col]*y[row]);
                    // }
                }
            }
            // printf("thread %d finished\n", tnum);
            ReleaseMutex(lock);
            break;
        case WAIT_ABANDONED:
            // printf("abandoned mutex for thread %d\n", tnum);
            // free(threadarg);
            // _endthreadex(0);
            break;
    }
    // printf("thread %d exiting...\n", tnum);
    free(threadarg);
    _endthreadex(0);
}
//to test dummy data
double*** divide_matrix_row_double(double** matrix, int rows, int cols, int t) {
    int remaining_rows = rows % t;
    int rows_per_thread = rows / t;
    double*** submatrices = (double***)malloc(t * sizeof(double**));

    int start_row = 0;
    for (int i = 0; i < t; i++) {
        int end_row = start_row + rows_per_thread + (i < remaining_rows ? 1 : 0);
        submatrices[i] = (double**)malloc((end_row - start_row) * sizeof(double*));

        for (int j = start_row; j < end_row; j++) {
            submatrices[i][j - start_row] = (double*)malloc(cols * sizeof(double));
            for (int k = 0; k < cols; k++) {
                submatrices[i][j - start_row][k] = matrix[j][k];
            }
        }

        start_row = end_row;
    }

    return submatrices;
}

int main()
{
    //dummy data
    // Define and initialize the weight array
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

    int m, t;
    printf("Enter number of rows in the matrix: ");
    scanf("%d", &m);
    printf("Enter number of threads to be made: ");
    scanf("%d", &t);

    int **X = (int **)malloc(m * sizeof(int *));
    for (int i = 0; i < m; i++)
    {
        X[i] = (int *)malloc(m * sizeof(int));
        if (X[i] == NULL)
        {
            printf("Memory allocation failed\n");
            return 1; //exits program
        }
    }

    // Make random matrix
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < m; j++)
        {
            X[i][j] = rand() % 100 + 1;
            // printf("%d ", X[i][j]);
        }
        // printf("\n");
    }

    int *y = (int *)malloc(m * sizeof(int));
    

    // check if memory allocation failed
    if (y == NULL)
    {
        printf("Memory allocation failed\n");
        return 1; // or handle error appropriately
    }

    // Make random y vector
    for (int i = 0; i < m; i++)
    {
        y[i] = rand() % 100 + 1;
    }

    // divide into submatrices (row-major)
    int ***submatrices = divide_matrix_row(X, m, m, t);
    //test dummy data
    // double ***submatrices = divide_matrix_row_double(weight, 10, 10, t);
    // column-major
    // int ***submatrices = divide_matrix(X, m, m, t);
    //print the submatrices (Row-major)
    // for(int i = 0;i < t;i++){
    //     printf("Subm %d\n",i);
    //     for(int j = 0; j<(m/t)+(i<m%t? 1:0);j++){
    //         for(int k = 0; k<m;k++){
    //             printf("%d ", submatrices[i][j][k]);
    //         }
    //         printf("\n");
    //     }
    // }
    printf("Size of matrix = %dx%d\n", m, m);

    //this is for the row-major implementation
    sum_x = (double *)malloc(m * sizeof(double));
    sum_xsquare = (double *)malloc(m * sizeof(double));
    sum_xy = (double *)malloc(m * sizeof(double));
    if(sum_x == NULL || sum_xsquare == NULL || sum_xy == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    srand(time(NULL));

    // Make 3 runs to get the average case
    for (int run = 0; run < 3; run++)
    {
        double *r = (double *)malloc(m * sizeof(double));
        for(int i = 0; i<m; i++){
            sum_x[i] = 0.0;
            sum_xsquare[i] = 0.0;
            sum_xy[i] = 0.0;
        }
        printf("----- RUN %d -----\n", run+1);
        //create threads before starting the clock
        // pthread_t threads[t];
        //windows api
        HANDLE threads[t];
        //make the lock
        lock = CreateMutex(NULL, FALSE, NULL);
        if(lock == NULL){
            printf("Mutex creation failed\n");
            return 1;
        }
        clock_t start = clock();

        // normal_pearson_cor(X, y, m, r);
        int starting_index = 0;
        for(int i = 0; i < t; i++){
            struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
            data->matrix = submatrices[i];
            // data->vect = height;
            data->vect = y;
            //for row-major implementation
            data->x = m%t==0? m/t:i<m%t? (m/t)+1:m/t;
            data->y = m;
            //for column-major implementation
            // data->x = m;
            // data->y = m%t==0? m/t:i<m%t? (m/t)+1:m/t; // uses the same logic with dividing uneven matrix
            data->results = r;
            data->tnum = i;
            data->starting_index = starting_index;
            // printf("r address is %p\n", r);
            // printf("Size of submatrix %d = %dx%d\n", i+1, data->x, data->y);
            //create threads using pthreads
            // pthread_create(&threads[i], NULL, pearson_cor, (void *)data);
            //create threads using windows api
            // threads[i] = (HANDLE)_beginthreadex(NULL, 0, pearson_cor_windows, (void *)data, 0, NULL);
            //row-major implementation
            threads[i] = (HANDLE)_beginthreadex(NULL, 0, pearson_cor_windows_row, (void *)data, 0, NULL);
            if (threads[i] == NULL) {
                fprintf(stderr, "Error creating thread %d\n", i);
                return 1;
            }
            //column-nmajor
            // starting_index += data->y;
            //row-major
            starting_index += data->x;
        }
        // Wait for all threads to finish
        WaitForMultipleObjects(t, threads, TRUE, INFINITE);
        printf("joined threads...\n");
        for(int i = 0;i<t;i++){
            CloseHandle(threads[i]);
        }
        //all threads are finished at this point, if row-major, can now calculate the r vector
        double sum_y = 0.0;
        double sum_ysquare = 0.0;

        for (int i = 0; i < m; i++)
        {
            // sum_y += height[i];
            // sum_ysquare += pow(height[i], 2);
            sum_y += y[i];
            sum_ysquare += pow(y[i], 2);
        }
        for (int col = 0; col < m; col++)
        {
            double upper = m * sum_xy[col] - sum_x[col] * sum_y;
            double lower = sqrt((m * sum_xsquare[col] - pow(sum_x[col], 2)) * (m * sum_ysquare - pow(sum_y, 2)));
            r[col] = upper / lower;
            // printf("r[%d] = %lf\n", col, r[col]);
        }
        // CloseHandle(lock);
        // printf("closed handles\n");
        // for(int i = 0; i < m;i++){
        //     printf("sum_x: %f, sum_xsquare: %f, sum_xy: %f, sum_y: %f, sum_ysquare: %f\n ", sum_x[i], sum_xsquare[i], sum_xy[i], sum_y, sum_ysquare);
        // }
        // printf("size of r: %d\n", sizeof(r));

        clock_t end = clock();

        double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;

        // printf("Pearson coefficients: ");
        // for (int i = 0; i < m; i++) {
        //     printf("%lf ", r[i]);
        // }
        printf("\n");

        printf("Run %d time: %f\n", run+1, time_taken);
        free(r);
    }

    printf("\n");

    // Free allocated memory
    for (int i = 0; i < m; i++)
    {
        free(X[i]);
    }
    free(X);
    free(y);
    // printf("Freeing submatrices...\n");
    free_submatrices(submatrices, m, t);
    free(sum_x);
    free(sum_xsquare);
    free(sum_xy);
    printf("Exiting program...\n");
    return 0;
}
