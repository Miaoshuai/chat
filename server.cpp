/*======================================================
    > File Name: server.cpp
    > Author: MiaoShuai
    > E-mail:  
    > Other :  
    > Created Time: 2015年11月23日 星期一 17时16分13秒
 =======================================================*/

#include "Codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <functional>
#include <set>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer : boost::noncopyable
{
    public:
        ChatServer(EventLoop *loop,const InetAddress &listenAddr)
        :server_(loop,listenAddr,"ChatServer"),
        codec_(std::bind(&ChatServer::onStringMessage,this,_1,_2,_3))
        {
            server_.setConnectionCallback(std::bind(&ChatServer::onStringMessage,this,_1));
            server_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage,&codec_,_1,_2,_3));

            void start()
            {
                server_.start();
            }
        }    
    private:
        //新连接或断开时的回调
        void onConnection(const TcpConnectionPtr &conn)
        {
            LOG_INFO <<conn->localAddress().toIpPort() << "->"
                     <<conn->peerAddress().toIpPort() << "is"
                     <<(conn->connected() ? "UP" : "DOWN");

            if(conn->connected())
            {
                //插入到用户集合中
                connections_.insert(conn);
            }
            else
            {
                //从集合中删除用户
                connections_.erase(conn);
            }
        }

        void onStringMessage(const TcpConnectionPtr &,const string &message,Timestamp)
        {
            //给每个用户发消息
            for(ConnectionList::iterator it = connections_.begin();
                    it != connections_.end(); ++it)
            {
                codec_.send(get_pointer(*it),message);
            }
        }

        typedef std::set<TcpConnectionPtr> ConnectionList;
        TcpServer server_;
        LengthHeaderCodec codec_;
        ConnectionList connections_;
               
};

int main(int argc,char **argv)
{
  LOG_INFO << "pid = " << getpid();
  if(argc > 2)
  {
      EventLoop loop;
      InetAddress serverAddr(argv[1],static<uint16_t>(atoi(argv[2])));
      ChatServer server(&loop,serverAddr);
      server.start();
      loop.loop();
  }
  else
  {
      printf("您输入的参数有误\n");
  }
  return 0;
}
