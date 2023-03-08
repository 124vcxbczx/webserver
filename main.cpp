/*
 *
 */

//!

//main

#include <iostream>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>

#include<string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "locker.h"
#include "threadpool.h"
#include "http_conn.h"
#include "timer.h"
using namespace std;

#define MAX_FD 65535          //最大的文件描述符个数
#define MAX_EVENT_NUMBER 100000  //监听的最大事件数量

//信号处理 添加信号 处理的信号sig 操作handler
void addsig(int sig, void (handler)(int)){
    struct sigaction sa;
    memset(&sa,'\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

//添加文件描述符到epoll对象
extern int addfd(int epollfd, int fd, bool one_shot);
//从epoll中删除文件描述符
extern int removefd(int epollfd, int fd, bool one_shot);
//修改文件描述符
extern void modfd(int epollfd, int fd, int ev);

void cb(http_conn* user_Data){
    user_Data->close_conn();
}

int main(int argc, char *argv[]) {
    //! 判断参数是否正确
    if(argc <= 1){
        cout<<"参数不够"<<endl;
    }

    //获取端口号 字符串转整数
    int port = atoi(argv[1]);

    //对SIGPIE信号（）进行处理
    //当服务器close一个连接时，若client端接着发数据。根据TCP协议的规定，会收到一个RST响应，
    // client再往这个服务器发送数据时，系统会发出一个SIGPIPE信号给进程，告诉进程这个连接已经断开了，不要再写了。
    addsig(SIGPIPE, SIG_IGN);

    //创建线程池，初始化线程池 // TODO: sikao
    threadpool<http_conn> *pool = NULL; // FIXME:
    try{
        pool = new threadpool<http_conn>;   // fenpei
    }catch(...){
        exit(-1);
    }

    //创建一个数组用于保存所有的客户端信息
    http_conn* users = new http_conn[MAX_FD];
    //写网络的代码
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);

    struct linger optLinger = { 0 };
    optLinger.l_onoff = 1;
    optLinger.l_linger = 1;

    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    int reuse = 1;
    //设置端口复用 要在绑定之前设置
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    bind(listenfd, (struct sockaddr*)&address, sizeof(address));

    listen(listenfd, 5);

    //创建epoll对象，事件数组，添加
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    //将监听的文件描述符添加到epoll对象中
    addfd(epollfd, listenfd, false);
    int timeoutMS = 20000;
    int timeMS = -1;
    HeapTimer* timer = new HeapTimer();
    //! zhixing
    http_conn::m_epollfd = epollfd;
    while(1){
        //监听 默认阻塞
        cout<<"+++++++++++++++++++++++++++++++++++"<<endl;
        if(timeoutMS > 0){
            timeMS = timer->GetNextTick();
        }
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, timeoutMS); //返回检测到了几个事件
        if(num < 0 && errno != EINTR) {
            cout<<"epoll failure"<<endl;
            break;
        }

        for(int i = 0; i < num; i++){
            int sockfd = events[i].data.fd;
            if(sockfd  == listenfd){
                //有客户端连接来
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlen);

                //目前连接数已经满了
                if(http_conn::m_user_count >= MAX_FD){
                    close(connfd);
                    continue;
                }

                //将连接放入数组中 users 将文件描述符作为索引
                users[connfd].init(connfd, client_address);
                timer->add(connfd, timeoutMS, cb, &users[connfd]);
                cout<<"建立连接："<<connfd<<endl;

            }
            //对方异常断开等错误事件
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                users[sockfd].close_conn();
            }
            //du
            else if(events[i].events & EPOLLIN){
                cout<<"接收HTTP请求："<<sockfd<<endl;
                if(users[sockfd].read()){
                    cout<<"接收成功，调整计时器时间"<<sockfd<<endl;
                    timer->adjust(sockfd, timeoutMS);
                    //一次性读完 交给工作线程处理
                    pool->append(users+sockfd);
                }
                else{
                    users[sockfd].close_conn();
                }
            }
            else if(events[i].events & EPOLLOUT){
                cout<<"回复HTTP请求："<<sockfd<<endl;
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }
        }

    }
    close(epollfd);
    close(listenfd);
    delete []users;

    return 0;

}
