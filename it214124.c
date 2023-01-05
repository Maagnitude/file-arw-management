#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

#define NUM_THREADS 4
#define NUM_CHARS 2000

//WE STARTED HERE

int count_all_letters = 0;

int count_thread[NUM_THREADS];

// A mutex to synchronize access to the shared file
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// A mutex to synchronize access to the shared count table
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// A mutex to synchronize acces to the 

// Calculating the chunk size as the division of the number of chars, by the number of threads
int chunk_size = NUM_CHARS / NUM_THREADS;

// A flag to synchronize the two processes
int flag = 0;

// A table with size=26 to store the count of each character (A-Z)
int count[26] = {0};

void* thread_function(void* arg);

void sigint_handler(int sig);


int main(int argc, char* argv[]) {

    pid_t child_pid = fork();

    if (child_pid == 0) {
    // This is the child process (the reader process)

	printf("Child: Started.\n");

	// Create an array of thread IDs
	pthread_t threads[NUM_THREADS];

	printf("Child: Creats 4 threads.\n");
	// Create the threads
	for (int i = 0; i < NUM_THREADS; i++) {
	    count_thread[i] = i;	
	    pthread_create(&threads[i], NULL, thread_function, &count_thread[i]);
	}

	printf("Child: Join 4 threads.\n");
	// Wait for the threads to finish
	for (int i = 0; i < NUM_THREADS; i++) {
	    pthread_join(threads[i], NULL);
	}

	printf("Child: Prints counter for each character.\n");
	// Print the count of each character
	for (int i = 0; i < 26; i++) {
	    printf("%c: %d\n", 'a' + i, count[i]);
	}
    
    } else {
    // This is the parent process (the writer process)
        
	printf("Parent: Started.\n");
	// Open the file for writing
	int fd = open("data.txt", O_WRONLY | O_TRUNC);

	printf("Parent: Opened file.\n");

	// Generate a string of random characters
	char buf[NUM_CHARS + 1];
	int urandom = open("/dev/urandom", O_RDONLY);
	read(urandom, buf, NUM_CHARS);
	close(urandom);
	int counterletter = 0;
	for (int i = 0; i < NUM_CHARS; i++) {
	    buf[i] = 'a' + ((int)buf[i] % 26);
//	    printf("buf[%d]=%c, ", i, buf[i]);
	    if (buf[i] > 96 && buf[i] < 123) {
	        counterletter++;
	    } else {
	        i--;
	    }
	}
	buf[NUM_CHARS] = '\0';

	// Write the string to the file
	write(fd, buf, NUM_CHARS);

	printf("Parent: Letters writen in file, and counted: %d.\n", counterletter);
	
	// Close the file
	close(fd);

	printf("Parent: Waits for the child.\n");

	// Wait for the child process to finish using waitpid()
	waitpid(child_pid, NULL, 0);
    }  
  
    for (int i = 0; i < 26; i++) {
        count_all_letters += count[i];   
    }

    printf("Generic: Letters in the file: %d.\n", count_all_letters);
    
    printf("Generic: Ending main.\n");
    return 0;  
}

void* thread_function(void* arg) {

    // Lock the mutex to ensure exclusive access to the file
    pthread_mutex_lock(&file_mutex);

    printf("Child: Inside the file_mutex.\n");
    // Open the file for reading
    int fd = open("data.txt", O_RDONLY);

    // Seek to the correct position in the file
    lseek(fd, (long)arg * chunk_size, SEEK_CUR);

    // Read the chunk of data from the file
    char buf[chunk_size + 1];
    size_t num_read = read(fd, buf, chunk_size);
    buf[num_read] = '\0';

    // Close the file
    close(fd);

    // Unlock the mutex to allow other threads to access the file
    pthread_mutex_unlock(&file_mutex);

    printf("Child: Outside the file_mutex.\n");

    int partialcount[26] = {0};
    //Process the chunk of data and update the count table
    for (int i = 0; i < num_read; i++) {
       int index = buf[i] - 'a';
       if (index >= 0 && index < 26) {
           // Lock the mutex to ensure exclusive access to the count table
//	   pthread_mutex_lock(&count_mutex);

	   // Update the count for this character
//	   count[index]++;

	   // Unlock the mutex to allow other threads to access the count table
//	   pthread_mutex_unlock(&count_mutex);
           partialcount[index] ++;
	   
       }
    }
    pthread_mutex_lock(&count_mutex);
    for (int i = 0; i < 26; i++) {
        count[i] += partialcount[i];
    }
    pthread_mutex_unlock(&count_mutex); 

    return NULL;
}

