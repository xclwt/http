//
// Created by xclwt on 2021/2/8.
//
#include "webServer.h"

int main(void){
    WebServer server(
            1316, 3, 120 * TIMESLOT, true,             /* 端口 ET模式 timeout 优雅退出  */
            8, true, 0, 1024);             /* 线程池数量 日志开关 日志等级 日志异步队列容量 */

    server.eventLoop();
}

