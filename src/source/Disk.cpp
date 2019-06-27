//
// Created by SheepCore on 2019-06-25.
//
#include "Disk.h"
Disk::Disk() :rootINode(1, -1, 0){
    initialization();
}


Disk::~Disk(){
    saveSuper();//保存超级块中的内容
}


bool Disk::initSuperBlk(unsigned &initLeader){
    superBlk.size = DSK_MAX_NUM;//整个磁盘的大小
    superBlk.freeBlockNum = DSK_MAX_NUM - DSK_INODE_BLK - 2;
    superBlk.freeINodeNum = FREE_INODE_NUM;
    //超级块中磁盘块的空间管理初始化
    unsigned restNum = (FREE_BLK_NUM) % 100 + 1;//计算在超级块中的剩余块
    initLeader = DSK_INODE_BLK + restNum + 1;//计算超级块中的组长号
    superBlk.blockStack[0] = restNum;//初始剩余块
    superBlk.blockStack[1] = initLeader;//指向组长块
    unsigned last = DSK_INODE_BLK + 2;
    for (unsigned i = restNum; i > 1; i--) {
        superBlk.blockStack[i] = last++;
    }
    return true;
}


bool Disk::initSuperINode(unsigned &initILeader){
    //超级块中I节点的空间管理初始化
    unsigned restNum = (FREE_INODE_NUM) % 15 + 1;//固定化计算公式
    initILeader = restNum - 1;
    superBlk.iNodeStack[0] = restNum;//初始剩余块
    superBlk.iNodeStack[1] = initILeader;//指向组长块
    unsigned last = 0;
    for (unsigned i = restNum; i > 1; i--) {
        superBlk.iNodeStack[i] = last++;
    }
    return false;
}


///分配磁盘块
bool Disk::allocBlock(unsigned& blockNo) {
    //分配磁盘块
    if (superBlk.blockStack[0] == 1) {
        if (superBlk.blockStack[1] == 0) {
            cerr << "磁盘块分配失败!" << endl;
            return false;
        }
        else {//分配组长块
            blockNo = superBlk.blockStack[1];
            readBlock(blockNo, superBlk.blockStack, sizeof(superBlk.blockStack));//从磁盘读取
        }
    }
    else {
        blockNo = superBlk.blockStack[superBlk.blockStack[0]--];
    }
    superBlk.freeBlockNum--;
    return true;
}


///回收磁盘块
bool Disk::freeBlock(unsigned blockNo) {
    superBlk.freeBlockNum++;
    if (superBlk.blockStack[0] == SPR_MAX_BLK) {  //回收溢出，换出超级块到新的组长块
        writeBlock(blockNo, superBlk.blockStack, sizeof(superBlk.blockStack));
        superBlk.blockStack[0] = 1;
        superBlk.blockStack[1] = blockNo;
    }
    else {		//正常回收磁盘块
        superBlk.blockStack[0]++;
        superBlk.blockStack[superBlk.blockStack[0]] = blockNo;
    }
    return true;
}


bool Disk::isExist(const char name[28]){
    fstream fs;
    fs.open(name, ios::in);
    if (!fs) {
        return false;
    }
    fs.close();
    return true;
}


///分配磁盘i节点
bool Disk::allocINode(unsigned& iNoderNo){
    //分配磁盘块
    if (superBlk.iNodeStack[0] == 1) {
        if (superBlk.iNodeStack[1] == 0) {
            cerr << "I节点块分配失败!" << endl;
            return false;
        }
        else {//分配组长块
            iNoderNo = superBlk.iNodeStack[1];
            readINode(iNoderNo, superBlk.iNodeStack, sizeof(superBlk.iNodeStack));//从磁盘读取
        }
    }
    else {
        iNoderNo = superBlk.iNodeStack[superBlk.iNodeStack[0]--];
    }
    superBlk.freeINodeNum--;
    return true;
}


///回收磁盘i节点
bool Disk::freeINode(unsigned iNoderNo){
    //回收一块I节点块
    superBlk.freeINodeNum++;
    if (superBlk.iNodeStack[0] == SPR_MAX_INODE) {  //回收溢出，换出超级块到新的组长块
        writeINode(iNoderNo, superBlk.iNodeStack, sizeof(superBlk.iNodeStack));
        superBlk.iNodeStack[0] = 1;
        superBlk.iNodeStack[1] = iNoderNo;
    }
    else {		//正常回收磁盘块
        superBlk.iNodeStack[0]++;
        superBlk.iNodeStack[superBlk.iNodeStack[0]] = iNoderNo;
    }
    return true;
}


