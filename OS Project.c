#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

pthread_mutex_t lock1, lock2;
int simulate_deadlock = 0;
int timeout_seconds = 2; // Default timeout
int repeat_count = 1;    // Default number of repeats

// Function to log messages with timestamps
void log_with_time(const char* message, const char* thread_name) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);
    printf("[%s] %s: %s\n", timebuf, thread_name, message);
}

// Function for timed lock to avoid deadlock
int timed_mutex_lock(pthread_mutex_t *mutex, int seconds) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += seconds;
    return pthread_mutex_timedlock(mutex, &ts);
}

// Thread function
void* thread_func(void* arg) {
    char* thread_name = (char*)arg;

    // Simulate deadlock if enabled
    if (simulate_deadlock && strcmp(thread_name, "Thread B") == 0) {
        log_with_time("trying to lock Mutex 2", thread_name);
        if (timed_mutex_lock(&lock2, timeout_seconds) != 0) {
            log_with_time("failed to acquire Mutex 2 (possible deadlock)", thread_name);
            return NULL;
        }
        log_with_time("locked Mutex 2", thread_name);

        sleep(1); // Simulate work

        log_with_time("trying to lock Mutex 1", thread_name);
        if (timed_mutex_lock(&lock1, timeout_seconds) != 0) {
            log_with_time("failed to acquire Mutex 1 (possible deadlock)", thread_name);
            pthread_mutex_unlock(&lock2);
            return NULL;
        }
        log_with_time("locked Mutex 1", thread_name);
    } else {
        // Normal lock order: lock1 then lock2
        log_with_time("trying to lock Mutex 1", thread_name);
        if (timed_mutex_lock(&lock1, timeout_seconds) != 0) {
            log_with_time("failed to acquire Mutex 1 (possible deadlock)", thread_name);
            return NULL;
        }
        log_with_time("locked Mutex 1", thread_name);

        sleep(1); // Simulate work

        log_with_time("trying to lock Mutex 2", thread_name);
        if (timed_mutex_lock(&lock2, timeout_seconds) != 0) {
            log_with_time("failed to acquire Mutex 2 (possible deadlock)", thread_name);
            pthread_mutex_unlock(&lock1);
            return NULL;
        }
        log_with_time("locked Mutex 2", thread_name);
    }

    // Critical section
    log_with_time("in critical section", thread_name);

    // Release locks
    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);

    log_with_time("released both locks", thread_name);
    return NULL;
}

// Function to parse command-line arguments
void parse_args(int argc, char* argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "dht:r:")) != -1) {
        switch (opt) {
            case 'd':
                simulate_deadlock = 1;
                break;
            case 't':
                timeout_seconds = atoi(optarg);
                break;
            case 'r':
                repeat_count = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-d] [-t timeout] [-r repeat_count]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[]) {
    parse_args(argc, argv);
	int i;
    for ( i = 0; i < repeat_count; i++) {
        pthread_t t1, t2;

        pthread_mutex_init(&lock1, NULL);
        pthread_mutex_init(&lock2, NULL);

        printf("Main: Starting threads %sdeadlock prevention with %d seconds timeout and %d repeats...\n",
               simulate_deadlock ? "WITHOUT " : "WITH ", timeout_seconds, repeat_count);

        pthread_create(&t1, NULL, thread_func, "Thread A");
        pthread_create(&t2, NULL, thread_func, "Thread B");

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);

        printf("Main: Both threads completed%s\n", simulate_deadlock ? " (may have deadlocked!)" : " successfully (no deadlock)");

        pthread_mutex_destroy(&lock1);
        pthread_mutex_destroy(&lock2);
    }

    return 0;
}
