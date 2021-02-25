//
// Created by xclwt on 2021/2/11.
//

#ifndef HTTP_HTTPCONN_H
#define HTTP_HTTPCONN_H

#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include <string>
#include "log.h"

using namespace std;

#define READ_BUF_SIZE 4096
#define WRITE_BUF_SIZE 4096

enum CHECK_STATE{
    REQUESTLINE,
    HEADERS,
    CONTENT,
    FINISH,
    ERROR
};

enum HTTP_CODE{

};

static const unordered_map<string, string> SUFFIX_TYPE = {
        { ".html",  "text/html" },
        { ".xml",   "text/xml" },
        { ".xhtml", "application/xhtml+xml" },
        { ".txt",   "text/plain" },
        { ".rtf",   "application/rtf" },
        { ".pdf",   "application/pdf" },
        { ".word",  "application/nsword" },
        { ".png",   "image/png" },
        { ".gif",   "image/gif" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".au",    "audio/basic" },
        { ".mpeg",  "video/mpeg" },
        { ".mpg",   "video/mpeg" },
        { ".avi",   "video/x-msvideo" },
        { ".gz",    "application/x-gzip" },
        { ".tar",   "application/x-tar" },
        { ".css",   "text/css "},
        { ".js",    "text/javascript "},
};

static const unordered_map<int, string> CODE_STATUS = {
        { 200, "OK" },
        { 400, "Bad Request" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
};

static const unordered_map<int, string> CODE_PATH = {
        { 400, "/400.html" },
        { 403, "/403.html" },
        { 404, "/404.html" },
};



class HttpConn{
public:
    HttpConn();

    ~HttpConn();

    void init(int sockfd, sockaddr_in addr, int timeout);

    bool read();

    int write(int &saveErrno);

    int bytesToWrite();

    bool isKeepAlive();

    bool process();

    void closeClient();

    int getFd() const;

    int getPort() const;

    const char* getIP();

    static bool m_ET;
    static const char* m_rootDir;
    static int m_user_cnt;

private:
    int readBufReadable();

    int writeBufReadable();

    void requestInit();

    bool parse();

    bool parseRequestLine(const string& line);

    void parseHeader(const string& line);

    void parseContent(const string& line);

    void parsePath();

    void parsePost();

    void parseFromUrlencoded();

    void writeBufAppend(const string &str);

    void responseInit();

    void makeResponse();

    void errorInfo();

    void addStateLine();

    void addHeader();

    void addContent();

    void addError(string msg);

    void unmapFile();

    string fileType();


    int m_fd;
    sockaddr_in m_addr;
    int m_timeout;
    bool m_isClose;
    bool m_isKeepAlive;

    iovec m_iov[2];
    int m_iov_cnt;

    int m_readbuf_r_idx;
    int m_readbuf_w_idx;
    int m_writebuf_r_idx;
    int m_writebuf_w_idx;

    char m_addr_buf[INET_ADDRSTRLEN];
    char m_read_buf[READ_BUF_SIZE];
    char m_write_buf[WRITE_BUF_SIZE];

    char *m_file = nullptr;
    struct stat m_fileStat;
    int m_responseCode;

    CHECK_STATE m_state;
    std::string m_method, m_path, m_version, m_content;
    std::unordered_map<std::string, std::string> m_header;
    std::unordered_map<std::string, std::string> m_post;
};

#endif //HTTP_HTTPCONN_H
