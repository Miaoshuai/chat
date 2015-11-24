/*======================================================
    > File Name: client.cpp
    > Author: MiaoShuai
    > E-mail:  
    > Other :  
    > Created Time: 2015年11月24日 星期二 08时23分47秒
 =======================================================*/

#include "Codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpClient.h>

#include <functional>
#include <boost/noncopyable.hpp>

#include <iostream>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

class ChatClient : boost::noncopyable
{
    public:
        ChatClient(EventLoop *loop,const InetAddress &serverAddr)
            :client_(loop,serverAddr,"ChatClient"),
            codec_(std::bind(&ChatClient::onStringMessage,this,_1,_2,_3))
    {
        client_.setConnectionCallback(std::bind(&ChatClient::onConnection,this,_1));
        client_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage,&codec,_1,_2,_3));
        client_.enableRetry();
    }

        void connect()
        {
            client_.connect();
        }

        void disconnect()
        {
            client_.disconnect();
        }

        void write(const StringPiece& message)
        {
            MutexLockGuard lock(mutex_);
            if(connection_)
            {
                codec_.send(get_pointer(connection_),message);
            }
        }
    private:
        //建立连接或断开时的回调
        void onConnection(const TcpConnectionPtr &conn)
        {
            MutexLockGuard lock(mutex_);
            if(conn->connected())
            {
                connection_ = conn;
            }
            else
            {
                connection_.reset();
            }
        }

        //收到消息后的回调
        void onStringMessage(const TcpConnectionPtr &,const string &message,Timestamp)
        {
            printf("<<<%s\n",message.c_str());
        }

        TcpClient client_;
        LengthHeaderCodec codec_;
        MutexLock mutex_;
        TcpConnectionPtr connection_;
};


int main(int argc,char **argv)
{
  LOG_INFO << "pid = " <<getpid();
  if(argc > 2)
  {
    EventLoopThread loopThread;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(argv[1],port);

    ChatClient client(loopThread.startLoop(),serverAddr);
    client.connect();
    std::string line;
    while(std::getline(std::cin,line))
    {
        client.write(line);
    }
    client.disconnect();
    CurrentThread::sleepUsec(1000*1000);
  }
  else
  {
    printf("Usage:%s host_ip port\n",argv[0]);
  }
  return 0;
}
