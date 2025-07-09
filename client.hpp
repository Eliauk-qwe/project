#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <../3.hpp>
#include <../StickyPacket.hpp>

#define SIGNUP 1
#define LOGIN 2
#define QUESTION_GET 3
#define ANSWER_GET 4
#define PASS_GET 5


using namespace std;


extern StickyPacket socket_fd;
