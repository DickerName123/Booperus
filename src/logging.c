#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "logging.h"

/* --------------------------------------
 * Helper functions (already implemented)
 * -------------------------------------- */

bool timestamp_to_string(time_t timestamp, char* timestring){
    struct tm result;
    gmtime_r(&timestamp, &result);
    strftime(timestring, TIMESTAMPSTRINGLENGTH, "%Y-%m-%dT%H:%M:%SZ", &result);
    return true;
}

bool check_conditions(char* string){
    return (strlen(string) <= 482) && (strchr(string, '\n') == NULL);
}

/* -----------------------------------------------------------
 * Internal helpers – not part of the public header interface
 * ----------------------------------------------------------- */

static int log_compare(const void* a, const void* b){
    const Log* la = *(const Log* const*)a;
    const Log* lb = *(const Log* const*)b;

    if (la->timestamp < lb->timestamp)
        return -1;
    if (la->timestamp > lb->timestamp)
        return 1;
    if (la->level < lb->level)
        return -1;
    if (la->level > lb->level)
        return 1;
    return 0;
}

static void free_log(Log* log){
    if (!log)
        return;
    free(log->msg);
    free(log);
}

char* level_to_string(Level level){
    switch(level){
        case INFO:    return "INFO";
        case WARNING: return "WARN";
        case ERROR:   return "ERRO";
        case DEBUG:   return "DEBG";
        default:      return "UNKN";
    }
}

void to_string(Log* log, char* string){
    if (!log || !string)
        return;
    char ts[TIMESTAMPSTRINGLENGTH];
    timestamp_to_string(log->timestamp, ts);
    snprintf(string, MAX_LINE_LENGTH, "%s: %s - %s\n", ts,
             level_to_string(log->level), log->msg);
}

Log* create_log(time_t timestamp, Level level, char* msg){
    if (!msg)
        return NULL;

    Log* l = malloc(sizeof(Log));
    if (!l)
        return NULL;

    l->timestamp = timestamp;
    l->level = level;
    l->msg = strdup(msg);
    if (!l->msg){
        free(l);
        return NULL;
    }
    return l;
}

/* ------------------------------------------------
 * Public API implementation
 * ------------------------------------------------ */

LoggingInfo* init_LoggingInfo(char* path){
    if (!path)
        return NULL;

    LoggingInfo* info = calloc(1, sizeof(LoggingInfo));
    if (!info)
        return NULL;

    info->log_filename = strdup(path);
    if (!info->log_filename){
        free(info);
        return NULL;
    }

    info->logs = calloc(BUFFERSIZE, sizeof(Log*));
    if (!info->logs){
        free(info->log_filename);
        free(info);
        return NULL;
    }

    sem_init(&info->log_mutex, 0, 1);
    sem_init(&info->writer_signal, 0, 0);

    if (!create_log_file(info->log_filename))
        goto fail;
    if (pthread_create(&info->writer, NULL, writer_thread_routine, info) != 0)
        goto fail;

    return info;

fail:
    sem_destroy(&info->log_mutex);
    sem_destroy(&info->writer_signal);
    free(info->logs);
    free(info->log_filename);
    free(info);
    return NULL;
}

void logging_cleanup(LoggingInfo* info){
    if (!info)
        return;

    flush_logBuffer(info);

    info->writer_end_signal = true;
    sem_post(&info->writer_signal);
    pthread_join(info->writer, NULL);

    for (int i = 0; i < BUFFERSIZE; ++i)
        free_log(info->logs[i]);

    sem_destroy(&info->log_mutex);
    sem_destroy(&info->writer_signal);
    free(info->logs);
    free(info->log_filename);
    free(info);
}

bool log_msg(LoggingInfo* info, time_t timestamp, Level level, char* msg){
    if (!info || !msg || !check_conditions(msg))
        return false;

    Log* entry = create_log(timestamp, level, msg);
    if (!entry)
        return false;

    sem_wait(&info->log_mutex);

    int idx = info->counter % BUFFERSIZE;
    free_log(info->logs[idx]);
    info->logs[idx] = entry;
    ++info->counter;

    int filled = info->counter < BUFFERSIZE ? info->counter : BUFFERSIZE;
    bool want_flush = (filled >= (int)(0.8 * BUFFERSIZE)) || (level == ERROR);

    if (want_flush && !info->write_flag){
        info->write_flag = true;
        sem_post(&info->writer_signal);
    }

    sem_post(&info->log_mutex);
    return true;
}

void* writer_thread_routine(void* arg){
    LoggingInfo* info = (LoggingInfo*)arg;

    while (1){
        sem_wait(&info->writer_signal);
        if (info->writer_end_signal)
            break;
        flush_logBuffer(info);
    }

    return NULL;
}

bool create_log_file(char* filename){
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return false;
    fclose(fp);
    return true;
}

static int line_compare(const void* a, const void* b){
    const char* la = *(const char* const*)a;
    const char* lb = *(const char* const*)b;
    return strcmp(la, lb);
}

bool flush_logBuffer(LoggingInfo* info){
    if (!info)
        return false;

    sem_wait(&info->log_mutex);

    int buf_count = info->counter < BUFFERSIZE ? info->counter : BUFFERSIZE;
    if (buf_count == 0){
        info->write_flag = false;
        sem_post(&info->log_mutex);
        return true;
    }

    FILE* fp_in = fopen(info->log_filename, "r");
    size_t cap_lines = buf_count + 32;
    char** lines = malloc(cap_lines * sizeof(char*));
    size_t total = 0;

    if (fp_in){
        char tmp[MAX_LINE_LENGTH];
        while (fgets(tmp, sizeof(tmp), fp_in)){
            if (total == cap_lines){
                cap_lines *= 2;
                lines = realloc(lines, cap_lines * sizeof(char*));
            }
            lines[total++] = strdup(tmp);
        }
        fclose(fp_in);
    }

    char line[MAX_LINE_LENGTH];
    for (int i = 0; i < buf_count; ++i){
        if (total == cap_lines){
            cap_lines *= 2;
            lines = realloc(lines, cap_lines * sizeof(char*));
        }
        to_string(info->logs[i], line);
        lines[total++] = strdup(line);
    }

    qsort(lines, total, sizeof(char*), line_compare);

    FILE* fp_out = fopen(info->log_filename, "w");
    if (!fp_out){
        for (size_t i = 0; i < total; ++i)
            free(lines[i]);
        free(lines);
        sem_post(&info->log_mutex);
        return false;
    }

    for (size_t i = 0; i < total; ++i){
        fputs(lines[i], fp_out);
        free(lines[i]);
    }
    free(lines);
    fclose(fp_out);

    for (int i = 0; i < buf_count; ++i){
        free_log(info->logs[i]);
        info->logs[i] = NULL;
    }

    info->counter = 0;
    info->write_flag = false;

    sem_post(&info->log_mutex);
    return true;
}