bool Disk::format(){//[删除原文件]创建成组链接空闲块、空闲I节点、初始化根目录
    FILE *fp = fopen("system", "wb");
    if (fp == NULL)
    {
        printf("can't open the file 'system'\n");
        return false;
    }
    char buffer[BLOCK_SIZ] = "";
    for (unsigned i = 0; i < DSK_MAX_NUM; i++) {//建立磁盘区
        fwrite(buffer, BLOCK_SIZ, 1, fp);
    }
    //写管理员信息
    User_D ud;
    fseek(fp, 0, SEEK_SET);
    ud.setInfo(0, "admin", "123");
    fwrite(&ud, sizeof(User_D), 1, fp);
    fseek(fp, 1 * BLOCK_SIZ, SEEK_SET);
    //初始化超级块
    unsigned initINodeNo;//首个I节点节点号
    unsigned initBlkNo;//首个磁盘块leader的块号

    initSuperINode(initINodeNo);
    initSuperBlk(initBlkNo);

    fwrite(&superBlk, sizeof(superBlk), 1, fp);//写超级块  fread(&superBlk, sizeof(superBlk),1,fp);
    unsigned curiNode = initINodeNo;
    for (; curiNode < (FREE_INODE_NUM); curiNode += SPR_MAX_INODE) {//I节点空闲块初始化
        unsigned iNode_leader[SPR_MAX_INODE + 1];
        iNode_leader[0] = SPR_MAX_INODE;
        for (unsigned i = SPR_MAX_INODE, k = 1; i > 0; i--, k++) {
            iNode_leader[i] = curiNode + k;
        }
        if (!m_fseek(fp, 2 * BLOCK_SIZ + DSK_INODE_SIZ * curiNode, SEEK_SET)) {//磁盘寻址
            return false;
        }
        fwrite(iNode_leader, sizeof(iNode_leader), 1, fp);//写入
    }

    unsigned curBlock = initBlkNo;//初始化当前块号
    for (; curBlock < DSK_MAX_NUM; curBlock += SPR_MAX_BLK) {//
        //对每个组长块初始化
        unsigned blk_leader[SPR_MAX_BLK + 1];//先建立组长块的缓冲区
        blk_leader[0] = SPR_MAX_BLK;
        for (unsigned i = SPR_MAX_BLK, k = 1; i > 0; i--, k++) {
            blk_leader[i] = curBlock + k;
        }
        if (!m_fseek(fp, BLOCK_SIZ*curBlock)) {//磁盘寻址
            return false;
        }
        fwrite(blk_leader, sizeof(blk_leader), 1, fp);//写入
    }

    //初始化根目录
    rootINode = DINode(1, -1, 0);
    unsigned iNodeNo, blockNo;
    if (allocINode(iNodeNo) && allocBlock(blockNo)) {
        rootINode.iNodeNo = iNodeNo;//(文件夹类型,父节点号，I节点号)该节点为根I节点
        FileDir filedir[2];
        rootINode.addr[0] = blockNo;
        strcpy(filedir[0].fileName, ".");
        strcpy(filedir[1].fileName, "..");
        filedir[0].iNodeNo = filedir[1].iNodeNo = iNodeNo;//根目录下两个文件都指向当前I节点
        //写I节点
        if (!m_fseek(fp, INODE_START + iNodeNo * DSK_INODE_SIZ))
            return false;
        fwrite(&rootINode, sizeof(DINode), 1, fp);
        //写目录文件
        if (!m_fseek(fp, blockNo*BLOCK_SIZ))
            return false;
        fwrite(filedir, sizeof(filedir), 1, fp);
    }
    fclose(fp);
    return true;
}


bool Disk::saveSuper(){
    return writeBlock(1, &superBlk, sizeof(superBlk));
}


bool Disk::loadDisk() {
    readINode(0, &rootINode, sizeof(rootINode));
    return readBlock(1, &superBlk, sizeof(superBlk));
}