#include <iostream>
#include <string>
#include <ctime>
#include "Coly.hpp"
using namespace std;
int main(){
    string command="sync var {\"Name\":\"WindowsCleaner:OnCleaning\",\"Value\":\"true\",\"Timestamp\":"+GXPass::c12c2<int,string>(time(0))+"}";
    // cout<<command<<endl;
    string host = "127.0.0.1";
    string port = "12345";
    NetworkSession session;
    if (!connect_to_server(session, host, port)) {
        cout << "Failed to connect to server" << endl;
        return 1;
    }
    string echo;
    echo = send_message(session,"login subprocess WindowsCleaner");
    echo = send_message(session,command);
    if(prefix(echo,string("[ERROR]").size())=="[ERROR]"){
        cout<<"Failed to sync var"<<endl;
        return 2;
    }
    return 0;
}
