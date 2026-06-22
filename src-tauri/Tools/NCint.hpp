// NCint v1.0.0 See more information at https://github.com/Necream/NCint

#ifndef __NCINT_HPP__
#define __NCINT_HPP__

#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#define MaxN(a,b) (a > b ? a : b)
#define MinN(a,b) (a < b ? a : b)

template <typename T = int>
struct NCint {
    private:
        //data[0] sign and length
        //data[1..length] number
        std::vector<T> data;
    public:
        NCint(){
            data.resize(1);
            data[0] = 0;
        }
        NCint(const NCint<T>& other){
            data = other.data;
        }
        NCint(const int& val){
            *this = val;
        }
        NCint(const long long& val){
            *this = val;
        }
        NCint(const std::string& str){
            *this = str;
        }
        operator int()const{
            if(data[0]>0){
                int ret = 0;
                for(T i = data[0]; i > 0; --i){
                    ret = ret * 10 + data[i];
                }
                if(data[0] < 0) ret = -ret;
                return ret;
            }else{
                int ret = 0;
                for(T i = data[0]; i < 0; ++i){
                    ret = ret * 10 + data[-i];
                }
                if(data[0] < 0) ret = -ret;
                return ret;
            }
        }
        operator long long()const{
            if(data[0]>0){
                long long ret = 0;
                for(T i = data[0]; i > 0; --i){
                    ret = ret * 10 + data[i];
                }
                if(data[0] < 0) ret = -ret;
                return ret;
            }else{
                long long ret = 0;
                for(T i = data[0]; i < 0; ++i){
                    ret = ret * 10 + data[-i];
                }
                if(data[0] < 0) ret = -ret;
                return ret;
            }
        }
        operator std::string()const{
            if(data[0]==0) return "0";
            std::string ret;
            short sign = (data[0]<0)?-1:1;
            if(data[0] < 0) ret += '-';
            for(T i = data[0]*sign; i > 0; --i){
                ret += char(data[i] + '0');
            }
            return ret;
        }
        NCint& operator =(const short& val){
            data.clear();
            data.push_back(0);
            bool sign = (val<0)?true:false;
            int absval = (sign)?-(int)(val):(int)(val);
            if(absval == 0){
                return *this;
            }
            while(absval > 0){
                data.push_back(absval % 10);
                absval /= 10;
            }
            data[0] = sign ? -(data.size()-1) : (data.size()-1);
            return *this;
        }
        NCint& operator =(const int& val){
            data.clear();
            data.push_back(0);
            bool sign = (val<0)?true:false;
            long long absval = (sign)?-(long long)(val):(long long)(val);
            if(absval == 0){
                return *this;
            }
            while(absval > 0){
                data.push_back(absval % 10);
                absval /= 10;
            }
            data[0] = sign ? -(data.size()-1) : (data.size()-1);
            return *this;
        }
        NCint& operator =(const long long& val){
            std::stringstream sin;
            sin<<val;
            std::string str;
            sin>>str;
            *this = str;
            return *this;
        }
        NCint& operator =(const std::string& str){
            data.clear();
            data.push_back(0);
            bool sign = false;
            size_t start = 0;
            if(str[0] == '-'){
                sign = true;
                start = 1;
            }
            for(size_t i = str.size(); i > start; --i){
                data.push_back(T(str[i-1]-'0'));
            }
            data[0] = sign ? -(data.size()-1) : (data.size()-1);
            return *this;
        }
        friend std::istream& operator>>(std::istream& in, NCint<T>& nci){
            std::string s;
            in >> s;
            nci=s;
            return in;
        }
        friend std::ostream& operator<<(std::ostream& out, const NCint<T>& nci){
            out << std::string(nci);
            return out;
        }
        NCint operator -()const{
            NCint<T> ret = *this;
            ret.data[0] = -ret.data[0];
            return ret;
        }
        bool operator <(NCint<T> const& other)const{
            if(data[0] != other.data[0]){
                return data[0] < other.data[0];
            }
            if(data[0] < 0) return (-other) < (-(*this));
            if(data[0] == 0) return false;
            // same sign and length
            for(T i = data[0]; i > 0; --i){
                if(data[i] < other.data[i]) return true;
                if(data[i] > other.data[i]) return false;
            }
            return false;
        }
        bool operator >(NCint<T> const& other)const{
            return other < (*this);
        }
        bool operator <=(NCint<T> const& other)const{
            return !((*this) > other);
        }
        bool operator >=(NCint<T> const& other)const{
            return !((*this) < other);
        }
        bool operator ==(NCint<T> const& other)const{
            return data == other.data;
        }
        NCint operator +(const NCint<T>& other)const{
            if(other.data[0]==0){
                return *this;
            }
            if(data[0]==0){
                return other;
            }
            NCint<T> other_Temp = other;
            if((*this)<other_Temp){
                return other_Temp + (*this);
            }
            if(data[0]<0 && other_Temp.data[0]<0){
                return -(( -(*this)) + (-other_Temp));
            }
            if((*this)<(-other_Temp)){
                return -(( -other_Temp) + (-(*this)));
            }
            // caculate (*this) + other
            if(other_Temp.data[0] < 0){
                // (*this) >= (-other)
                NCint<T> ret=*this;
                other_Temp.data.resize(data[0]+1);
                for(T i=1; i<=data[0]; ++i){
                    ret.data[i] -= other_Temp.data[i];
                }
                for(T i=1; i<=data[0]; ++i){
                    while(ret.data[i]<0){
                        ret.data[i]+=10;
                        --ret.data[i+1];
                    }
                }
                for(T i=data[0]; i>0; --i){
                    if(ret.data[i]>0){
                        ret.data[0]=i;
                        break;
                    }
                }
                return ret;
            }else{
                NCint <T> ret=*this;
                ret.data.resize(MaxN(data[0], other_Temp.data[0])+2);
                for(T i=1; i<=other_Temp.data[0]; ++i){
                    ret.data[i] += other_Temp.data[i];
                }
                for(T i=1; i<=MaxN(data[0], other_Temp.data[0]); ++i){
                    while(ret.data[i]>=10){
                        ret.data[i]-=10;
                        ++ret.data[i+1];
                    }
                }
                for(T i=MaxN(data[0], other_Temp.data[0])+1; i>0; --i){
                    if(ret.data[i]>0){
                        ret.data[0]=i;
                        break;
                    }
                }
                return ret;
            }
        }
        NCint operator -(const NCint<T>& other)const{
            return (*this) + (-other);
        }
        NCint operator *(const NCint<T>& other)const{
            if(data[0]==0 || other.data[0]==0){
                return NCint<T>(0);
            }
            if(data[0]<0 && other.data[0]<0){
                return (-(*this))*(-other);
            }else if(data[0]<0 || other.data[0]<0){
                return -(( -(*this)) * other);
            }
            NCint<T> ret;
            ret.data.resize(data[0]+other.data[0]+2);
            for(T i=1; i<=data[0]; ++i){
                for(T j=1; j<=other.data[0]; ++j){
                    ret.data[i+j-1] += data[i]*other.data[j];
                }
            }
            for(T i=1; i<=data[0]+other.data[0]; ++i){
                while(ret.data[i]>=10){
                    ret.data[i]-=10;
                    ++ret.data[i+1];
                }
            }
            for(T i=data[0]+other.data[0]+1; i>0; --i){
                if(ret.data[i]>0){
                    ret.data[0]=i;
                    break;
                }
            }
            if((data[0]<0)^(other.data[0]<0)){
                ret.data[0] = -ret.data[0];
            }
            return ret;
        }
        NCint& operator +=(const NCint<T>& other){
            *this = (*this) + other;
            return *this;
        }
        NCint& operator -=(const NCint<T>& other){
            *this = (*this) - other;
            return *this;
        }
        NCint& operator *=(const NCint<T>& other){
            *this = (*this) * other;
            return *this;
        }
        NCint operator/(const NCint<T>& other) const {
            if (other.data[0] == 0) throw std::runtime_error("Division by zero");

            // 特判 1 和 -1
            if (other == NCint<T>(1)) return *this;
            if (other == NCint<T>(-1)) return -(*this);

            NCint<T> dividend = *this;
            NCint<T> divisor = other;
            bool negative = false;

            if (dividend.data[0] < 0) {
                negative = !negative;
                dividend = -dividend;
            }
            if (divisor.data[0] < 0) {
                negative = !negative;
                divisor = -divisor;
            }

            if (dividend < divisor) return NCint<T>(0);

            // 商和余数
            NCint<T> quotient(0);
            NCint<T> remainder(0);

            int len = dividend.data[0];
            for (int i = len; i >= 1; --i) {
                // 向余数追加当前位
                remainder = remainder * (NCint<T>)10 + NCint<T>(dividend.data[i]);
                int q = 0;
                while (remainder >= divisor) {
                    remainder -= divisor;
                    ++q;
                }
                quotient.data.insert(quotient.data.begin() + 1, q); // 先插入高位
            }

            // 调整长度和符号
            int qlen = quotient.data.size() - 1;
            while (qlen > 0 && quotient.data[qlen] == 0) --qlen;
            quotient.data.resize(qlen + 1);
            quotient.data[0] = negative ? -qlen : qlen;

            return quotient;
        }
        NCint& operator /=(const NCint<T>& other){
            *this = (*this) / other;
            return *this;
        }
        NCint operator%(const NCint<T>& other) const {
            NCint<T> quotient = (*this) / other;
            NCint<T> product = quotient * other;
            NCint<T> remainder = (*this) - product;
            return remainder;
        }
};

#endif // __NCINT_HPP__