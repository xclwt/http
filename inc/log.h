//
// Created by xclwt on 2021/1/30.
//

#ifndef HTTP_LOG_H
#define HTTP_LOG_H

#include <mutex>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdarg>
#include <memory>
#include <string>
#include "queue.h"

#define LEVEL_DEBUG   0
#define LEVEL_INFO    1
#define LEVEL_WARN    2
#define LEVEL_ERROR   3

#define DEFAULT_LOG_BUF 8192
#define DEFAULT_MAX_LINE 1000000



//#define LOG_ASYNC   true
//#define LOG_SYNC    false

using namespace std;

class Log{
public:
    void init(int level, int maxLineNum, int maxQueueSize, int logBufSize, const char *filepath, const char *filename);

    int getLevel();

    void setLevel(int level);

    void writeLog(int level, const char *format, ...);

    bool isOpen() const;

    static Log *getInstance();

    static void *logWriteThread(void *args);

private:
    Log();

    ~Log();

    void openNewLog(int oflag, const char *format, ...);

    [[noreturn]] void asyncWrite();

    static const int LOG_NAME_LEN = 256;

    const char *levelInfo[4] = {"[DEBUG]","[INFO]","[WARN]","[ERROR]"};
    const char *m_filepath;
    const char *m_suffix;
    char *m_logBuffer;
    int m_fd;           //log fd
    int m_max_line_num;
    int m_logBufSize;
    int m_line_cnt;     //current line count
    int m_level;        //current log level
    int m_log_idx;
    int m_today;
    unique_ptr<BlockQueue<string>> m_logQueue;
    pthread_t m_async_write_thread;
    bool m_isAsync;
    bool m_log_open;  //log open or not
    mutex m_mtx;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::getInstance();\
        if (log->isOpen() && log->getLevel() <= level) {\
            log->writeLog(level, format, ##__VA_ARGS__); \
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(LEVEL_DEBUG, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(LEVEL_INFO, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(LEVEL_WARN, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(LEVEL_ERROR, format, ##__VA_ARGS__)} while(0);

#endif
