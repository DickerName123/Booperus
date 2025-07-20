#ifndef VPL6_LOGGING_SOLUTION_STUD
#define VPL6_LOGGING_SOLUTION_STUD

#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>

#define TIMESTAMPSTRINGLENGTH (21)
#define BUFFERSIZE (20)
#define MAX_LINE_LENGTH (512)


typedef enum Level{
    INFO,
    WARNING,
    ERROR,
    DEBUG
} Level;

typedef struct Log{
    time_t timestamp;
    Level level;
    char *msg;
} Log;

typedef struct LoggingInfo{
    sem_t writer_signal;
    sem_t log_mutex;
    char* log_filename;
    bool write_flag;
    int counter;
    Log** logs;
    pthread_t writer;
    bool writer_end_signal;
} LoggingInfo;

bool timestamp_to_string(time_t timestamp, char* timestring);
bool check_conditions(char* string);

LoggingInfo* init_LoggingInfo(char* path);
void logging_cleanup(LoggingInfo* info);
bool log_msg(LoggingInfo* info, time_t timestamp, Level level, char* msg);
char* level_to_string(Level level);
void* writer_thread_routine(void* arg);
bool flush_logBuffer(LoggingInfo* info);
bool create_log_file(char* path);
void to_string(Log* log, char* string);
Log *create_log(time_t timestamp, Level level, char* msg);

#endif
