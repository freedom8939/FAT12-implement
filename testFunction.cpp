#include "bits/stdc++.h"

using namespace std;

int main() {
    string str = "cd data";
    string new_str = str.substr(3);
    cout << new_str << endl;
    cout << new_str.length() << endl;

    cout << "--------------" << endl;
    char d = tolower('A');
    cout << d << endl;
    cout << "--------------" << endl;


    char source[13]= "Hello, World";
    char des[] = "";
    memcpy(des,source, strlen(source) + 1); //把 '/0'带着
    cout << des << endl;
    cout << "--------------" << endl;

    char a[8]= "1234567";
    cout << strlen(a) <<endl; //7
    cout << sizeof(a) <<endl; // 结果是 8


    //
    uint8_t bbb[9] = "12345678"; //能存8个 a[0] ~ a[7] a[8] = '/0'
//    char *ab = "12345678";
    cout << "--------------" << endl;

    // 将字符数组转换为字符串
    std::string str1(reinterpret_cast<char*>(bbb));

    cout << str1;
    cout << "--------------" << endl;



    return 0;
}