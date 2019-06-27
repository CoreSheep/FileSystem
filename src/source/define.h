//
// Created by SheepCore on 2019-06-25.
//

#ifndef FILESYSTEM_DEFINE_H
#define FILESYSTEM_DEFINE_H
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <string>
#include <fstream>
using namespace std;

/*
系统内部这个过程分成三步：
首先，系统找到这个文件名对应的inode号码；
其次，通过inode号码，获取inode信息；
最后，根据inode信息，找到文件数据所在的block，读出数据
*/

//单位为字节
#define DSK_MAX_NUM 2000  //磁盘块总数

#define BLOCK_SIZ   1024		//每块大小1KB

#define SPR_MAX_BLK  100	//超级块中空闲块的最大数
#define SPR_MAX_INODE 15	//超级块中空闲I节点的最大数

#define DSK_INODE_SIZ 64	//每个磁盘i节点所占字节
#define DSK_INODE_BLK 64	//所有磁盘i节点共占64个物理块

#define FREE_INODE_NUM		(DSK_INODE_BLK*(BLOCK_SIZ/DSK_INODE_SIZ)) //可用I节点数
#define FREE_BLK_NUM		(DSK_MAX_NUM-DSK_INODE_BLK-2)

#define IMAX_ADDR 6		//每个i节点最多指向4+256+256*256块，addr[0]~addr[5]，[0-3]:直接寻址，[4]一次间接寻址，[5]二次间接寻址

#define INODE_START (2*BLOCK_SIZ)	//i节点起始地址
#define BLOCK_START ((2+DSK_INODE_BLKNUM)*BLOCK_SIZ) //目录、文件区起始地址


#define DIR_ITEMSIZ  32		//每个目录项名字部分所占字节数，另加i节点号2个字节
#define DIR_ITEMNUM  (BLOCK_SIZ/DIR_ITEMSIZ)	//每个目录块所包含的最大目录项数（文件数）

#define SYS_MAX_OPEN 40		//系统打开文件表最大项数
#define USER_MAX_OPEN 20	//每个用户最多可打开20个文件，即用户打开文件最大次数

#define PWD_SIZ   12		//用户密码长度
#define PWD_NUM   16		//最多可设16个口令登录
#define USER_NUM  16		//最多允许16个用户登录

#define HASH_IDXNUM  128	//共128个Hash链表，提供索引i节点（必须为2的幂）
#define USED_SIZ 64         //用户信息占用字节数
#define UNSIGN_MAX	(1<<31)
enum ModeType {//文件的操作权限
    P_WRITE, P_READ, P_EXE, P_RW,
    S_WRITE, S_READ, S_EXE, S_ALL
};
const string modetype[] = { "ReadWrite","Read Only","Exe","All" };
enum BlockState {//磁盘块状态
    Empty0, Occupied0, Using0, Other0
};
enum BlockType {//磁盘块类型
    SuperBlock0, INode0, DirBlock0, FileBlock0
};
const unsigned oneNum = IMAX_ADDR - 1;
const unsigned twoNum = oneNum + 256;
bool readBlock(unsigned blockNo, void * buffer, size_t size);

struct DINode {//磁盘I节点，最大允许64B
    unsigned fileType;//文件类型（0:文件, 1:文件夹）
    unsigned blockNum;//记录占用了多少块
    unsigned lastBlockUsedByte;//最后一块已使用字节数
    unsigned userID;//所属用户号（该文件供哪个用户使用）

    unsigned parentINo;//父亲I节点号
    unsigned iNodeNo;//I节点号,在调至内存时可能批量写回
    ModeType priorMode;//访问权限，私有/公有*读/写/读写(默认拥有所有权限)
    unsigned linkCount=1;//链接计数,便于文件共享

    unsigned addr[IMAX_ADDR];
    //unsigned groupID;//所属组号（该文件供哪个用户组使用）
    //DINode(){}
    DINode():fileType(1),parentINo(-1),iNodeNo(0){}
    DINode(unsigned type, unsigned parentINo,unsigned iNodeNo,unsigned userID=0,
           unsigned blkNum=1, unsigned used=2*DIR_ITEMSIZ,ModeType mode=P_RW,unsigned linkCount=1):
            fileType(type),parentINo(parentINo),iNodeNo(iNodeNo),userID(userID),
            blockNum(blkNum),lastBlockUsedByte(used),priorMode(mode),linkCount(linkCount){}
    unsigned getCur() {//获得当前该I节点正在使用的磁盘号
        if (blockNum < oneNum) {
            return addr[blockNum - 1];
        }
        else if (blockNum < twoNum) {
            unsigned filedir[BLOCK_SIZ/4];
            readBlock(addr[4], filedir, sizeof(filedir));
            return filedir[blockNum-4-1];
        }
        else//存在严重危险
        {
            unsigned twoLeft = blockNum - 4 - 256;
            unsigned k = (twoLeft-1) / 256;
            unsigned buf1[256];
            unsigned buf2[256];
            readBlock(addr[5],buf1,sizeof(buf1));
            readBlock(buf1[k],buf2,sizeof(buf2));
            return buf2[twoLeft - k * 256 - 1];
        }
        return -1;
    }
};
struct MINode {//内存I节点
    //struct MINode *forw=NULL;//链表指针
    struct MINode *back=NULL;//双向链表
    unsigned mi_count;//内存节点访问计数
    unsigned state;//内存I节点状态，判断是否被加锁,是否修改等信息
    DINode mi_dinode;//磁盘I节点
};
struct FileDir {//文件目录项
    unsigned iNodeNo;
    char fileName[28];
    //unsigned i_addr;
};
struct SuperBlock
{
    unsigned size=DSK_MAX_NUM;//整个磁盘的大小
    unsigned userNum = 1;
    unsigned freeBlockNum=DSK_MAX_NUM-DSK_INODE_BLK-2;
    unsigned freeINodeNum=FREE_INODE_NUM;
    unsigned blockStack[SPR_MAX_BLK+1];//磁盘块的超级块
    unsigned iNodeStack[SPR_MAX_INODE+1];//I节点的超级块
};
struct User_M {
    unsigned openNum=0;//打开的文件数
    unsigned userID=-1;
    char userName[28];
    //unsigned groupID;
    char password[28];
    unsigned openINo[USER_MAX_OPEN];//打开的文件
};//分配128B
struct User_D
{
    unsigned userID = 0;
    char userName[28];
    char password[28];
    void setInfo(unsigned id,const char name[28],const char psw[28]) {
        userID = id;
        strcpy(userName, name);
        strcpy(password, psw);
    }
};//分配64B
/*struct Password {
	unsigned  pUserID;
	unsigned  pGroupID;
	char password[PWD_SIZ];
};*/
extern BlockState ui_blockstate[DSK_MAX_NUM];//磁盘块状态数组，便于可视化
///下面是磁盘操作的快捷方法
bool m_fseek(FILE *m_stream, long offset, int origin = SEEK_SET);

bool write(unsigned offset, const void * buffer, size_t size);

bool read(unsigned offset, void * buffer, size_t size);

bool writeBlock(unsigned blockNo, const void * buffer, size_t size);

bool readBlock(unsigned blockNo, void * buffer, size_t size);

bool writeINode(unsigned iNodeNo, const void * buffer, size_t size);

bool readINode(unsigned iNodeNo, void * buffer, size_t size);


#endif //FILESYSTEM_DEFINE_H
