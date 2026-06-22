#ifndef __COLY_HPP__
#define __COLY_HPP__

#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>
#include <map>
#include <cstdlib>
#include <vector>
#include <regex>
#include "json.hpp"
#include "GXPass.hpp"
#include "VariableSyncService.hpp"
using JSON = nlohmann::json;
#ifdef _WIN32
#define COLYPATH "C:\\Coly\\"
#define TempPATH "C:\\Coly\\TempCode\\"
#else
#define COLYPATH "/lib/Coly/"
#define TempPATH "/usr/local/lib/Coly/TempCode/"
#endif

void StartProcess(
    const std::string& filepath,
    const std::vector<std::string>& argv,
    std::function<void(const std::string&)> onOutput)
{

#ifdef _WIN32

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = NULL;
    HANDLE hWrite = NULL;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0))
        return;

    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;

    std::string cmd = "\"" + filepath + "\"";
    for (const auto& a : argv)
        cmd += " \"" + a + "\"";

    char* cmdCStr = new char[cmd.size() + 1];
    std::strcpy(cmdCStr, cmd.c_str());
    BOOL ok = CreateProcessA(
        NULL,
        cmdCStr,
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi);

    CloseHandle(hWrite);

    if (!ok)
    {
        CloseHandle(hRead);
        return;
    }

    std::thread([hRead, pi, onOutput]() {

        char buffer[4096];

        while (true)
        {
            DWORD bytesAvailable = 0;

            if (!PeekNamedPipe(hRead, NULL, 0, NULL, &bytesAvailable, NULL))
                break;

            if (bytesAvailable > 0)
            {
                DWORD bytesRead = 0;
                DWORD maxSize = sizeof(buffer) - 1;
                DWORD toRead = (bytesAvailable < maxSize) ? bytesAvailable : maxSize;

                if (ReadFile(hRead, buffer, toRead, &bytesRead, NULL) && bytesRead > 0)
                {
                    buffer[bytesRead] = 0;
                    onOutput(std::string(buffer));
                }
            }

            DWORD exitCode = 0;
            if (GetExitCodeProcess(pi.hProcess, &exitCode) &&
                exitCode != STILL_ACTIVE)
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        CloseHandle(hRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

    }).detach();


#else

    int pipefd[2];
    if (pipe(pipefd) == -1)
        return;

    pid_t pid = fork();

    if (pid == 0)
    {
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        close(pipefd[0]);
        close(pipefd[1]);

        std::vector<char*> args;
        args.push_back(const_cast<char*>(filepath.c_str()));
        for (const auto& a : argv)
            args.push_back(const_cast<char*>(a.c_str()));
        args.push_back(nullptr);

        execvp(filepath.c_str(), args.data());
        _exit(1);
    }
    else if (pid > 0)
    {
        close(pipefd[1]);

        int flags = fcntl(pipefd[0], F_GETFL, 0);
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

        std::thread([pipefd, pid, onOutput]() {

            char buffer[4096];

            struct pollfd pfd{};
            pfd.fd = pipefd[0];
            pfd.events = POLLIN;

            while (true)
            {
                int ret = poll(&pfd, 1, 50);

                if (ret > 0 && (pfd.revents & POLLIN))
                {
                    ssize_t count = read(pipefd[0], buffer, sizeof(buffer) - 1);
                    if (count > 0)
                    {
                        buffer[count] = 0;
                        onOutput(std::string(buffer));
                    }
                }

                int status = 0;
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result == pid)
                    break;
            }

            close(pipefd[0]);

        }).detach();
    }

#endif
}
void RunCommand(const std::string& command)
{
    std::vector<std::string> args;
    std::string filepath;

    // 使用正则支持双引号带空格的参数
    std::regex re(R"((\"[^\"]+\"|\S+))");
    auto words_begin = std::sregex_iterator(command.begin(), command.end(), re);
    auto words_end   = std::sregex_iterator();

    bool first = true;
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::string token = (*i).str();
        // 去掉双引号
        if (!token.empty() && token.front() == '"' && token.back() == '"')
            token = token.substr(1, token.size() - 2);

        if (first) {
            filepath = token; // 第一个是可执行文件
            first = false;
        } else {
            args.push_back(token); // 后续是参数
        }
    }

    StartProcess(filepath, args, [](const std::string& output){
        std::cout << output;
    });
}


