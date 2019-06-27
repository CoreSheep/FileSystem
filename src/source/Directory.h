//
// Created by SheepCore on 2019-06-25.
//

#ifndef FILESYSTEM_DIRECTORY_H
#define FILESYSTEM_DIRECTORY_H
/*目前战略方位，先用直接寻址法做一个能用的系统*/
/*管理目录和文件（树形文件结构,节点的管理）
*/
#include "Disk.h"
class Directory
{
public:
    friend class User;
    friend class FSystem;
    Disk disk;
    Directory();
    ~Directory();
    bool mkdir_touch(const char dirName[28],bool type);//在父文件夹创建子文件夹或文件，涉及混合索引
    bool chmod(const char fileName[28], ModeType mod);//修改文件权限
    bool rmdir_file(const char fileName[28]);//删除文件夹，级联删除
    bool cd(const char fileName[28]);//改变目录，需保存缓存内容及父节点信息，设置新的父节点
    void showInfo(const char name[28]);//显示文件信息
    void showDir();//目录显示
    DINode getParentNode() {
        return parentINode;
    }
    //当用户转到其他目录或退出时，需要保存内存的内容到磁盘(父节点信息和缓存)
    bool saveParABuf();//保存父节点和缓存内容到磁盘
    bool format();//磁盘格式化
    bool fastTouch();//创建快捷方式
    void setUserID(unsigned id) {
        userID = id;
    }
    bool findDir(const char name[28], unsigned &iNodeNo);
private:
    unsigned userID;
    struct BlockBuf {//建立磁盘块缓冲区，写满时送回相应磁盘块
        FileDir filedir[DIR_ITEMNUM];
        int used = 0;//缓冲数组使用情况
        size_t write(FILE *fp) {//将缓冲区写入内存
            return fwrite(filedir, sizeof(filedir),1,fp);
        }
        bool reset(DINode parINode) {
            bool rel=readBlock(parINode.getCur(), filedir, sizeof(filedir));
            used = (parINode.lastBlockUsedByte / 32);
            return rel;
        }
    }m_dirBuf;
    DINode parentINode;//整个Directory在此父级目录中操作
    vector<string> nameStk;//存放父级目录名
    bool findDir(const char name[28]);//在当前父目录下查找名字为name一项

    bool getDirNo(unsigned &dirNo);//获得当前可用目录项
    bool mkdir();//为新建目录项分配I节点和Block
    bool touch();//为新建文件项分配I节点
    bool delItem(const char name[28], unsigned &iNodeNo);//删除父目录下的目录项，成功则返回删除项的I节点号
    bool recurDel(DINode parNode);//递归删除
    bool priorCheck(const DINode &iNode) {
        if (iNode.parentINo != -1 && iNode.userID == userID) {
            return true;
        }
        return false;
    }
};
#endif //FILESYSTEM_DIRECTORY_H
