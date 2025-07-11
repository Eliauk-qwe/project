#include "client.hpp"
void friend_quit_menu(){
    string opt;
    while(1){
        printf("-------------屏蔽好友界面-------------\n");
        printf("选项：\n[1]屏蔽好友\n[2]屏蔽好友列表\n[3]恢复好友\n[4]返回\n");
        printf("请输入你的选择：\n");
        getline(cin,opt);
        printf("-------------------------\n");

        while (1)
        {
            switch (stoi(opt))
            {
            case 1:
                friend_quit();
                break;
            case 2:
                friend_quit_list();
                break;
            case 3:
                friend_back();
                break;
            case 4:
                return;
            
            default:
                printf("请输入范围内的选择\n");
                break;
            }
        }
        

    }
    
    
}


void friend_quit(){
    string friend_quit_uid;
    cout<<"你想屏蔽的好友uid为:"<< endl;
    getline(cin,friend_quit_uid);

    Message msg(log_uid,friend_quit_uid,FRIEND_QUIT);
    socket_fd.mysend(msg.S_to_json());

    string recv=socket_fd.client_recv();
    if(recv=="friend_no_exist"){
        printf("你未添加该好友\n");
        return;
    }else if(recv=="success"){
        printf("屏蔽成功\n");
        return;
    }else if(recv=="fail"){
        printf("你之前已屏蔽过该好友\n");
        return;
    }

}


void friend_quit_list(){
    Message msg(log_uid,FRIEND_QUIT_LIST);
    socket_fd.mysend(msg.S_to_json());

    string recv;
    while((recv=socket_fd.client_recv())!= "end"){
        if(recv=="no_have_quit_list"){
            printf("你没有屏蔽的好友\n");
            return;
        }else{
            cout << recv <<endl;
        }
    }
    return;
}


void friend_back(){
    string friend_back_uid;
    printf("请输入你想恢复的好友的uid:\n");
    getline(cin,friend_back_uid);
    Message msg(log_uid,friend_back_uid,FRIEND_BACK);
    socket_fd.mysend(msg.S_to_json());

    string recv=socket_fd.client_recv();
    if(recv=="no_quit_freind"){
        printf("你没有屏蔽过该好友\n");
        return;
    }else if(recv=="no_add"){
        printf("你没有添加过该好友\n");
        return;
    }else if(recv=="success"){
        printf("你已成功恢复好友\n");
        return;
    }
}