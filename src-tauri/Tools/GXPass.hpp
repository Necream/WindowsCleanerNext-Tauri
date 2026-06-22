// Version: 1.7.2    Latest Version: https://github.com/Necream/GXPass
#ifndef __GXPASS_HPP__
#define __GXPASS_HPP__

// Oigin GXPass
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include "NCint.hpp" // v1.0.0

// encrypt
#include <fstream>
#include <cstdint>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#endif

namespace GXPass {
    struct map{
        char value[256];
        map() {
            for(int i=0;i<256;i++){
                value[i]=0;
            }
        }
        // 可写版本（非 const 对象）
        inline char& operator[](unsigned char c) {
            return value[c];
        }

        // 只读版本（const 对象）
        inline const char& operator[](unsigned char c) const {
            return value[c];
        }
    };

    // Get a unique device identifier
    std::string getDeviceUniqueID() {
        std::string id;
#ifdef _WIN32
        // Windows: Use the motherboard serial number
        HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (SUCCEEDED(hres)) {
            hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                                        NULL, EOAC_NONE, NULL);
            if (SUCCEEDED(hres)) {
                IWbemLocator *pLoc = nullptr;
                if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                                            IID_IWbemLocator, (LPVOID *)&pLoc))) {
                    IWbemServices *pSvc = nullptr;
                    if (SUCCEEDED(pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"),
                                                    NULL, NULL, 0, NULL, 0, 0, &pSvc))) {
                        CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                                        NULL, EOAC_NONE);

                        IEnumWbemClassObject* pEnumerator = nullptr;
                        if (SUCCEEDED(pSvc->ExecQuery(
                            bstr_t("WQL"),
                            bstr_t("SELECT SerialNumber FROM Win32_BaseBoard"),
                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                            NULL, &pEnumerator))) {

                            IWbemClassObject *pclsObj = nullptr;
                            ULONG uReturn = 0;
                            if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn != 0) {
                                VARIANT vtProp;
                                pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
                                id = _bstr_t(vtProp.bstrVal);
                                VariantClear(&vtProp);
                                pclsObj->Release();
                            }
                            pEnumerator->Release();
                        }
                        pSvc->Release();
                    }
                    pLoc->Release();
                }
            }
            CoUninitialize();
        }

        // if motherboard serial number is unavailable, fallback to volume serial number
        if (id.empty()) {
            DWORD serial = 0;
            if (GetVolumeInformationA("C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0)) {
                id = std::to_string(serial);
            }
        }

#else
        // Linux/macOS: use machine UUID
        std::ifstream file("/etc/machine-id");
        if (file.is_open()) {
            std::getline(file, id);
        }
        if (id.empty()) {
            // For old Linux
            file.close();
            file.open("/var/lib/dbus/machine-id");
            if (file.is_open()) std::getline(file, id);
        }
#endif

        if (id.empty()) id = "UNKNOWN_DEVICE";

        return id;
    }

    // 92个可打印字符，可自行修改
    std::string charset =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "!@#$%^&*()_+-=[]{}|;':\",./<>?";

    // 类型转换工具：c1类型转c2类型（如数字转字符串）
    template<class c1, class c2>
    c2 c12c2(const c1& data) {
        c2 ret;
        std::stringstream ss;
        ss << data;
        ss >> ret;
        return ret;
    }

    // 快速幂模板函数，默认模为类型最大值（表示无模）
    template<class T = unsigned long long>
    T ksm(T a, T n, T mod = static_cast<T>(-1)) {
        if (n == 0) return 1 % mod;
        if (n == 1) return a % mod;

        T temp = ksm<T>(a, n / 2, mod);
        temp = (temp * temp) % mod;

        if (n % 2 == 1) {
            temp = (temp * (a % mod)) % mod;
        }
        return temp;
    }

    // 计算字符串字符ASCII码和，返回字符串
    template<class type = unsigned long long>
    std::string sum(const std::string password) {
        type sum = 0;
        for (char c : password) {
            sum += static_cast<type>(c);
        }
        return c12c2<type, std::string>(sum);
    }

    // 奇偶位置加减和，返回字符串
    template<class type = unsigned long long>
    std::string dxsum1(const std::string password) {
        type sum = 0;
        for (type i = 0; i < static_cast<type>(password.size()); i++) {
            if (i % 2 == 0) {
                sum += static_cast<type>(password[i]);
            } else {
                sum -= static_cast<type>(password[i]);
            }
        }
        return c12c2<type, std::string>(sum);
    }

    // 另一种奇偶位置加减和，返回字符串
    template<class type = unsigned long long>
    std::string dxsum2(const std::string password) {
        type sum = 0;
        for (type i = 0; i < static_cast<type>(password.size()); i++) {
            if (i % 2 == 0) {
                sum -= static_cast<type>(password[i]);
            } else {
                sum += static_cast<type>(password[i]);
            }
        }
        return c12c2<type, std::string>(sum);
    }

    // 使用快速幂累加加密，模默认为类型最大值（无模）
    template<class type = unsigned long long>
    std::string ksmsum(const std::string password, type modulus = static_cast<type>(-1)) {
        type sum = 0;
        for (char c : password) {
            // 显式指定模板参数，确保推导正确
            sum = (sum + ksm<type>(static_cast<type>(c), static_cast<type>(password.size()), modulus)) % modulus;
        }
        return c12c2<type, std::string>(sum);
    }

    // 编译入口，调用各种密码运算  version:-1最新，0基础，1顺序重置
    template<class type = unsigned long long>
    std::string compile(const std::string data, int version = -1) {
        std::string ret;
        switch (version) {
            case -1: // latest
            case 2: {
                type sum_data=c12c2<std::string,type>(sum<type>(data));
                type dxsum1_data=c12c2<std::string,type>(dxsum1<type>(data));
                type dxsum2_data=c12c2<std::string,type>(dxsum2<type>(data));
                type ksmsum_data=c12c2<std::string,type>(ksmsum<type>(data));
                type sortdata[4]={sum_data,dxsum1_data,dxsum2_data,ksmsum_data};
                type data[4]={sum_data,dxsum1_data,dxsum2_data,ksmsum_data};
                std::sort(sortdata,sortdata+4);
                for(int i=0;i<4;i++){
                    for(int j=0;j<4;j++){
                        if(sortdata[i]==data[j]) ret+=c12c2<type,std::string>(data[j]);
                    }
                }
                break;
            }
            case 1: {
                ret += dxsum1<type>(data);
                ret += sum<type>(data);
                ret += dxsum2<type>(data);
                ret += ksmsum<type>(data);
                break;
            }
            case 0: {
                ret += sum<type>(data);
                ret += dxsum1<type>(data);
                ret += dxsum2<type>(data);
                ret += ksmsum<type>(data);
                break;
            }
            default:
                break;
        }
        return ret;
    }

    // 数字转字母映射辅助函数
    std::string number2ABC(const std::string data) {
        std::string ret = "";
        int number = -1;
        for (int i = 0; i < static_cast<int>(data.size()); i++) {
            if (number == -1) {
                number = data[i] - '0';
                continue;
            }
            if (number == 0) {
                if (data[i] == '0') continue;
                else {
                    number = data[i] - '0';
                    ret += char('A' + number - 1);
                    number = -1;
                }
            } else {
                switch (number) {
                    case 1: {
                        number = number * 10 + data[i] - '0';
                        ret += char('A' + number - 1);
                        number = -1;
                        break;
                    }
                    case 2: {
                        if (data[i] <= '6') {
                            number = number * 10 + data[i] - '0';
                            ret += char('A' + number - 1);
                            number = -1;
                        } else {
                            ret += char('A' + number - 1);
                            number = -1;
                            ret += char('A' + (data[i] - '0') - 1);
                        }
                        break;
                    }
                    default: {
                        ret += char('A' + number - 1);
                        number = data[i] - '0';
                        break;
                    }
                }
            }
        }
        if (number != 0 && number != -1) {
            ret += char('A' + number - 1);
        }
        return ret;
    }
    // 数字转安全字符映射辅助函数私有实现，不可以外部调用，防止错误使用
    std::string number2safestring_Private_CannotUse(std::string data, const std::string& chars = charset) {
        int charsLen=charset.size();
        std::string prefix="";
        std::string ret="";
        while(!data.empty()){
            prefix="";
            prefix=data.substr(0,2);
            int number=c12c2<std::string,int>(prefix);
            double rate = (double)(charsLen) / 100.0;
            int index=(int)(number*rate)%charsLen;
            // std::cout<<index<<" "<<chars[index]<<std::endl;
            ret+=chars[index];
            // std::cout<<ret<<std::endl;
            data.erase(0,2);
        }
        return ret;
    }
    // 数字转安全字符映射辅助函数（确定性扰动版）
    std::string number2safestring(std::string data, const std::string& chars = charset) {
        int dataLen = data.size();
        int charsLen = chars.size();
        std::string ret = "";

        // 如果长度为奇数，添加一个数字，使长度为偶数
        if (dataLen % 2 == 1) {
            int sum = 0;
            for (char c : data) sum += (c - '0');
            data += '0' + (sum % 10);
            dataLen++;
        }

        // 计算一个固定哈希值作为扰动基
        unsigned long long hash = 14695981039346656037ull; // FNV offset basis
        for (char c : data) {
            hash ^= static_cast<unsigned long long>(c);
            hash *= 1099511628211ull; // FNV prime
        }

        // 按两位一组映射到字符表
        for (int i = 0; i < dataLen; i += 2) {
            int num = (data[i] - '0') * 10 + (data[i + 1] - '0');
            // 添加确定性扰动：哈希混合
            int index = (num + (hash % charsLen)) % charsLen;
            ret += chars[index];
            // 更新哈希用于下一轮扰动
            hash = hash * 31 + index;
        }

        return ret;
    }

    // 计算安全密码长度，默认最大碰撞概率1e-6，字符集大小92
    int computeSafePassLen(int inputLength, int minPassLen = 6) {
        if(inputLength<minPassLen){
            return minPassLen/inputLength*1000;
        }else{
            return minPassLen*1000/inputLength+128;
        }
    }

    // Full safety Pass
    // data: 输入数据
    // version: -1最新，1旧版
    // PassLen: -1输入长度，-2根据输入长度计算，其他为固定长度
    // preLen: 预处理长度
    // chars: 可选字符集
    // minPassLen: 计算时的最小密码长度
    template<class type = unsigned long long>
    std::string fullsafe(const std::string data, int version = -1, int PassLen = 256,int preLen = 256, const std::string& chars = charset,int minPassLen = 6){ // 新增参数
        switch(version){
            case -1:
            case 2:{
                std::string Inputdata=data;
                for(int i=0;i<Inputdata.size();i++){
                    Inputdata[i]=(Inputdata[i]+i)%127;
                }
                std::string OriginalPass=number2safestring(compile<type>(Inputdata, version), chars);
                if(PassLen==-1) PassLen=OriginalPass.size();
                if(PassLen==-2) PassLen=computeSafePassLen(data.size(),minPassLen);
                if(PassLen<=0) return "";
                std::string FinalPass="";
                FinalPass+=OriginalPass[0];
                std::string StepPass=OriginalPass;
                for(int i=1;i<PassLen+preLen;i++){
                    StepPass+=number2safestring(compile<type>(StepPass, version), chars);
                    FinalPass+=StepPass[i%StepPass.size()];
                }
                FinalPass=FinalPass.substr(preLen,PassLen+preLen);
                return FinalPass;
                break;
            }
            case 1:{
                std::string Inputdata=data;
                for(int i=0;i<Inputdata.size();i++){
                    Inputdata[i]=(Inputdata[i]+i)%127;
                }
                std::string OriginalPass=number2safestring(compile<type>(Inputdata, version), chars);
                if(PassLen==-1) PassLen=OriginalPass.size();
                if(PassLen==-2) PassLen=computeSafePassLen(data.size(),minPassLen);
                if(PassLen<=0) return "";
                std::string FinalPass="";
                FinalPass+=OriginalPass[0];
                std::string StepPass=OriginalPass;
                for(int i=1;i<PassLen;i++){
                    StepPass+=number2safestring(compile<type>(StepPass, version), chars);
                    FinalPass+=StepPass[i%StepPass.size()];
                }
                return FinalPass;
                break;
            }
        }
        return "";
    }
    std::string getPermutationAfterN(NCint<> n,std::string charset) {
        static std::vector<NCint<>> jiecheng;
        static bool initialized = false;
        if(!initialized) {
            jiecheng.push_back(NCint<>(1)); // 0!
            for (int i = 1; i <= 300; i++) {
                jiecheng.push_back(jiecheng.back() * NCint<>(i));
            }
            initialized = true;
        }
        int max_jiecheng_id = charset.size();
        n=n%jiecheng[max_jiecheng_id];
        std::string result="";
        for(;max_jiecheng_id>0;max_jiecheng_id--){
            NCint<> index=n/jiecheng[max_jiecheng_id-1];
            n=n%jiecheng[max_jiecheng_id-1];
            result+=charset[int(index)];
            charset.erase(charset.begin()+int(index));
        }
        return result;
    }
    map getCharsetsMap_encrypy(NCint<> n){
        std::string origin_chars;
        for(int i=-128;i<128;i++){
            origin_chars+=char(i);
        }
        std::string target_chars=getPermutationAfterN(n,origin_chars);
        map charmap;
        for(int i=0;i<origin_chars.size();i++){
            charmap[origin_chars[i]]=target_chars[i];
        }
        return charmap;
    }
    map getCharsetsMap_decrypy(NCint<> n){
        std::string origin_chars;
        for(int i=-128;i<128;i++){
            origin_chars+=char(i);
        }
        std::string target_chars=getPermutationAfterN(n,origin_chars);
        map charmap;
        for(int i=0;i<origin_chars.size();i++){
            charmap[target_chars[i]]=origin_chars[i];
        }
        return charmap;
    }
    std::string encrypt(std::string data, std::string PassHash, int version = -1, bool crossDevices = false, bool showprocess = false, bool debug = false) {
        std::string OriginHash=crossDevices?"| Device ID: "+getDeviceUniqueID()+" ":""; // 跨设备则加入设备ID
        OriginHash+= "| PassHash: "+PassHash;
        OriginHash+= " | DataHash: "+GXPass::fullsafe(data,version,32,16);
        if(debug) std::cout<<"OriginHash: "<<OriginHash<<std::endl;
        std::string FinalPass=GXPass::fullsafe(OriginHash,version,256,256);
        if(debug) std::cout<<"FinalPass: "<<FinalPass<<std::endl;
        std::string numbercharset="123456789";
        NCint<> DictionaryLength=GXPass::fullsafe(FinalPass,version,2,8,numbercharset);
        if(debug) std::cout<<"Init Dictionary Length: "<<DictionaryLength<<std::endl;
        int dl=DictionaryLength;
        map charmap=getCharsetsMap_encrypy(GXPass::fullsafe(FinalPass,-1,600,16,numbercharset));
        std::string encrypted="";
        int stage=0;
        for(int i=0;i<data.size();i++){
            if(debug) std::cout<<"Stage "<<++stage<<": Dictionary Length "<<dl<<std::endl;
            int DictionryStopPoint=std::min(static_cast<int>(data.size()),i+dl);
            if(debug) std::cout<<"Encrypting data from index "<<i<<" to "<<DictionryStopPoint-1<<std::endl;
            for(;i<DictionryStopPoint;i++){
                if(debug) std::cout<<"Index "<<i<<"/"<<DictionryStopPoint-1<<": ("<<int(data[i])<<") -> ("<<int(charmap[data[i]])<<")"<<std::endl;
                encrypted+=charmap[data[i]];
            }
            i--;
            if(stage%100==0 && showprocess){
                std::cout<<"\rEncryption Progress: "<<(i+1)*100/data.size()<<"% ("<<(i+1)<<"/"<<data.size()<<")"<<std::endl;
            }
            NCint<> WindowSize=GXPass::fullsafe(FinalPass,-1,3,8,numbercharset);
            int ws=WindowSize;
            int ws_final=std::min(ws,i);
            std::string WindowData="";
            for(int j=i-ws_final;j<=i;j++){
                WindowData+=data[j];
            }
            DictionaryLength=GXPass::fullsafe(OriginHash+" | WindowData: "+WindowData,version,2,16,numbercharset);
            dl=DictionaryLength;
            if(debug) std::cout<<" WindowSize: "<<ws_final<<std::endl;
            if(debug) std::cout<<" Next Dictionary Length: "<<DictionaryLength<<std::endl;
        }
        return encrypted;
    }
    std::string decrypt(std::string data, std::string FinalPass, int version = -1, bool crossDevices = false, bool showprocess = false, bool debug = false) {
        if(debug) std::cout<<"FinalPass: "<<FinalPass<<std::endl;
        std::string numbercharset="123456789";
        NCint<> DictionaryLength=GXPass::fullsafe(FinalPass,version,2,8,numbercharset);
        if(debug) std::cout<<"Init Dictionary Length: "<<DictionaryLength<<std::endl;
        int dl=DictionaryLength;
        map charmap=getCharsetsMap_decrypy(GXPass::fullsafe(FinalPass,-1,600,16,numbercharset));
        std::string decrypted="";
        for(int i=0;i<data.size();i++){
            static int stage=0;
            if(debug) std::cout<<"Stage "<<++stage<<": Dictionary Length "<<dl<<std::endl;
            int DictionryStopPoint=std::min(static_cast<int>(data.size()),i+dl);
            if(debug) std::cout<<"Decrypting data from index "<<i<<" to "<<DictionryStopPoint-1<<std::endl;
            for(;i<DictionryStopPoint;i++){
                if(debug) std::cout<<"Index "<<i<<"/"<<DictionryStopPoint-1<<": ("<<int(data[i])<<") -> ("<<int(charmap[data[i]])<<")"<<std::endl;
                decrypted+=charmap[data[i]];
            }
            i--;
            if(stage%100==0 && showprocess){
                std::cout<<"\rDecryption Progress: "<<(i+1)*100/data.size()<<"% ("<<(i+1)<<"/"<<data.size()<<")"<<std::endl;
            }
            NCint<> WindowSize=GXPass::fullsafe(FinalPass,-1,3,8,numbercharset);
            int ws=WindowSize;
            int ws_final=std::min(ws,i);
            std::string WindowData="";
            for(int j=i-ws_final;j<=i;j++){
                WindowData+=decrypted[j];
            }
            DictionaryLength=GXPass::fullsafe(FinalPass+" | WindowData: "+WindowData,version,2,16,numbercharset);
            dl=DictionaryLength;
            if(debug) std::cout<<" WindowSize: "<<ws_final<<std::endl;
            if(debug) std::cout<<" Next Dictionary Length: "<<DictionaryLength<<std::endl;
        }
        return decrypted;
    }
    void makeencryptfile(std::string filename, std::string Pass, int version = -1, bool crossDevices = false, bool showprocess = false, bool debug = false) {
        std::string binaryCharset="";
        for(int i=-128;i<128;i++){
            binaryCharset+=char(i);
        }
        std::ifstream infile(filename, std::ios::binary);
        if(!infile.is_open()) {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return;
        }
        std::ofstream outfile(filename + ".GXF", std::ios::binary);
        if(!outfile.is_open()) {
            std::cerr << "Error opening file for writing: " << filename + ".GXF" << std::endl;
            return;
        }
        if(debug) std::cout<<"Reading file data..."<<std::endl;
        std::stringstream buffer;
        buffer << infile.rdbuf();
        infile.close();
        if(debug) std::cout<<"Encrypting data..."<<std::endl;
        std::string filedata = buffer.str();
        std::string PassHash=GXPass::fullsafe(Pass, version,256,256,binaryCharset);
        if(debug) std::cout<<"FinalPass: "<<PassHash<<std::endl;
        std::string encrypteddata = encrypt(filedata, PassHash, version, crossDevices, showprocess, debug);
        if(debug) std::cout<<"Writing encrypted file..."<<std::endl;
        std::string magicheader=GXPass::fullsafe("GXPassFileHeader",version,16,8,binaryCharset);
        std::string DataHash=GXPass::fullsafe(filedata,version,32,16);
        outfile << magicheader;// 32 bytes
        if(debug) std::cout<<"Magic Header: "<<magicheader<<std::endl;
        outfile << char(version);// 1 byte
        if(debug) std::cout<<"Version: "<<version<<std::endl;
        outfile << char(crossDevices?1:0);// 1 byte
        if(debug) std::cout<<"Cross Devices: "<<(crossDevices?"Yes":"No")<<std::endl;
        outfile << char(DataHash.size());// 1 byte
        if(debug) std::cout<<"Data Hash Size: "<<DataHash.size()<<std::endl;
        outfile << DataHash;// variable size
        if(debug) std::cout<<"Data Hash: "<<DataHash<<std::endl;
        if(debug) std::cout<<"Writing encrypted data..."<<std::endl;
        outfile << encrypteddata;// rest is encrypted data
        if(debug) std::cout<<"Encryption complete. Output file: "<<filename + ".GXF"<<std::endl;
        outfile.close();
    }
    void makedecryptfile(std::string filename, std::string Pass, bool showprocess = false, bool debug = false) {
        std::string binaryCharset="";
        for(int i=-128;i<128;i++){
            binaryCharset+=char(i);
        }
        std::ifstream infile(filename, std::ios::binary);
        if(!infile.is_open()) {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return;
        }
        std::stringstream buffer;
        buffer << infile.rdbuf();
        infile.close();
        std::string filedata = buffer.str();
        std::string magicheader=GXPass::fullsafe("GXPassFileHeader",-1,16,8,binaryCharset);
        for(int i=0;i<16;i++){
            if(filedata[i]!=magicheader[i]){
                std::cerr << "Invalid GXPass file header!" << std::endl;
                return;
            }
        }
        if(debug) std::cout<<"Valid GXPass file header found."<<std::endl;
        if(debug) std::cout<<"Reading file metadata..."<<std::endl;
        if(debug) std::cout<<"Version: "<<int(filedata[16])<<std::endl;
        int version=int(filedata[16]);
        bool crossDevices=(filedata[17]!=0);
        if(debug) std::cout<<"Cross Devices: "<<(crossDevices?"Yes":"No")<<std::endl;
        int DataHashSize=int(filedata[18]);
        if(debug) std::cout<<"Data Hash Size: "<<DataHashSize<<std::endl;
        std::string DataHash=filedata.substr(19,DataHashSize);
        if(debug) std::cout<<"Data Hash: "<<DataHash<<std::endl;
        int PassHashSize=int(filedata[19+DataHashSize]);
        if(debug) std::cout<<"Decrypting data..."<<std::endl;
        std::string encrypteddata=filedata.substr(19+DataHashSize);
        std::string PassHash=GXPass::fullsafe(Pass, version,256,256,binaryCharset);
        std::string OriginPass="";
        if(crossDevices){
            OriginPass+="| Device ID: "+getDeviceUniqueID()+" ";
        }
        OriginPass+= "| PassHash: "+PassHash;
        OriginPass+= " | DataHash: "+DataHash;
        if(debug) std::cout<<"OriginPass: "<<OriginPass<<std::endl;
        std::string FinalPass=GXPass::fullsafe(OriginPass,version,256,256);
        if(debug) std::cout<<"FinalPass: "<<FinalPass<<std::endl;
        std::string decrypteddata = decrypt(encrypteddata, FinalPass, version, crossDevices, showprocess, debug);
        std::string computedDataHash=GXPass::fullsafe(decrypteddata,version,32,16);
        if(debug) std::cout<<"Computed Data Hash: "<<computedDataHash<<std::endl;
        if(computedDataHash!=DataHash){
            std::cerr << "Data integrity check failed! The data may be corrupted or the password is incorrect." << std::endl;
            return;
        }
        std::ofstream outfile(filename.substr(0, filename.find_last_of('.')), std::ios::binary);
        if(!outfile.is_open()) {
            std::cerr << "Error opening file for writing: " << filename.substr(0, filename.find_last_of('.')) << std::endl;
            return;
        }
        if(debug) std::cout<<"Writing decrypted file..."<<std::endl;
        outfile << decrypteddata;
        if(debug) std::cout<<"Decryption complete. Output file: "<<filename.substr(0, filename.find_last_of('.'))<<std::endl;
        outfile.close();
    }
} // namespace GXPass

#endif // __GXPASS_HPP__
