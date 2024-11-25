#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>


#define P_THR 3  // Number of parent threads
#define C_THR 10 // Number of child threads
#define P_NUM 500 // Numbers generated by each parent thread
#define C_NUM 150 // Numbers read by each child thread

int pipe_fd[2]; // Pipe for communication
pthread_mutex_t p_lock; // Mutex for synchronizing pipe access
volatile int is_ready = 0; // Flag to signal readiness

// Parent thread function: generates random numbers and writes to the pipe
void* p_task(void* arg) {
    int buf[P_NUM];
    for (int i = 0; i < P_NUM; i++) {
        buf[i] = rand() % 1001; // Random numbers 0-1000
    }

    pthread_mutex_lock(&p_lock);
    write(pipe_fd[1], buf, sizeof(buf)); // Write data to pipe
    pthread_mutex_unlock(&p_lock);
    return NULL;
}

// Signal handler: sets readiness flag
void sig_handler(int sig) {
    if (sig == SIGUSR1) {
        is_ready = 1; // Mark child process as ready
    }
}

// Child thread function: reads numbers from pipe and calculates their sum
void* c_task(void* arg) {
    int buf[C_NUM];
    int* sum = (int*)arg;

    read(pipe_fd[0], buf, sizeof(buf)); // Read data from pipe
    *sum = 0;
    for (int i = 0; i < C_NUM; i++) {
        *sum += buf[i]; // Calculate sum
    }
    return NULL;
}

int main() {
    pid_t pid;
    pthread_t p_thr[P_THR];
    pthread_t c_thr[C_THR];
    int sums[C_THR], total = 0;
    double avg;

    srand(time(NULL)); // Initialize random seed
    pthread_mutex_init(&p_lock, NULL); // Initialize mutex
    pipe(pipe_fd); // Create pipe

    pid = fork(); // Create child process
    if (pid == 0) {
        // Child process
        signal(SIGUSR1, sig_handler); // Setup signal handler
        while (!is_ready); // Wait for parent signal

        for (int i = 0; i < C_THR; i++) {
            pthread_create(&c_thr[i], NULL, c_task, &sums[i]); // Start child threads
        }

        for (int i = 0; i < C_THR; i++) {
            pthread_join(c_thr[i], NULL); // Wait for threads to finish
            total += sums[i]; // Add each thread's sum to total
        }

        avg = (double)total / C_THR; // Calculate average
        freopen("result.txt", "w", stdout); // Redirect output to file
        printf("Average: %.2f\n", avg); // Output average
        close(pipe_fd[0]); // Close pipe read end
        exit(0); // End child process
    } else {
        // Parent process
        for (int i = 0; i < P_THR; i++) {
            pthread_create(&p_thr[i], NULL, p_task, NULL); // Start parent threads
        }

        for (int i = 0; i < P_THR; i++) {
            pthread_join(p_thr[i], NULL); // Wait for threads to finish
        }

        kill(pid, SIGUSR1); // Send signal to child
        close(pipe_fd[1]); // Close pipe write end
        wait(NULL); // Wait for child to finish
    }

    pthread_mutex_destroy(&p_lock); // Clean up mutex
    return 0;
}
