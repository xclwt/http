//
// Created by xclwt on 2021/1/30.
//

#include "log.h"

Log::Log(){
    m_log_open = false;
    m_log_idx = 0;
    m_isAsync = false;
    m_fd = -1;
    m_logQueue = nullptr;
}

Log::~Log(){
    if (m_fd != -1){
        close(m_fd);
    }

    m_log_open = false;
}

void Log::openNewLog(int oflag, const char *format, ...){
    va_list args;
    char entireName[LOG_NAME_LEN];

    va_start(args, format);
    vsnprintf(entireName, LOG_NAME_LEN - 1, format, args);
    va_end(args);

    if (m_fd != -1)
        close(m_fd);

    m_fd = open(entireName, oflag);
    if (m_fd == -1){
        mkdir(m_filepath, 0777);
        m_fd = open(entireName, oflag);
    }
}

[[noreturn]] void Log::asyncWrite(){
    string oneLog;
    while (true){
        m_logQueue->pop(oneLog);
        lock_guard<mutex> locker(m_mtx);\
        write(m_fd, oneLog.c_str(), oneLog.size());
    }
}

void Log::init(int level, int maxLineNum, int maxQueueSize, int logBufSize, const char *filepath, const char *suffix){
    m_level = level;
    m_line_cnt = maxLineNum;
    m_max_line_num = maxLineNum;
    m_log_open = true;
    m_logBufSize = logBufSize;
    m_logBuffer = new char[logBufSize];

    if (maxQueueSize > 0 && !m_isAsync){
        m_isAsync = true;
        m_logQueue = unique_ptr<BlockQueue<string>>(new BlockQueue<string>(maxQueueSize));
        pthread_create(&m_async_write_thread, nullptr, logWriteThread, nullptr);
    }

    time_t tloc = time(nullptr);
    struct tm time = *localtime(&tloc);

    m_filepath = filepath;
    m_suffix = suffix;

    m_today = time.tm_mday;
}

int Log::getLevel(){
    lock_guard<mutex> locker(m_mtx);
    return m_level;
}

void Log::setLevel(int level){
    lock_guard<mutex> locker(m_mtx);
    m_level = level;
}

void Log::writeLog(int level, const char *format, ...){
    time_t tloc = time(nullptr);
    struct tm time = *localtime(&tloc);
    va_list args;

    unique_lock<mutex> locker(m_mtx);
    if(time.tm_mday != m_today || m_line_cnt == m_max_line_num){
        if (time.tm_mday != m_today){
            m_log_idx = 0;
            openNewLog(O_APPEND | O_CREAT | O_RDWR, "%s/%04d%02d%02d-%d%s",
                       m_filepath, time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, m_log_idx++, m_suffix);
        }else{
            openNewLog(O_APPEND | O_CREAT | O_RDWR, "%s/%04d%02d%02d-%d%s",
                       m_filepath, time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, m_log_idx++, m_suffix);
        }

        m_line_cnt = 0;
    }

    ++m_line_cnt;

    int n = snprintf(m_logBuffer, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                     time.tm_hour, time.tm_min, time.tm_sec, tloc, levelInfo[level]);

    va_start(args, format);
    int m = vsnprintf(m_logBuffer + n, m_logBufSize - n, format, args);
    va_end(args);

    m_logBuffer[n + m] = '\n';
    m_logBuffer[n + m + 1] = '\0';
    string log_str(m_logBuffer);

    locker.unlock();

    if (m_isAsync && !m_logQueue->full())
    {
        m_logQueue->push(log_str);
    }
    else
    {
        locker.lock();
        write(m_fd, log_str.c_str(), log_str.size());
        locker.unlock();
    }
}

bool Log::isOpen() const{
    return m_log_open;
}

Log* Log::getInstance(){
    static Log log_inst;
    return &log_inst;
}

void* Log::logWriteThread(void *args){
    Log::getInstance()->asyncWrite();
}
