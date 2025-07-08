#ifndef _MessageTrans_H_
#define _MessageTrans_H_
#include <server.hpp>

class MessageTrans
{
public:
    void translation(StickyPacket socket,string cmd){
        Message msg;
        msg.Json_to_s(cmd);
        switch (msg.flag)
        {
        case SIGNUP:
            sign_up(socket,msg);
            break;
        case LOGIN:
            log_in(socket,msg);
            break;
        case QUESTION_GET:
            question_get(socket,msg);
            break;
        case ANSWER_GET:
            answer_get(socket,msg);
            break;
        case PASS_GET:
            pass_get(socket,msg);
            break;
        case USER_DEL:
            user_del(socket,msg);
            break;
        case UNREAD_MSG:
            unread_msg(socket,msg);
            break;
        default:
            break;
        }

    }
};


#endif