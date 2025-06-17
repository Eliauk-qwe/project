#ifndef _REDIS_H_
#define _REDIS_H_

#include <hiredis/hiredis.h>
#include <iostream>
#include <string>

using namespace std;


class Redis{
private:
    redisContext  *con ;
    
           

public:
    Redis(){
        con=redisConnect("127.0.0.1",6379);
        if(con == nullptr){
            cout << "Can not allocate redis context" << endl;
        }
        else if(con -> err){
            cout << con->errstr <<endl;
            redisFree(con);
        }
        else{
            cont << "Connect to redis successfully!" << endl;
        }
    }

    ~Redis(){
        redisFree(con);
    }

    //集合SADD  在集合key中添加member元素
    int sadd (const string &key,const string &member){
        redisReply *reply = redisCommand(con,"SADD %s %s",key.c_str(),member.c_str());

        if(reply == nullptr){
            fprintf(stderr,"SADD fail\n");
        }
        else if(reply->type == REDIS_REPLY_ERROR){
            fprintf(stderr,"SADD错误 :%s",reply->str);
        }
        else if(reply->type ==REDIS_REPLY_INTEGER && reply->integer==1){
            printf("SADD成功: %s",reply->str);
            freeReplyObject(reply);
            return ture;
        }

        freeReplyObject(reply);
        return false;
    }

    //集合SCARD  获取集合key中元素个数
    int scard (const string &key){
        redisReply *reply = redisCommand(con,"SCARD %s",key.c_str());

        if(reply == nullptr){
            fprintf(stderr,"SCARD fail\n");
        }
        else if(reply->type == REDIS_REPLY_ERROR){
            fprintf(stderr,"SADD错误 :%s",reply->str);
        }
        else if(reply->type ==REDIS_REPLY_INTEGER ){
            printf("SADD成功: %s",reply->str);
            cout << key<<"中元素个数为"<< reply->integer;
            freeReplyObject(reply);
            return reply->integer;
        }

        freeReplyObject(reply);
        return reply->integer;
    }


    //哈希HSET  设置  1001 name wly |||| key field alue
    int hset (const string &key,const string &field,const string &value){
        redisReply *reply = redisCommand(con,"HSET %s %s",key.c_str(),field.c_str(),value.c_str());

        if(reply == nullptr){
            fprintf(stderr,"HSET fail\n");
        }
        else if(reply->type == REDIS_REPLY_ERROR){
            fprintf(stderr,"HSET错误 :%s",reply->str);
        }
        else if(reply->type ==REDIS_REPLY_INTEGER && reply->integer==1){
            printf("SADD成功: %s",reply->str);
            freeReplyObject(reply);
            return ture;
        }

        freeReplyObject(reply);
        return false;
    }




};



#endif