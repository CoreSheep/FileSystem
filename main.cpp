#include <iostream>
//
// Created by SheepCore on 2019-06-22.
//
#include "src/source/FSystem.h"
struct record {
    char name[10];
    int age;
};

void testDiskAlloc() {
    printf("testDiskAlloc\n");
    Directory dir;
    int choose=1;
    unsigned No = 1;

    while (cin>>choose)///
    {
        switch (choose)
        {
            case 1://aB
                dir.disk.allocBlock(No);
                break;
            case 2://fB
                cin >> No;
                dir.disk.freeBlock(No);
                break;
            case 3://aI
                dir.disk.allocINode(No);
                break;
            case 4://fI
                cin >> No;
                dir.disk.freeINode(No);
                break;
            default:
                break;
        }
        cout << No << endl;
    }
}

void fileTest() {
    FILE *fp = fopen("ok", "rb+");
    unsigned a[100];
    for (int i = 0; i < 100; i++) {
        a[i] = i;
    }
    fseek(fp, 4 * 100, SEEK_SET);
    fwrite(a, sizeof(a), 1, fp);
    fclose(fp);
    FILE *fp2 = fopen("ok", "rb+");
    fseek(fp2, 4 * 100, SEEK_SET);
    unsigned b[100];
    fread(b, sizeof(b), 1, fp2);
    for (int i = 0; i < 100; i++) {
        cout << b[i] << " ";
    }
    fclose(fp2);
}


void testFormat() {
    Directory dir;
    dir.format();
    unsigned a[101];
    for (int i = 0; i < 101; i++) {
        a[i] = i;
    }
    int b[101];
    readINode(4, b,sizeof(b));
    for (int i = 0; i < 101; i++) {
        printf("%d ", b[i]);
    }
}


void testMKdir() {
    Directory dir;
    dir.mkdir_touch("cz", 1);
    //dir.mkdir("Lisi", dir.disk.rootNode);
    dir.saveParABuf();
    //cout<<dir.findDir("cz")<<endl;
    //cout << dir.disk.rootINode.lastBlockUsedByte<<endl;
    //dir.disk.format();
}
void test() {
    string a;
    char b[18] = "you are a fool";
    a = b;
    a = b;
}


void testStr() {
    string a;
    getline(cin,a);
    char b[1025];
    scanf("%s", b);
    a.append(b);
    a.append(b);
    a.append(b);
    //cout << b << endl;
}


void testDir() {
    Directory dir;
    dir.showDir();
    int choose;
    while (cin >> choose && choose != 0)
    {
        string a; int type;
        switch (choose)
        {
            case 1://新建目录
                cin >> a;
                dir.mkdir_touch(a.c_str(), 1);
                break;
            case 2://cd
                cin >> a;
                dir.cd(a.c_str());
                break;
            case 3://修改权限
                cin >> a >> type;
                dir.chmod(a.c_str(), (ModeType)type);
                break;
            case 4://删除文件
                cin >> a;
                dir.rmdir_file(a.c_str());
                break;
            case 5://新建文件
                cin >> a;
                dir.mkdir_touch(a.c_str(), 0);
                break;
            case 6://显示信息
                cin >> a;
                dir.showInfo(a.c_str());
                break;
            case 7://格式化
                dir.format();
                break;
            default:
                break;
        }
        dir.showDir();
    }dir.showDir();
}

//void testForkUser(){
//    System sys;
//    User user;
//    sys.createUser(user);
//
//}

void testSystem() {
    string cmd,name;
    System fs;
    unsigned iNo=0;
    while (cin >> cmd) {
        if (cmd=="open") {
            cin >> name;
            fs.openFile(name.c_str(),iNo);//open时还需要检查该文件所属用户及其权限
        }
        else if (cmd=="close") {
            fs.closeFile(iNo);
        }
        else if (cmd == "read") {
            name.clear();
            fs.readFile(iNo, name);
            printf("\nread!\n%s\n", name.c_str());
        }
        else if (cmd == "write") {
            name.clear();
            for (unsigned i = 0; i < 1024 * 263; i++) {
                char a[2] = "0";
                a[0] += (i%10);
                name.append(a);
            }
            fs.writeData(iNo,name);
        }
        else if (cmd == "touch") {
            cin >> name;
            fs.user.dir.mkdir_touch(name.c_str(),0);
        }
        else if (cmd == "format") {
            fs.user.dir.format();
        }
        fs.user.dir.showDir();
    }
}



void testUser(){
    System system;
    unsigned userId;
    system.user.dir.disk.setUserNum(1);
    system.traverseUsers();
    system.user.showLogInfo();
    system.user.forkUser("ljf", "119", userId);
    system.user.logIn("admin", "123", userId);
    system.traverseUsers();
    system.user.delUser(1);
    system.traverseUsers();


}

int main(void){
    testUser();
}
