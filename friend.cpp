#include "server.hpp"
void friend_add(StickyPacket socket,Message msg){
    //判断是不是friend在不在uid集合
    if(!redis.sismember("uid集合",msg.friend_uid)){
        socket.mysend("该用户不存在");
        return;
    }

    //判断是不是friend在不在uid的好友列表
    int num=redis.Hlen(msg.uid,"的好友列表");
    if(num!=0){
        vector<string> friendlist =redis.Hgetall(msg.uid,"的好友列表");
        for(const string &friendID : friendlist){
            if(friendID==msg.friend_uid){
                socket.mysend("friend_exit");
                return;
            }
        }
    }

    if(redis.Hexists(msg.uid+"收到的好友申请",msg.friend_uid)){
        socket.mysend("receive_friend_apply");
        return;
    }

    if(redis.Hexists(msg.friend_uid+"收到的好友申请",msg.uid)){
        socket.mysend("have_send");
        return;
    }

    string apply ="来自" + msg.uid + "的好友申请："+ msg.para[0];
    redis.hset(msg.friend_uid+"收到的好友申请",msg.uid,apply);

    string count=redis.Hget(msg.friend_uid+"的未读消息","好友申请");
    redis.Hset(msg.friend_uid+"的未读消息","好友申请",to_string(stoi(count)+1));

    if(online_users.find(msg.friend_uid) != online_users.end()){
        string friend_fd =redis.hget(msg.friend_uid,"实时socket");
        StickyPacket friend_socket(stoi(friend_fd));
        string notice = "收到来自" + msg.uid + "的好友申请";
        friend_socket.mysend(RED + notice + RESET);
        
    }

    socket.mysend("success");


}

void friend_del(StickyPacket socket,Message msg){
    if(!redis.Hexists(msg.uid+"的好友列表",msg.friend_uid)){
        socket.mysend("friend_no_exist");
        return;
    }else if(redis.Hdel(msg.uid+"的好友列表",msg.friend_uid)){
        redis.Hdel(msg.friend_uid+"的好友列表",msg.uid);
        socket.mysend("success")
        return;
    }
}

void friend_list(StickyPacket socket,Message msg){
    if(!redis.Exists(msg.uid+"的好友列表")){
        socket.mysend("no_friend");
        return;
    }

    vector<string> friendlist =redis.Hgetall(msg.uid,"的好友列表");
    for(const string &friendID :friendlist){
        if(!redis.sismember(msg.uid+"的屏蔽列表",friendID)){
            if(online_users.find(friendID)!=online_users.end()){
                socket.mysend(YELLOW+friendID+RESET);
            }else{
                socket.mysend(friendID);
            }
        }
    }

    socket.mysend("over");
}


void friend_quit(StickyPacket socket,Message msg){
    if(!redis.Hexists(msg.uid+"的好友列表",msg.friend_uid)){
        socket.mysend("friend_no_exist");
        return;
    }

    if(redis.sadd(msg.uid+"的屏蔽列表",msg.friend_uid)){
        socket.mysend("success");
    }else{
        socket.mysend("fail");
    }
    return;
}
