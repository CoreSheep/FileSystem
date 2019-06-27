//
// Created by SheepCore on 2019-06-25.
//

#ifndef FILESYSTEM_FSYSTEM_H
#define FILESYSTEM_FSYSTEM_H
#include "define.h"
#include "User.h"
#include <cstring>
using namespace std;
//System 完成对用户的管理，和内存I节点的管理
/**/
class System
{
public:
    System();
    ~System();
    //vector<User> users;//多用户
    User user;//用户
    bool closeFile(unsigned iNo);//若以写方式打开需要写回文件
    bool openFile(const char fileName[28], unsigned &iNo);//打开成功，返回I节点编号
    bool readFile(unsigned iNo, string &data);//从文件读出数据
    bool writeData(unsigned iNo, const string &data);//将数据写入磁盘
    bool createUser(User_D &newUser);//创建一个用户
    bool login(const char name[16],const char psw[16], unsigned &userID);
    bool logout(const char name[16],const char psw[16], unsigned &userID);
    bool changeUser(User &otherUser);//切换用户
    void traverseUsers();
private:
    MINode *hash[128];//hash表
    MINode *ihash(unsigned no);//获得内存I节点所在链表
    MINode* miGet(unsigned iNo);//获得内存I节点,if not exist return NULL
    void miPut(unsigned iNo, MINode *mi);//添加内存I节点
    void miDel(unsigned iNo, MINode *mi);
    //仅打开后才可读写
    bool saveMINode();//将所有已修改的内存I节点保存至磁盘
    void freeOldBlock(const DINode &di);

};
#endif //FILESYSTEM_FSYSTEM_H
