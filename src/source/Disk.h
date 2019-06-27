//
// Created by SheepCore on 2019-06-25.
//

#ifndef FILESYSTEM_DISK_H
#define FILESYSTEM_DISK_H

#include "define.h"
/*对所有磁盘块的回收和分配进行统一的管理
*/
class Disk
{
public:
    friend class Directory;
    bool allocINode(unsigned &iNoderNo);//分配一块I节点块
    bool allocBlock(unsigned &blockNo);//分配磁盘块
    bool freeINode(unsigned iNoderNo);//分配一块I节点块
    bool freeBlock(unsigned blockNo);//分配磁盘块
    static bool isExist(const char name[28]);//判断文件是否存在
    bool saveSuper();//保存所有在内存中未写回磁盘的数据
    unsigned getUserNum() {
        return superBlk.userNum;
    }
    void setUserNum(unsigned num) {
        superBlk.userNum = num;
        saveSuper();
    }
    Disk();
    ~Disk();
    void initialization() {//disk类的初始化
        strcpy(rootName, "root");
        if (!isExist("system")) {//不存在时格式化
            format();
        }
        else//0存在
            loadDisk();
    }
private:
    char rootName[28];
    bool format();//创建成组链接空闲块、空闲I节点、初始化根目录
    DINode rootINode;
    bool loadDisk();//加载信息
    SuperBlock superBlk;
    bool initSuperBlk(unsigned  &);
    bool initSuperINode(unsigned &);

    //磁盘块和I节点只是占用磁盘大小不一样，其他分配逻辑应该差不多。需要掌握fwrite、fread、fseek

};
#endif //FILESYSTEM_DISK_H
