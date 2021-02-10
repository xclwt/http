//
// Created by xclwt on 2021/1/30.
//

#ifndef HTTP_LOG_H
#define HTTP_LOG_H

#include <mutex>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdarg>
#include <memory>
#include "queue.h"

#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_WARNING 2
#define LOG_ERROR   3



//#define LOG_ASYNC   true
//#define LOG_SYNC    false

using namespace std;

class Log{
public:
    void init(int level, int maxLineNum, int maxQueueSize, int logBufSize, const char *filepath, const char *filename);

    int getLevel();

    void setLevel(int level);

    void writeLog(int level, const char *format, ...);

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
    int m_max_queue_size;
    int m_logBufSize;
    int m_line_cnt;     //current line count
    int m_level;        //current log level
    int m_log_idx;
    int m_today;
    unique_ptr<BlockQueue<string>> m_logQueue;
    pthread_t m_async_write_thread;
    bool m_isAsync;
    bool m_log_closed;  //log closed or not
    mutex m_mtx;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif
