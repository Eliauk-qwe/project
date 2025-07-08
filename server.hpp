#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "../ThreadPool.hpp"
#include <fcntl.h>
#include <../StickyPacket.hpp>
#include <../3.hpp>
#include <MessageTrans.hpp>
#include <string>
#include <unordered_set>
#include <vector>
#include <../Redis.hpp>


#define SIGNUP 1
#define LOGIN  2
#define QUESTION_GET 3
#define ANSWER_GET 4
#define PASS_GET 5
#define USER_DEL 6
#define UNREAD_MSG 7


void sign_up(StickyPacket socket,Message msg);
void log_in(StickyPacket socket,Message msg);
void question_get(StickyPacket socket,Message msg);
void answer_get(StickyPacket socket,Message msg);
void pass_get(StickyPacket socket,Message msg);
void user_del(StickyPacket socket,Message msg);
void unread_msg(StickyPacket socket,Message msg);
extern Redis redis;
unordered_set<string> online_users;





using namespace std;

#define LISTEN_NUM 150
#define MAX_EVENTS 1024

MessageTrans trans;

class User{
public:
   string UID, Name, Pass, Question, Answer,Phone;
   
   User(string name,string uid,string pass,string question,string answer,string phone ){
       this->Answer=answer;
       this->Name=name;
       this->Pass=pass;
       this->Phone=phone;
       this->Question=question;
       this->UID=uid;
   }

private:
   mutex user_mutex;
   
};

class 