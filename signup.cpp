#include "client.hpp"

void sign_up(){
    string phone,pass,pass2;
    string question,answer;
    string name;
    printf("请输入电话号码：\n");
    getline(cin,phone);
    for(int i=5;i>0;i--){
        cout<<"请设置密码"<<endl;
        getline(cin,pass);
        cout<<"请重新输入密码"<<endl;
        getline(cin,pass2);
        if(pass!=pass2){
            cout<<"两次密码不一致，请重新输入"<<endl;
            cout<<"你还有"<<i-1<<"次机会"<<endl;
        }
        else   break;
    }

    cout<<"请输入密保问题"<<endl;
    getline(cin,question);
    cout<<"请输入回答"<<endl;
    getline(cin,answer);

    cout<<"请设置昵称"<<endl;
    getline(cin,name);

    Message msg(phone,pass,question,{answer},name,SIGNUP);
    socket_fd.mysend(msg.S_to_json());
    //错误处理

    string uid=socket_fd.client_recv();
    cout<<"你注册的uid为"<<uid<<endl;
    cout<<"接下来将进入登录界面"<<endl<<endl;


}

void log_in(){
    string phone,pass,uid;
    while(1){
        cout<<"请输入你的电话号码"<<endl;
        getline(cin,phone);
        cout<<"请输入你的密码"<<endl;
        getline(cin,pass);
        cout<<"请输入你的uid"<<endl;
        getline(cin,uid);

        Message msg(phone,pass,uid,LOGIN);
        socket_fd.mysend(msg.S_to_json());

        string recv=socket_fd.client_recv();
        if(recv=="该用户未注册"){
            cout<<"你还未注册，请先注册"<<endl;
        }else if(recv=="该用户已登录"){
            cout<<"你已经登录，请勿重复登录"<<endl;
        }else if(recv=="密码错误"){
            cout<<"密码错误"<<endl;
        }else{
            cout<<"登录成功"<<endl;
            system("clear");
        }


    }
}


void pass_find(){
    string uid,pass,answer;
    cout<<"请输入你的uid:"<<endl;
    getline(cin,uid);

    Message msg(uid,QUESTION_GET);
    socket_fd.mysend(msg.S_to_json());
    string recv=socket_fd.client_recv();
    cout<<recv<<endl;

    getline(cin,answer);

    Message msg1(uid,ANSWER_GET);
    socket_fd.mysend(msg1.S_to_json());
    string right_answer=socket_fd.client_recv();

    if(answer != right_answer){
        cout<<"回答错误，无法找回"<<endl;
    }else{
        Message msg2(uid,PASS_GET);
        socket_fd.mysend(msg2.S_to_json());
        string pass=socket_fd.client_recv();

        cout<<"回答正确，你的密码是"<<pass<<endl;
    }

    return;    
}