// Map to store the line number of defined variables, codes and positions
// This is used to check if a variable or code is defined before use
std::map<std::string, int> definedline;
// Get the prefix of a std::string with a specified length
std::string prefix(std::string str,int len){
    if(str.length() < len) return str;
    return str.substr(0, len);
}
// The following operations are supported in Coly
// define: define a variable or code or position
// use: use a  code
// jump: jump to a code / position
// import lib: import a libary
// print: print a variable or code
// printwithoutanewline: print a variable or code without a newline
// do: execute a code
// exit: exit the program
// ifn: if not, execute a code
// if: if, execute a code
std::vector<std::string> operationlist = {
    "define",
    "usewithoutwait",
    "use",
    "jump",
    "printwithoutanewline",
    "print",
    "do",
    "exit",
    "ifn",
    "if",
    "import lib",
    "commitvaroperation"
};
std::vector<std::string> skipsynclist = {
    "Input",
    "InputLine",
    "NoReg",
    "Size",
    "ASCII"
};
// Structure to hold information about defined variables, codes, and positions
// It includes type, name, language, content, code information, and variable type
struct defineinfo {
    std::string name; // name of the variable/code/positon
    std::string language; // language type, e.g., C++, Python, etc.
    std::string content; // content of the variable or code
    std::vector<std::string> functioncontent; // for function, store the content of the function
    bool isprivate; // whether the variable or code is private
    defineinfo(){
        name = "";
        language = "";
        content = "";
        isprivate = false;
    }
    friend std::ostream& operator<<(std::ostream &os, const defineinfo &info) {
        os  << "Name: "     << info.name     << std::endl
        << "Language: " << info.language << std::endl
        << "Content: "  << info.content  << std::endl;
        return os;
    }
    std::string getvalue(NetworkSession& session){
        std::string commit_command="get var ";
        commit_command+=this->name;
        std::string echo="";
        if(!isprivate) echo=send_message(session,commit_command);
        if(prefix(echo, 7) == "[ERROR]" && !isprivate){
            std::cout << echo << std::endl;
            return echo;
        }
        JSON j;
        if(!isprivate) j=JSON::parse(echo);
        if(!isprivate) this->content=j["Value"];
        std::string content = this->content;
        if(name == "InputLine"){
            std::getline(std::cin >> std::ws, content);
        }else if(name == "Input"){
            std::string temp;
            std::cin>> content;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }else if(name == "Size"){
            return std::to_string(content.size());
        }else if(name == "ASCII"){
            return GXPass::c12c2<char,std::string>((char)(GXPass::c12c2<std::string,int>(content)));
        }
        return content;
        return "ERROR: Undefined type";
    }
};
// Map to store defined positions
std::map<std::string, int> definedposition;
// Map to store defined variables
std::map<std::string, defineinfo> definedvar;
// Map to store compiled codes
std::map<std::string, bool> compiledcode;
// Map to store defined codes
std::map<std::string, defineinfo> definedfunction;
std::string GetVarValue(std::string varname, NetworkSession& session){
    if(definedvar.find(varname) != definedvar.end()){
        return definedvar[varname].getvalue(session);
    } else if(definedvar.find(varname) != definedvar.end()){
        return definedvar[varname].getvalue(session);
    } else {
        std::string command="get var "+varname;
        std::string echo = send_message(session, command);
        if(prefix(echo,7)=="[ERROR]"){
            std::cout<<echo<<std::endl;
            return echo;
        }
        JSON j = JSON::parse(echo);
        return j["Value"];
    }
}
std::string getSyntaxValue(const std::string &content, NetworkSession& session){
    std::stringstream ret;
    for(int i=0;i<content.size();i++){
        char c = content[i];
        if(c == '$'){
            std::string varname;
            for(i=i+1;i<content.size();i++){
                if(content[i] == ' ' || content[i] == '\n' || content[i] == '\r'){
                    break;
                }
                varname += content[i];
            }
            ret << GetVarValue(varname, session);
        }else if(c == '\n'){
            ret << std::endl;
        }else if(c == '\r'){
            // Ignore carriage return
        }else ret << c;
    }
    return ret.str();
}
// Judge the type of a define operation, register the variable or code and return the defineinfo
defineinfo judgedefine(std::string content, NetworkSession& session, bool &defining, int lineid, bool &overdefine){
    //0: type 1: named 2: with 3: codeinfo/varinfo 4:vartype 5:| 6: codevar
    defineinfo info;
    int infotype = 0;
    std::string varname="";
    std::string type="";
    std::string left="";
    std::string functionname="";
    defining = true;
    // Avoid replacing var/privatevar content
    // while(content.find("  ") != std::string::npos) content.replace(content.find("  "), 2, " ");
    for(char c : content){
        if(c == ' '){
            infotype++;
            if(infotype >= 5&&(type == "var"||type == "privatevar")){
                left += c;
            }
            if(infotype == 1&&type == "function"){
                if(!info.functioncontent.empty()) info.functioncontent.clear();
            }
            if(infotype >= 5&&type == "function"){
                if(infotype >= 6) left = "define var named "+left+" with ";
                else info.functioncontent.push_back(left);
                left="";
            }
            continue;
        }
        if(infotype == 0){// type
            type += c;
        } else if(infotype == 1){// named
        } else if(infotype == 2){// name
            info.name += c;
        } else if(infotype == 3){// with
            info.content = "";
        } else if(infotype == 4&&(type == "code"||type == "privatecode")){// Language
            info.language += c;
        } else if(infotype >= 4&&(type == "var"||type == "privatevar")){// var value
            left+=c;
        }else if(infotype >= 4&&(type == "function")){// function content
            left+=c;
        }
        else if(infotype ==5 && type == "code"){// |
        }else if(infotype==6 && type == "code"){
            left+=c;
        }
    }
    if(infotype >= 4&&type == "function"){
        if(infotype >= 5) left = "define var named "+left+" with ";
        info.functioncontent.push_back(left);
        left="";
    }
    if(type == "function") info.isprivate = true;
    if(type == "privatevar" || type == "privatecode") info.isprivate = true;
    for(std::string str:skipsynclist) if(str==info.name) info.isprivate = true;
    if(infotype==6 && type == "code"){
        getSyntaxValue(left, session);
        info.content = left;
        defining = false;
    }else if(infotype >= 2&&type == "position"){
        definedposition[info.name] = lineid;
        definedline[info.name] = lineid;
        defining = false;
        overdefine=0;
        return defineinfo();
    }else if(infotype >=4&&type == "function"){
        definedfunction[info.name] = info;
        defining = false;
    }
    if(!left.empty()){
        std::string value = getSyntaxValue(left, session);
        info.content = value;
        overdefine=1;
        defining = false;
    }
    // std::co<<info<<std::endl;
    return info;
}
// Get the file extension for a given language from LanguageMap.json
std::string getextension(const std::string language, NetworkSession& session) {
    std::string LanguageVarName = language + ":extension";
    if(definedvar.find(LanguageVarName) != definedvar.end()){
        return definedvar[LanguageVarName].getvalue(session);
    }
    std::string LanguageJSONPath = COLYPATH;
    LanguageJSONPath+="Settings/LanguageMap.json";
    FILE *stream=fopen(LanguageJSONPath.c_str(), "r");
    if (!stream) {
        std::cout << "Error: Cannot open LanguageMap.json" << std::endl;
        return ".txt"; // Default extension if file cannot be opened
    }
    int c=0;
    std::string fileinfo;
    while((c=fgetc(stream))!=-1){
        fileinfo += (char)c;
    }
    fclose(stream);
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(fileinfo);
    } catch (const nlohmann::json::parse_error& e) {
        std::cout << "Error: JSON parse error: " << e.what() << std::endl;
        return ".txt"; // Default extension if parsing fails
    }
    return json[language]["extension"].get<std::string>();
}
// Get the need of compile for a given language from LanguageMap.json
bool getneedcompile(const std::string language, NetworkSession& session) {
    std::string LanguageVarName = language + ":needcompile";
    if(definedvar.find(LanguageVarName) != definedvar.end()){
        std::string value = definedvar[LanguageVarName].getvalue(session);
        return value == "true" || value == "1";
    }
    std::string LanguageJSONPath = COLYPATH;
    LanguageJSONPath+="Settings/LanguageMap.json";
    FILE *stream=fopen(LanguageJSONPath.c_str(), "r");
    if (!stream) {
        std::cout << "Error: Cannot open LanguageMap.json" << std::endl;
        return false; // Default if file cannot be opened
    }
    int c=0;
    std::string fileinfo;
    while((c=fgetc(stream))!=-1){
        fileinfo += (char)c;
    }
    fclose(stream);
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(fileinfo);
    } catch (const nlohmann::json::parse_error& e) {
        std::cout << "Error: JSON parse error: " << e.what() << std::endl;
        return false; // Default if parsing fails
    }
    return json[language]["needcompile"].get<bool>();
}
// Get the compiler run command for a given language from LanguageMap.json
std::string getcompilerun(const std::string language, NetworkSession& session) {
    std::string LanguageVarName = language + ":compilerun";
    if(definedvar.find(LanguageVarName) != definedvar.end()){
        return definedvar[LanguageVarName].getvalue(session);
    }
    std::string LanguageJSONPath = COLYPATH;
    LanguageJSONPath+="Settings/LanguageMap.json";
    FILE *stream=fopen(LanguageJSONPath.c_str(), "r");
    if (!stream) {
        std::cout << "Error: Cannot open LanguageMap.json" << std::endl;
        return ".txt"; // Default run if file cannot be opened
    }
    int c=0;
    std::string fileinfo;
    while((c=fgetc(stream))!=-1) fileinfo += (char)c;
    fclose(stream);
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(fileinfo);
    } catch (const nlohmann::json::parse_error& e) {
        std::cout << "Error: JSON parse error: " << e.what() << std::endl;
        return ""; // Default run if parsing fails
    }
    return json[language]["compilerun"].get<std::string>();
}
// Get the run command for a given language from LanguageMap.json
std::string getrun(const std::string language, NetworkSession& session) {
    std::string LanguageVarName = language + ":run";
    if(definedvar.find(LanguageVarName) != definedvar.end()){
        return definedvar[LanguageVarName].getvalue(session);
    }
    // std::co << "Getting run command for language: " << language << std::endl;
    std::string LanguageJSONPath = COLYPATH;
    LanguageJSONPath+="Settings/LanguageMap.json";
    FILE *stream=fopen(LanguageJSONPath.c_str(), "r");
    if (!stream) {
        std::cout << "Error: Cannot open LanguageMap.json" << std::endl;
        return ".txt"; // Default run if file cannot be opened
    }
    int c=0;
    std::string fileinfo;
    while((c=fgetc(stream))!=-1){
        fileinfo += (char)c;
    }
    fclose(stream);
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(fileinfo);
    } catch (const nlohmann::json::parse_error& e) {
        std::cout << "Error: JSON parse error: " << e.what() << std::endl;
        return ""; // Default run if parsing fails
    }
    return json[language]["run"].get<std::string>();
}
// TODO: 3rd language variable sync service
// Use a defined code, compile it if necessary, and run it
void usedefine(const std::string &content, NetworkSession& session, bool wait=true) {
    if(definedline.find(content) == definedline.end()){
        std::cout << "Error: Undefined variable or code: " << content << std::endl;
        return;
    }
    defineinfo info = definedvar[content];
    std::string extension = getextension(info.language, session);
    std::string filename = TempPATH;
    filename += GXPass::number2ABC(GXPass::compile(info.name))+"."+extension;
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        std::cout << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    for(char c:info.content) fputc(c, fp);
    fclose(fp);
    std::string command = "";
    std::string codename = GXPass::number2ABC(GXPass::compile(info.name));
    if((compiledcode.find(codename) != compiledcode.end() && compiledcode[codename]) || !getneedcompile(info.language, session)){
        command=getrun(info.language, session); // If the code has been compiled, just run it
    } else command=getcompilerun(info.language, session); // If the code has not been compiled, compile it first
    size_t pos = command.find('$');
    while(pos != std::string::npos)command.replace(pos, 1, filename),pos = command.find('$');
    pos = command.find('^');
    std::string outputfilepath = filename.substr(0, filename.find_last_of('.'));
    compiledcode[codename] = true; // Mark the code as compiled
    while(pos != std::string::npos) command.replace(pos, 1, outputfilepath),pos = command.find('^');
    pos = command.find('*');
    static int num=0;
    std::string sessiondata(reinterpret_cast<const char*>(&session),sizeof(session));
    static std::string RunProof=GXPass::c12c2<int,std::string>(time(0));
    RunProof = GXPass::number2ABC(GXPass::compile(RunProof)+"_"+GXPass::compile(sessiondata));
    while(pos != std::string::npos) command.replace(pos, 1, RunProof),pos = command.find('*');
    std::string regcommand = "reg subprocess ";
    regcommand += RunProof;
    std::string echo;
    if(definedvar.find("NoReg") != definedvar.end()){
        if(definedvar["NoReg"].getvalue(session) != "true"){
            echo = send_message(session, regcommand);
        }
    }else echo = send_message(session, regcommand);
    if(prefix(echo, 7) == "[ERROR]"){
        std::cout << echo << std::endl;
        return;
    }
    if(command.empty()){
        std::cout << "Error: No run command defined for language: " << info.language << std::endl;
        return;
    }
    // std::cout<< "Running command: " << command << std::endl;
    if(wait) system(command.c_str());
    else{
        if((compiledcode.find(codename) != compiledcode.end() && compiledcode[codename]) || !getneedcompile(info.language, session)){
            RunCommand(command);
        }
    }
}
// Print the content to the console, handling variables and newlines
// This function replaces variables with their values and prints the content to the console
void print(const std::string &content, NetworkSession& session) {
    std::cout<< getSyntaxValue(content, session);
}
std::string definevarcommand(const defineinfo &info, NetworkSession& session){
    if(info.isprivate == true) return "";
    std::string command;
    JSON j = {
        {"Name", info.name},
        {"Value", info.content},
        {"Timestamp", time(0)}
    };
    command = "sync var " + j.dump();
    // std::co<<command<<std::endl;
    return command;
}
std::vector<std::string> readCly(std::string path);
void useCly(std::vector<std::string> lines,NetworkSession& session);
void doCly(std::string content, int *lineid, NetworkSession& session);
// Address a line of code, handling different operations like define, use, jump, import, print, etc.
// This function processes a line of code and performs the corresponding operation
void addressline(const std::string &lineContent,int *lineid, NetworkSession& session){
    std::string line = "";
    for(char c:lineContent){
        if( c != '\t'&&
            c != '\r'&&
            c != '\n') line += c;
    }
    while(line[0] == ' ') line = line.substr(1);
    /* Process function */
    int firstspace=0;
    while(firstspace<line.size()&&line[firstspace]!=' ') firstspace++;
    if(firstspace<=line.size()){
        std::string firstword = line.substr(0, firstspace);
        if(definedfunction.find(firstword) != definedfunction.end()){
            defineinfo info = definedfunction[firstword];
            std::string content = "";
            std::vector<std::string> args;
            for(int i=firstspace+1;i<line.size();i++){
                if(line[i] == ' '){
                    args.push_back(content);
                    content = "";
                    continue;
                }
                content += line[i];
            }
            if(!content.empty()) args.push_back(content),content="";
            for(int i=1;i<info.functioncontent.size();i++){
                std::string content = info.functioncontent[i];
                content+=args[i-1];
                addressline(content, lineid, session);
            }
            content = info.functioncontent[0];
            content="use "+content;
            addressline(content, lineid, session);
            return;
        }
    }
    /* End of process function */
    static bool defining = false;
    bool overdefine=0;
    static defineinfo info;
    bool definedoperation=0;
    std::string commit_command="";
    std::string content = line;
    if(defining && content[0] == '|'){
        content = content.substr(1);
        // std::co<<content<<" | "<<info.codeinfo<<std::endl;
        info.content += content + "\n";
        return;
    }else if(defining && content[0] != '|'){
        defining = false;
        overdefine = 1;
        definedvar[info.name] = info;
    }
    for(int i=0;i<operationlist.size();i++){
        if(prefix(line, operationlist[i].length()) == operationlist[i]){
            std::string content = line.substr(operationlist[i].length());
            while(content[0] == ' ') content = content.substr(1);
            if(operationlist[i] == "define"){
                definedoperation=1;
                // std::co << "Defining: " << content << std::endl;
                info=judgedefine(content, session,defining,*lineid,overdefine);
                // std::co<<info<<std::endl;
                if(!info.name.empty()) definedvar[info.name] = info;
                // if(!command.empty()){
                //     std::string echo=send_message(session, command);
                //     std::cout<<echo<<std::endl;
                //     if(echo.substr(0,sizeof("[ERROR]"))=="[ERROR]"){
                //         std::cout<<echo<<std::endl;
                //     }
                // }
                if(!info.name.empty()) definedline[info.name] = *lineid;
                break;
            } else if(operationlist[i] == "usewithoutwait"){
                definedoperation=1;
                // std::co << "Using: " << content << std::endl;
                // std::co<< info <<std::endl;
                usedefine(content, session, false);
                break;
            } else if(operationlist[i] == "use"){
                definedoperation=1;
                // std::co << "Using: " << content << std::endl;
                // std::co<< info <<std::endl;
                usedefine(content, session);
                break;
            } else if(operationlist[i] == "jump"){
                definedoperation=1;
                // std::co << "Jumping to: " << content << std::endl;
                // std::co<< info <<std::endl;
                *lineid = definedline[content];
                if(definedposition.find(content) == definedposition.end()) usedefine(content, session);
                break;
            } else if(operationlist[i] == "printwithoutanewline"){
                definedoperation=1;
                print(content, session);
                break;
            } else if(operationlist[i] == "print"){
                definedoperation=1;
                print(content + " \n", session);
                break;
            }else if (operationlist[i] == "do") {
                definedoperation=1;
                int *fake_lineid = new int(-1);
                doCly(content, fake_lineid, session);
                delete fake_lineid;
                break;
            } else if (operationlist[i] == "exit") {
                definedoperation=1;
                exit(0);
            } else if(operationlist[i] == "if") {
                definedoperation=1;
                // std::co << "If: " << content << std::endl;
                int varnum=0;
                std::string var1,var2;
                std::string varname;
                std::string left="";
                for(int i=0;i<content.size();i++){
                    char c = content[i];
                    if(c=='$'){
                        varnum++;
                    } else if(c==' '){
                        if(varnum == 1) {
                            var1=GetVarValue(varname, session);
                        } else if (varnum == 2) {
                            var2=GetVarValue(varname, session);
                            i++;
                            for(;i<content.size();i++){
                                left+=content[i];
                            }
                            break;
                        }
                        varname="";
                    } else varname+=c;
                    // else break;
                }

                // std::cout<<"var1: "<<var1<<" var2: "<<var2<<" left: "<<left<<std::endl;
                if(var1==var2) doCly(left, lineid, session);
                break;
            } else if(operationlist[i] == "ifn") {
                definedoperation=1;
                // std::co << "If: " << content << std::endl;
                int varnum=0;
                std::string var1,var2;
                std::string varname;
                std::string left="";
                for(int i=0;i<content.size();i++){
                    char c = content[i];
                    if(c=='$'){
                        varnum++;
                    } else if(c==' '){
                        if(varnum == 1) {
                            var1=GetVarValue(varname, session);
                        } else if (varnum == 2) {
                            var2=GetVarValue(varname, session);
                            i++;
                            for(;i<content.size();i++){
                                left+=content[i];
                            }
                            break;
                        }
                        varname="";
                    } else varname+=c;
                    // else break;
                }

                // std::cout<<"var1: "<<var1<<" var2: "<<var2<<" left: "<<left<<std::endl;
                if(var1!=var2) doCly(left, lineid, session);
                break;
            } else if(operationlist[i] == "import lib"){
                definedoperation=1;
                useCly(readCly(content), session);
                break;
            } else if(operationlist[i] == "commitvaroperation"){
                definedoperation=1;
                std::string echo = send_message(session, content);
                if(prefix(echo, 7) == "[ERROR]"){
                    std::cout << echo << std::endl;
                    return;
                }
                break;
            }
        }
    }
    if(!definedoperation) std::cout<<"Unknown Command:"<<line<<std::endl;
    if(overdefine){
        commit_command=definevarcommand(info, session);
        if(!commit_command.empty()){
            std::string echo=send_message(session, commit_command);
            // std::co<<echo<<std::endl;
            if(prefix(echo, 7) == "[ERROR]"){
                std::cout<<echo<<std::endl;
            }
        }
        overdefine=0;
    }
}
void doCly(std::string content, int *lineid, NetworkSession& session){
    std::string command = getSyntaxValue(content, session);
    addressline(command, lineid, session);
}
// Read a Coly file and return its lines, handling imports
std::vector<std::string> readCly(std::string path){
    std::ifstream stream(path);
    if(!stream.is_open()){
        return {"Error: Cannot open file " + path};
    }
    std::vector<std::string> lines;
    std::string line;
    while(getline(stream, line)){
        if(prefix(line, sizeof("import lib")) == "import lib "){
            std::string importpath = line.substr(sizeof("import lib"));
            // std::co<< "Importing library: " << importpath << std::endl;
            bool isabs = false;
            for(char c:importpath) if(c == ':'){ isabs = true; break; }
            if(!isabs){
                importpath = COLYPATH;
                importpath += "/lib/";
                importpath += importpath;
            }
            std::vector<std::string> importinfo=readCly(importpath);
            for(std::string importline : importinfo){
                lines.push_back(importline);
            }
            continue;
        }
        lines.push_back(line);
    }
    stream.close();
    // for(std::string line:lines) std::cout<<line<<std::endl;
    return lines;
}
// Use a Coly file, processing its lines and handling jumps
void useCly(std::vector<std::string> lines,NetworkSession& session){
    for(int lineid=0;lineid<lines.size();lineid++){
        std::string line = lines[lineid];
        if(line.empty() || line[0] == '#') continue; // Skip empty lines and comments
        int beforelineid = lineid;
        addressline(line, &lineid, session);
        if(beforelineid != lineid) {
            lineid--; // Adjust lineid if it was changed by a jump
        }
    }
}

#endif // __COLY_HPP__