/*======================================================
    > File Name: Codec.h
    > Author: MiaoShuai
    > E-mail:  
    > Other :  
    > Created Time: 2015年11月23日 星期一 15时07分48秒
 =======================================================*/

#ifndef CHAT_CODEC_H
#define CHAT_CODEC_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>

#include <functional>
#include <boost/noncopyable.hpp>

class LengthHeaderCodec : boost::noncopyable
{
    public:
        typedef std::function<void(const muduo::net::TcpConnectionPtr &,const muduo::string &message,muduo::Timestamp)> StringMessageCallback;
        explicit LengthHeaderCodec(const StringMessageCallback &cb)
            :messageCallback_(cb)
        {}

        //解包
        void onMessage(const muduo::net::TcpConnectionPtr &conn,
                muduo::net::Buffer *buf,
                muduo::Timestamp receiveTime)
        {
            while(buf->readableBytes() >= kHeaderLen)
            {
                const void *data = buf->peek();
                int32_t be32 = *static_cast<const int32_t *>(data);
                const int32_t len = muduo::net::sockets::networkToHost32(be32);
                if(len > 65536 || len < 0)
                {
                    LOG_ERROR << "Invalid length" << len;
                    conn->shutdown();
                    break;
                }

                else if(buf->readableBytes() >= len + kHeaderLen)//可以读取出完整的消息
                {
                    buf->retrieve(kHeaderLen);          //可读位置重置
                    muduo::string message(buf->peek(),len);
                    messageCallback_(conn,message,receiveTime);
                    buf->retrieve(len);                 //可读位置重置

                }
                else
                {
                    break;
                }
            }
        }

        //打包发送
        void send(muduo::net::TcpConnection *conn,const muduo::StringPiece &message)
        {
            muduo::net::Buffer buf;
            buf.append(message.data(),message.size());//将待发送内容放入buffer中
            int32_t len = static_cast<int32_t>(message.size());
            int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
            buf.prepend(&be32,sizeof(be32));    //利用buffer的预留空间直接将4字节的内容长度放在内容之前
            conn->send(&buf);
        }
    private:
        StringMessageCallback messageCallback_;
        const static size_t kHeaderLen = sizeof(int32_t);
};



#endif
