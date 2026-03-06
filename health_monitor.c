#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

// Shared patient data
int heart_rate = 70;
float body_temp = 36.5;

// Mutex for shared data
pthread_mutex_t data_mutex;

// Utility: set real-time priority
void set_realtime_priority(int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        perror("Failed to set RT priority");
    }
}

// Get current time in ms
long current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// 1️⃣ Sensor Task (Simulated vitals)
void* sensor_task(void* arg) {
    set_realtime_priority(80);

    while (1) {
        pthread_mutex_lock(&data_mutex);

        heart_rate = 60 + rand() % 80;     // 60–140 bpm
        body_temp  = 36.0 + (rand() % 40) / 10.0; // 36.0–40.0 C

        pthread_mutex_unlock(&data_mutex);

        usleep(100000); // 100 ms
    }
}

// 2️⃣ Processing Task
void* processing_task(void* arg) {
    set_realtime_priority(60);

    while (1) {
        pthread_mutex_lock(&data_mutex);

        // Simple smoothing (example logic)
        heart_rate = (heart_rate + 70) / 2;
        body_temp  = (body_temp + 36.5) / 2;

        pthread_mutex_unlock(&data_mutex);

        usleep(200000); // 200 ms
    }
}

// 3️⃣ Alarm Task (CRITICAL)
void* alarm_task(void* arg) {
    set_realtime_priority(95);

    while (1) {
        long start = current_time_ms();

        pthread_mutex_lock(&data_mutex);

        if (heart_rate < 40 || heart_rate > 130 || body_temp > 39.0) {
            printf("[ALARM] HR=%d bpm | Temp=%.1f C\n",
                   heart_rate, body_temp);
        }

        pthread_mutex_unlock(&data_mutex);

        long end = current_time_ms();
        long response = end - start;

        if (response > 50) {
            printf("[WARNING] Alarm latency exceeded: %ld ms\n", response);
        }

        usleep(50000); // 50 ms
    }
}

// 4️⃣ Logging Task (Low priority)
void* logger_task(void* arg) {
    set_realtime_priority(30);

    FILE* log = fopen("patient_log.txt", "w");

    while (1) {
        pthread_mutex_lock(&data_mutex);

        fprintf(log, "HR=%d bpm, Temp=%.1f C\n", heart_rate, body_temp);
        fflush(log);

        pthread_mutex_unlock(&data_mutex);

        usleep(500000); // 500 ms
    }
}

// 🚀 Main
int main() {
    srand(time(NULL));
    pthread_mutex_init(&data_mutex, NULL);

    pthread_t sensor, process, alarm, logger;

    pthread_create(&sensor, NULL, sensor_task, NULL);
    pthread_create(&process, NULL, processing_task, NULL);
    pthread_create(&alarm, NULL, alarm_task, NULL);
    pthread_create(&logger, NULL, logger_task, NULL);

    pthread_join(sensor, NULL);
    pthread_join(process, NULL);
    pthread_join(alarm, NULL);
    pthread_join(logger, NULL);

    pthread_mutex_destroy(&data_mutex);
    return 0;
}