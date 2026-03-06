#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

// ---------------- CONFIG ----------------
#define SENSOR_PERIOD_MS   100
#define PROCESS_PERIOD_MS  200
#define ALARM_PERIOD_MS     50
#define LOGGER_PERIOD_MS   500

// ----------------------------------------

int heart_rate = 70;
float body_temp = 36.5;

int sensor_deadline_miss = 0;
int process_deadline_miss = 0;
int alarm_deadline_miss = 0;

pthread_mutex_t data_mutex;
FILE *logfile;

// ---------------- UTILITIES ----------------
long now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void set_rt_priority(int prio) {
    struct sched_param param;
    param.sched_priority = prio;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        perror("RT priority set failed");
    }
}

// ---------------- TASKS ----------------

// 1️⃣ SENSOR TASK
void* sensor_task(void* arg) {
    set_rt_priority(80);

    while (1) {
        long start = now_ms();

        pthread_mutex_lock(&data_mutex);
        heart_rate = 60 + rand() % 90;                 // 60–150
        body_temp  = 36.0 + (rand() % 50) / 10.0;      // 36–41
        pthread_mutex_unlock(&data_mutex);

        long exec = now_ms() - start;
        if (exec > SENSOR_PERIOD_MS) sensor_deadline_miss++;

        usleep(SENSOR_PERIOD_MS * 1000);
    }
}

// 2️⃣ PROCESSING TASK
void* processing_task(void* arg) {
    set_rt_priority(60);

    while (1) {
        long start = now_ms();

        pthread_mutex_lock(&data_mutex);
        heart_rate = (heart_rate + 70) / 2;
        body_temp  = (body_temp + 36.5) / 2;
        pthread_mutex_unlock(&data_mutex);

        long exec = now_ms() - start;
        if (exec > PROCESS_PERIOD_MS) process_deadline_miss++;

        usleep(PROCESS_PERIOD_MS * 1000);
    }
}

// 3️⃣ ALARM TASK (CRITICAL)
void* alarm_task(void* arg) {
    set_rt_priority(95);
    static int alarm_active = 0;

    while (1) {
        long start = now_ms();

        pthread_mutex_lock(&data_mutex);
        int hr = heart_rate;
        float temp = body_temp;
        pthread_mutex_unlock(&data_mutex);

        if ((hr < 40 || hr > 130 || temp > 39.0)) {
            if (!alarm_active) {
                long latency = now_ms() - start;
                fprintf(logfile,
                        "ALARM,%ld,%d,%.1f\n",
                        latency, hr, temp);
                fflush(logfile);
                alarm_active = 1;
            }
        } else {
            alarm_active = 0;
        }

        long exec = now_ms() - start;
        if (exec > ALARM_PERIOD_MS) alarm_deadline_miss++;

        usleep(ALARM_PERIOD_MS * 1000);
    }
}

// 4️⃣ LOGGER TASK
void* logger_task(void* arg) {
    set_rt_priority(30);

    while (1) {
        fprintf(logfile,
                "DATA,%ld,%d,%.1f,%d,%d,%d\n",
                now_ms(),
                heart_rate,
                body_temp,
                sensor_deadline_miss,
                process_deadline_miss,
                alarm_deadline_miss);
        fflush(logfile);

        usleep(LOGGER_PERIOD_MS * 1000);
    }
}

// ---------------- MAIN ----------------
int main() {
    srand(time(NULL));
    pthread_mutex_init(&data_mutex, NULL);

    logfile = fopen("health_log.csv", "w");
    fprintf(logfile,
            "TYPE,LATENCY,HR,TEMP,SENSOR_DM,PROCESS_DM,ALARM_DM\n");

    pthread_t sensor, process, alarm, logger;

    pthread_create(&sensor, NULL, sensor_task, NULL);
    pthread_create(&process, NULL, processing_task, NULL);
    pthread_create(&alarm, NULL, alarm_task, NULL);
    pthread_create(&logger, NULL, logger_task, NULL);

    pthread_join(sensor, NULL);
    pthread_join(process, NULL);
    pthread_join(alarm, NULL);
    pthread_join(logger, NULL);

    fclose(logfile);
    pthread_mutex_destroy(&data_mutex);
    return 0;
}