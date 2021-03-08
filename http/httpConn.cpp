//
// Created by xclwt on 2021/2/11.
//

#include "httpConn.h"

HttpConn::HttpConn(): m_fd(-1), m_addr({0}), m_isClose(true){}

HttpConn::~HttpConn(){
    unmapFile();
    if (!m_isClose){
        m_isClose = true;
        --m_user_cnt;
        close(m_fd);
        LOG_INFO("client[%d](%s: %d) terminated!, user count: %d", m_fd, getIP(), getPort(), m_user_cnt);
    }
}

void HttpConn::init(int sockfd, sockaddr_in addr, int timeout){
    ++m_user_cnt;
    m_fd = sockfd;
    m_addr = addr;
    m_timeout = timeout;
    m_readbuf_r_idx = 0;
    m_readbuf_w_idx = 0;
    m_writebuf_r_idx = 0;
    m_writebuf_w_idx = 0;
    m_isClose = false;
}

int HttpConn::read(int &saveErrno){
    int ret;

    do{
        ret = recv(m_fd, m_read_buf + m_readbuf_w_idx, READ_BUF_SIZE - m_readbuf_w_idx, 0);

        if (ret == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                saveErrno = errno;
            }
            break;
        }else if(ret == 0){
            return 0;
        }

        m_readbuf_w_idx += ret > 0 ? ret : 0;
    } while (m_ET);

    return ret;
}

int HttpConn::write(int &saveErrno){
    int ret;
    do {
        ret = writev(m_fd, m_iov, m_iov_cnt);

        if(ret <= 0) {
            saveErrno = errno;
            break;
        }

        if(m_iov[0].iov_len + m_iov[1].iov_len  == 0){
            break;
        }else if(ret > m_iov[0].iov_len){
            m_iov[1].iov_base = (char*)(m_iov[1].iov_base) + (ret - m_iov[0].iov_len);
            m_iov[1].iov_len -= (ret - m_iov[0].iov_len);

            if(m_iov[0].iov_len) {
                bzero(m_write_buf, m_writebuf_w_idx);
                m_readbuf_r_idx = 0;
                m_readbuf_w_idx = 0;
                m_iov[0].iov_len = 0;
            }
        }else{
            m_iov[0].iov_base = (char*)(m_iov[0].iov_base) + ret;
            m_iov[0].iov_len -= ret;
            m_writebuf_r_idx += ret;
        }
    } while(m_ET);

    return ret;
}

int HttpConn::bytesToWrite(){
    return m_writebuf_w_idx - m_writebuf_r_idx;
}

bool HttpConn::process(){
    requestInit();
    if (readBufReadable() <= 0){
        return false;
    }else if (parse()){
        responseInit();
    }else{
        return false;
    }

    makeResponse();

    m_iov[0].iov_base = m_write_buf + m_writebuf_r_idx;
    m_iov[0].iov_len = writeBufReadable();
    m_iov_cnt = 1;

    if (m_fileStat.st_size > 0 && m_file){
        m_iov[1].iov_base = m_file;
        m_iov[1].iov_len = m_fileStat.st_size;
        m_iov_cnt = 2;
    }

    LOG_DEBUG("filesize: %d, total bytes: %d", m_fileStat.st_size, m_iov[0].iov_len + m_iov[1].iov_len);
    return true;
}

void HttpConn::closeClient(){
    if(!m_isClose){
        m_isClose = true;
        close(m_fd);
        --m_user_cnt;
        LOG_INFO("client[%d](%s: %d) disconnect!, user count: %d", m_fd, getIP(), getPort(), m_user_cnt);
    }
}

int HttpConn::getFd() const{
    return m_fd;
}

int HttpConn::getPort() const{
    return ntohs(m_addr.sin_port);
}

const char * HttpConn::getIP(){
    return inet_ntop(AF_INET, &m_addr.sin_addr, m_addr_buf, INET_ADDRSTRLEN);
}

int HttpConn::readBufReadable(){
    return m_readbuf_w_idx - m_readbuf_r_idx;
}

int HttpConn::writeBufReadable(){
    return m_writebuf_w_idx - m_writebuf_r_idx;
}

void HttpConn::requestInit(){
    m_method = m_path = m_version = "";
    m_state = REQUESTLINE;
    m_header.clear();
    m_content.clear();
}

