//
// Created by SheepCore on 2019-06-25.
//

#ifndef FILESYSTEM_USER_H
#define FILESYSTEM_USER_H
#include "Directory.h"
/*组织多个用户的登录活动*/
//用户的初始化操作为完成，最好格式化磁盘时指定登录名，在第0块存放用户信息
class User
{
public:
    friend class FSystem;
    User();
    ~User();
    Directory dir;

    bool logIn(const char name[16],const char psw[16], unsigned &userID);//登录,查找用户是否存在

    bool logOut();//退出，保存用户相关信息


    bool forkUser(const char username[], const char password[], unsigned& userID);

    bool updateNamePsw(char username[], char password[]);//设置用户名和密码

    bool modifyPassword(char *password);

    bool modifyUsername(char *username);

    unsigned getCurUserID();//获得当前用户的ID号

    void showLogInfo();

    bool delUser(const char *username);

    bool delUser(unsigned int userId);


private:
    User_M admin;//user的信息

    bool checkUsername(const char *username);


    unsigned int findUser(const char *username);


};
#endif //FILESYSTEM_USER_H
