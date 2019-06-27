//
// Created by SheepCore on 2019-06-25.
//

#include "User.h"
User::User(){
    User_D curUser;
    read(0, &curUser, sizeof(User_D));
    strcpy(admin.userName, curUser.userName);
    strcpy(admin.password, curUser.password);
    admin.userID = curUser.userID;
    dir.setUserID(admin.userID);
}

User::~User(){
    logOut();
}

bool User:: forkUser(const char username[], const char password[], unsigned& userID){
    if(admin.userID != 0){
        return false;
    }
    unsigned CurUserNum = dir.disk.getUserNum();
    if(CurUserNum >= USER_NUM){
        printf("Cannot create a new user.\n");
        return false;
    }
    User_D newUser;
    //保存用户信息
    newUser.setInfo(CurUserNum, username, password);
    userID = CurUserNum;
    dir.disk.setUserNum(CurUserNum + 1);
    write(CurUserNum * sizeof(User_D), &newUser, sizeof(newUser));
    return true;
}


bool User:: logIn(const char name[16],const char psw[16], unsigned &userID) {//登录,查找用户是否存在

    unsigned CurUserNum = dir.disk.getUserNum();
    User_D UserBuffer[USER_NUM];
    read(0, &UserBuffer, sizeof(UserBuffer));
    for(unsigned i = 0; i < CurUserNum; i++){
        if(strcmp(UserBuffer[i].userName, name) == 0){
            if(strcmp(UserBuffer[i].password, psw) == 0){
                printf("\nLog in successfully.\n\n");
                userID = UserBuffer[i].userID;
                admin.userID = userID;
                strcpy(admin.userName, name);
                strcpy(admin.password, psw);
                return true;
            }
        }
    }
    userID = -1;
    cout << "\nLogin unsuccessfully." << endl;
    cout << "No such user:\t" << name << "\n\n";
    return false;
}


bool User:: logOut() {
    ///用户注销
    ///保存当前用户信息
    //保存User_D
    User_D curUser;
    strcpy(curUser.userName, admin.userName);
    strcpy(curUser.password, admin.password);
    curUser.userID = admin.userID;
    //定位用户区所在地址
    write(admin.userID * sizeof(User_D), &curUser, sizeof(User_D));
    cout << "Logout successfully.\n\n";
    return true;
}

bool User:: delUser(const char username[]){
    //no authority
    if(admin.userID != 0){
        cout << "Adminster only!\n";
        return false;
    }
    //illegal username
    if(!checkUsername(username)){
        cout << "No such user.\n";
        return false;
    }
    unsigned delUserId = findUser(username);
    unsigned curUserNum = dir.disk.getUserNum();
    if(delUserId == curUserNum - 1){
        dir.disk.setUserNum(curUserNum - 1);
        return true;
    }
    if(delUserId == 0){
        cout << "Cannot delete administer.\n";
        return false;
    }
    User_D lastUser;
    read((curUserNum - 1) * sizeof(User_D), &lastUser, sizeof(User_D));
    write(delUserId * sizeof(User_D), &lastUser, sizeof(User_D));
    dir.disk.setUserNum(curUserNum - 1);
    return true;
}


bool User::delUser(unsigned userId) {
    //Delete operation is limited to administer.
    if(admin.userID != 0){
        cout << "Administer only!\n";
        return false;
    }
    //index out of bounds
    unsigned curUserNum = dir.disk.getUserNum();
    if(userId >= dir.disk.getUserNum()){
        cout << "Illegal userId!\n";
        return false;
    }
    if(userId == curUserNum - 1){
        dir.disk.setUserNum(curUserNum - 1);
        return true;
    }
    User_D lastUser;
    read((curUserNum - 1) * sizeof(User_D), &lastUser, sizeof(User_D));
    write(userId * sizeof(User_D), &lastUser, sizeof(User_D));
    dir.disk.setUserNum(curUserNum - 1);
    return true;

}

void User:: showLogInfo(){
    ///显示用户基本登录信息
    cout << "username:\t" << admin.userName << endl;
    cout << "userId:\t\t" << admin.userID << endl;
    cout << "password:\t" << admin.password << endl;

}

bool User:: modifyPassword(char password[]){
    strcpy(admin.password, password);
    User_D curUser;
    curUser.setInfo(admin.userID, admin.userName, admin.password);
    return true;
}

bool User:: modifyUsername(char username[]){
    if(!checkUsername(username))
        return false;
    strcpy(admin.userName, username);
    User_D curUser;
    curUser.setInfo(admin.userID, admin.userName, admin.password);
    return true;
}

bool User:: updateNamePsw(char username[], char password[]) {
    ///用户修改或更新密码
    if(!checkUsername(username))
        return false;
    strcpy(admin.userName, username);
    strcpy(admin.password, password);
    User_D curUser;
    curUser.setInfo(admin.userID, admin.userName, admin.password);
    return true;

}


unsigned User:: getCurUserID() {//获得当前用户的ID号
    return admin.userID;
}


bool User::checkUsername(const char *username) {
    unsigned CurUserNum = dir.disk.getUserNum();
    //read all users into a temp buffer
    User_D UserBuffer[USER_NUM];
    //seek to administer
    read(0, &UserBuffer, sizeof(UserBuffer));
    for (unsigned i = 0; i < CurUserNum; ++i) {
        if(strcmp(username, UserBuffer[i].userName) == 0)
            return true;
    }
    return false;
}

unsigned User::findUser(const char *username){
    unsigned CurUserNum = dir.disk.getUserNum();
    //read all users into a temp buffer
    User_D UserBuffer[USER_NUM];
    //seek to administer
    read(0, &UserBuffer, sizeof(UserBuffer));
    for (unsigned i = 0; i < CurUserNum; ++i) {
        if(strcmp(username, UserBuffer[i].userName) == 0)
            return UserBuffer[i].userID;
    }
    return 0;
}





