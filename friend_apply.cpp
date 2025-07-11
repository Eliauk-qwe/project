#include "client.hpp"
void friend_apply_menu(){
    string opt;
    while(1){
        printf("-------------申请好友界面-------------\n");
        printf("选项：\n[1]同意好友申请\n[2]拒绝好友申请\n[3]返回\n");
        printf("请输入你的选择：\n");
        getline(cin,opt);
        printf("-------------------------\n");

        while (1)
        {
            switch (stoi(opt))
            {
            case 1:
                friend_apply_agree();
                break;
            case 2:
                friend_apply_refuse();
                break;
            
            default:
                printf("请输入范围内的选择\n");
                break;
            }
        }
        

    }
}

void friend_apply_agree(){
    string friend_agree_uid;
    printf("你想同意的收到的好友申请的uid:\n");
    getline(cin,friend_agree_uid);

    Message msg(log_uid,friend_agree_uid,FRIEND_APPLY_AGREE);
    socket_fd.mysend(msg.S_to_json());

    string recv =socket_fd.client_recv();
    if(recv=="no_recv_apply"){
        printf("你未收到该好友的申请\n");
        return;
    }

    if(recv=="success"){
        printf("你们已成功加为好友\n");
        return;
    }
    


}







void friend_apply_refuse(){
    string friend_refuse_uid;
    printf("你想拒绝的收到的好友申请的uid:\n");
    getline(cin,friend_refuse_uid);

    Message msg(log_uid,friend_refuse_uid,FRIEND_APPLY_REFUSE);
    socket_fd.mysend(msg.S_to_json());

    string recv =socket_fd.client_recv();
    if(recv=="no_recv_apply"){
        printf("你未收到该好友的申请\n");
        return;
    }

    if(recv=="success"){
        printf("你们已成功拒绝加为好友\n");
        return;
    }
}