bool HttpConn::parse(){
    LOG_DEBUG("Connection %d enter parse", getFd());
    const char CRLF[] = "\r\n";
    string line;

    if(!readBufReadable()){
        return false;
    }

    while (readBufReadable() && m_state != FINISH){
        LOG_DEBUG("Connection %d enter parse loop", getFd());
        const char *lineEnd = search(m_read_buf + m_readbuf_r_idx,
                                     m_read_buf + m_readbuf_w_idx, CRLF, CRLF + 2);

        if (m_state != CONTENT){
            if (m_state != ERROR && lineEnd == m_read_buf + m_readbuf_w_idx)
                break;

            line = string(m_read_buf + m_readbuf_r_idx, lineEnd - (m_read_buf + m_readbuf_r_idx));
            m_readbuf_r_idx += (lineEnd + 2 - m_read_buf) + m_readbuf_r_idx;
        }

        LOG_DEBUG("Connection %d enter state machine", getFd());
        switch (m_state){
            case REQUESTLINE:
                if (!parseRequestLine(line)){
                    m_responseCode = 400;
                    return true;
                }

                parsePath();
                break;
            case HEADERS:
                parseHeader(line);
                if (readBufReadable() <= 2)
                    m_state = FINISH;
                break;
            case CONTENT:
                if (stoi(m_header.find("content-length")->second) <= readBufReadable()){
                    line = string(m_read_buf + m_readbuf_r_idx, stoi(m_header.find("content-length")->second));
                }else{
                    return false;
                }

                parseContent(line);
                m_readbuf_r_idx += (lineEnd - m_read_buf) + m_readbuf_r_idx;
                break;
            default:
                m_responseCode = 400;
                return true;
        }
    }

    LOG_DEBUG("parse:[%s], [%s], [%s]", m_method.c_str(), m_path.c_str(), m_version.c_str());
    m_responseCode = 200;
    return true;
}

bool HttpConn::parseRequestLine(const string &line){
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;

    if(regex_match(line, subMatch, patten)) {
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_version = subMatch[3];
        m_state = HEADERS;
        return true;
    }

    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpConn::parseHeader(const string &line){
    LOG_DEBUG("Connection %d enter parse reuqest line", getFd());
    regex pattern("^([^:]*): *(.*)$");
    smatch subMatch;

    if (regex_match(line, subMatch, pattern))
        m_header[subMatch[1]] = subMatch[2];
    else if (m_header.count("content-length") == 1 && m_method == "POST"){
        m_state = CONTENT;
    }else{
        m_state = ERROR;
    }

}

void HttpConn::parseContent(const string &line){
    LOG_DEBUG("Connection %d enter parse content", getFd());
}

void HttpConn::parsePath(){
    LOG_DEBUG("Connection %d enter parse path", getFd());
}

bool HttpConn::isKeepAlive(){
    if(m_header.count("Connection") == 1){
        return m_header.find("Connection")->second == "keep-alive" && m_version == "1.1";
    }

    return false;
}

void HttpConn::writeBufAppend(const string &str){
    strcpy(m_write_buf + m_writebuf_w_idx, str.c_str());
    m_writebuf_w_idx += str.length();
}

void HttpConn::responseInit(){
    unmapFile();
    m_isKeepAlive = isKeepAlive();
    m_fileStat = {0};
}

void HttpConn::makeResponse(){
    if(stat((m_rootDir + m_path).c_str(), &m_fileStat) < 0 || S_ISDIR(m_fileStat.st_mode)){
        m_responseCode = 404;
    }else if (!(m_fileStat.st_mode & S_IROTH)){
        m_responseCode = 403;
    }

    if (m_responseCode != 200){
        errorInfo();
    }

    addStateLine();
    addHeader();
    addContent();
}

void HttpConn::errorInfo(){
    m_path = CODE_PATH.find(m_responseCode)->second;
    stat((m_rootDir + m_path).c_str(), &m_fileStat);
}

void HttpConn::addStateLine(){
    string status = CODE_STATUS.find(m_responseCode)->second;
    string stateLine = "HTTP/1.1 " + to_string(m_responseCode) + " " + status + "\r\n";
    writeBufAppend(stateLine);
}

void HttpConn::addHeader(){
    writeBufAppend("Connection: ");

    if (m_isKeepAlive){
        writeBufAppend("keep-alive\r\n");
        writeBufAppend("keep-alive: max = 1000, timeout = " + to_string(m_timeout));
    }else{
        writeBufAppend("close\r\n");
    }

    writeBufAppend("Content-type: " + fileType() + "\r\n");
}

void HttpConn::addContent(){
    int fd = open((m_rootDir + m_path).c_str(), O_RDONLY);

    if (fd < 0){
        LOG_DEBUG("Connection %d File Not Found", getFd());
        addError("File Not Found");
        return;
    }

    m_file = (char *)mmap(0, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (m_file == MAP_FAILED){
        LOG_DEBUG("Connection %d File Not Found", getFd());
        addError("File Not Found");
        return;
    }

    close(fd);
    writeBufAppend("Content-length: " + to_string(m_fileStat.st_size) + "\r\n\r\n");
}

void HttpConn::addError(string msg){
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(m_responseCode) == 1) {
        status = CODE_STATUS.find(m_responseCode)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(m_responseCode) + " : " + status  + "\n";
    body += "<p>" + msg + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    writeBufAppend("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    writeBufAppend(body);
}

string HttpConn::fileType(){
    string::size_type idx = m_path.find_last_of('.');

    if (idx == string::npos){
        return "text/plain";
    }

    string suffix = m_path.substr(idx);

    if (SUFFIX_TYPE.count(suffix) == 1){
        return SUFFIX_TYPE.find(suffix)->second;
    }

    return "text/plain";
}

void HttpConn::unmapFile(){
    if(m_file) {
        munmap(m_file, m_fileStat.st_size);
        m_file = nullptr;
    }
}

bool HttpConn::m_ET;
const char* HttpConn::m_rootDir;
int HttpConn::m_user_cnt;
