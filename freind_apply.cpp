#include "server.hpp"

void friend_apply_agree(StickyPacket socket,Message msg){
    if(!redis.Hexists(msg.uid+"收到好友的申请",msg.friend_uid)){
        socket.mysend("no_recv_apply");
        return;
    }

    else if(redis.Hdel(msg.uid+"收到好友的申请",msg.friend_uid)){
        string num1=redis.Hget(msg.uid+"的未读消息","好友申请");
        redis.hset(msg.uid+"的未读消息","好友申请",(to_string(stoi(num)-1)));

        redis.hset(msg.uid+"的好友列表",msg.friend_uid,"ok");
        redis.hset(msg.friend_uid+"的好友列表",msg.uid,"ok");

        redis.Rpush(msg.uid+"与"+msg.friend_uid+"的聊天记录","---------");
        redis.Rpush(msg.friend_uid+"与"+msg.uid+"的聊天记录","---------");
        redis.Rpush(msg.friend_uid+"与"+msg.uid+"的聊天记录",msg.uid+"通过了你的好友申请");

        string num2=redis.Hget(msg.friend_uid+"的未读消息","通知类消息");
        redis.hset(msg.friend_uid+"的未读消息","通知类消息",(to_string(stoi(num2)+1)));

        if(online_users.find(msg.friend_uid) != online_users.end()){
            string friend_fd = redis.Hget(msg.friend_uid,"实时socket");
            StickyPacket friendsocket (stoi(friend_fd));
            friendsocket.mysend(msg.uid+"通过了你的好友申请");
        }

        socket.mysend("success");
        return;


    }
    
}

void friend_apply_refuse(StickyPacket socket,Message msg){
    if(!redis.Hexists(msg.uid+"收到好友的申请",msg.friend_uid)){
        socket.mysend("no_recv_apply");
        return;
    }

    else if(redis.Hdel(msg.uid+"收到好友的申请",msg.friend_uid)){
        string num1=redis.Hget(msg.uid+"的未读消息","好友申请");
        redis.hset(msg.uid+"的未读消息","好友申请",(to_string(stoi(num)-1)));

        //redis.hset(msg.uid+"的好友列表",msg.friend_uid,"ok");
        //redis.hset(msg.friend_uid+"的好友列表",msg.uid,"ok");

        //redis.Rpush(msg.uid+"与"+msg.friend_uid+"的聊天记录","---------");
        //redis.Rpush(msg.friend_uid+"与"+msg.uid+"的聊天记录","---------");
        //redis.Rpush(msg.friend_uid+"与"+msg.uid+"的聊天记录",msg.uid+"通过了你的好友申请");

        string num2=redis.Hget(msg.friend_uid+"的未读消息","通知类消息");
        redis.hset(msg.friend_uid+"的未读消息","通知类消息",(to_string(stoi(num2)+1)));
        redis.Rpush(msg.friend_uid+"的通知消息",msg.uid+"拒绝了你的好友申请");

        if(online_users.find(msg.friend_uid) != online_users.end()){
            string friend_fd = redis.Hget(msg.friend_uid,"实时socket");
            StickyPacket friendsocket (stoi(friend_fd));
            friendsocket.mysend(msg.uid+"拒绝了你的好友申请");
        }

        socket.mysend("success");
        return;


    }
    
}