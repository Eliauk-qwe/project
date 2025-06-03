#ifndef _MESSAGE_
#define _MESSAGE_
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;
class Message
{
public:
    Message() = default;
    Message(int flag) : flag(flag) {};
    Message(string uid, int flag) : uid(uid), flag(flag) {};
    Message(string uid, int flag, vector<string> para) : uid(uid), flag(flag), para(para) {};
    Message(string uid, string recuid, int flag) : uid(uid), recuid(recuid), flag(flag) {};
    Message(string uid, string recuid, int flag, vector<string> para) : uid(uid), recuid(recuid), flag(flag), para(para) {};
    Message(string name, string question, int flag, string pass, vector<string> para) : name(name), question(question), flag(flag), pass(pass), para(para) {};
    Message(string uid, string question, string recuid, int flag, string name, string pass, vector<string> para)
        : uid(uid), question(question), recuid(recuid), flag(flag), name(name), pass(pass), para(para) {}
    ~Message() = default;
    string uid = "", name = "", pass = "", recuid = "", question = "";
    int flag;
    vector<string> para = {""};
    void Json_to_s(string jsr)
    {
        json js = json::parse(jsr);
        js.at("uid").get_to(uid);
        js.at("name").get_to(name);
        js.at("pass").get_to(pass);
        js.at("question").get_to(question);
        js.at("recuid").get_to(recuid);
        js.at("flag").get_to(flag);
        js.at("para").get_to(para);
    }
    string S_to_json()
    {
        json js;
        js["uid"] = uid;
        js["name"] = name;
        js["pass"] = pass;
        js["question"] = question;
        js["recuid"] = recuid;
        js["flag"] = flag;
        js["para"] = para;
        return js.dump();
    }
};

#endif