//
// Created by xclwt on 2021/2/11.
//

#ifndef HTTP_HTTPCONN_H
#define HTTP_HTTPCONN_H

#include <arpa/inet.h>
#include <sys/uio.h>

#define READ_BUF_SIZE 4096
#define WRITE_BUF_SIZE 4096

class HttpConn{
public:
    HttpConn();

    ~HttpConn();

    void init(int sockfd, const sockaddr_in& addr);

    int read(int* saveErrno);

    int write(int* saveErrno);

    bool process();

    void close();

    int getFd() const;

    int getPort() const;

    const char* getIP() const;

    sockaddr_in getAddr() const;

    //static int m_max_users;
    static const char* m_rootDir;
    static int m_user_cnt;

private:
    int m_fd;
    sockaddr_in m_addr;

    iovec m_iv[2];
    int m_iv_cnt;

    char m_read_buf[READ_BUF_SIZE];
    char m_write_buf[WRITE_BUF_SIZE];
};

#endif //HTTP_HTTPCONN_H
