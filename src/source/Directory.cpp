//
// Created by SheepCore on 2019-06-25.
//

#include "Directory.h"
Directory::Directory() :
        disk(), parentINode(disk.rootINode)
{
    setUserID(0);//默认为管理员ID
    nameStk.push_back(disk.rootName);
    m_dirBuf.reset(parentINode);
}

Directory::~Directory()
{
    //最后写回去
    saveParABuf();
}

bool Directory::mkdir_touch(const char dirName[28], bool type)
{
    if (findDir(dirName))
        return false;
    if (m_dirBuf.used == 0)
        m_dirBuf.reset(parentINode);
    //根据参数，申请I节点，并在磁盘块中添加目录项；在创建过程中，每个文件夹要预约一个磁盘块
    //先要根据父节点信息知道该向那个磁盘号追加目录项
    unsigned curBlockNo;//当前磁盘号
    unsigned haveUsed = parentINode.lastBlockUsedByte;//在当前块号中的空间使用情况,初始为32*2
    if (parentINode.blockNum <= 4) {//[0-3]直接寻址
        curBlockNo = parentINode.getCur();//指向父目录正在使用的块号
        if (haveUsed == BLOCK_SIZ) {//已使用满，写回磁盘 || blockBuf.used==DIR_ITEMNUM
            writeBlock(curBlockNo, m_dirBuf.filedir, sizeof(m_dirBuf.filedir));//将已满的磁盘号，写回磁盘
            //更新父I节点信息
            parentINode.blockNum++;
            parentINode.lastBlockUsedByte = DIR_ITEMNUM;//新增的占一项
            m_dirBuf.used = 0;//重新初始化Blockbuf
            strcpy(m_dirBuf.filedir[m_dirBuf.used].fileName, dirName);
            m_dirBuf.used++;
            if (disk.allocBlock(curBlockNo)) {//为父目录申请空闲块, 涉及混合索引
                if (parentINode.blockNum < oneNum) {//blockNum<5
                    parentINode.addr[parentINode.blockNum - 1] = curBlockNo;//先指向新的块
                }
                else//blockNum=5,一次间接寻址，指向新的块
                {
                    printf("一次间接寻址...\n");
                }
            }
        }
        else {//未满(满和没满)
            strcpy(m_dirBuf.filedir[m_dirBuf.used].fileName, dirName);
            parentINode.lastBlockUsedByte += DIR_ITEMSIZ;
            m_dirBuf.used++;//目录磁盘块缓冲项++
        }
    }
    else if (parentINode.blockNum <= oneNum) {//目录项所占块数需一次间接寻址（目录项>128-2）

    }
    else//二次间接寻址
    {
        //初步考虑，简单实现，不管效率，能用就行
    }
    //**//为新建目录申请I节点和磁盘块
    if (type == 1)
        mkdir();
    else {//type==0
        touch();
    }
    return true;
}

bool Directory::chmod(const char fileName[28], ModeType mod)//修改权限后立即写回磁盘
{
    unsigned iNo;
    if (!findDir(fileName, iNo)) {
        printf("can't find %s\n", fileName);
        return false;
    }
    DINode diNode;
    readINode(iNo, &diNode, sizeof(diNode));
    if (userID != diNode.userID) {
        printf("you can't change this file's mode!\n");
        return false;
    }
    diNode.priorMode = mod;
    writeINode(iNo, &diNode, sizeof(diNode));
    return true;
}


///删除文件/文件夹-->（1）删除在父级目录下的目录项<和最后一项调换>，（2）删除该目录项下的所有内容[即释放block与I节点]
bool Directory::rmdir_file(const char fileName[28])
{
    unsigned iNo;//当前磁盘号
    if (!delItem(fileName, iNo)) {//删除在父级目录下的目录项
        printf("待删除文件不存在\n");
        return false;
    }
    DINode rmINode;
    readINode(iNo, &rmINode, sizeof(rmINode));//读待删除文件的I节点
    if (userID != rmINode.userID) {
        printf("wrong user ID\n");
        return false;
    }
    recurDel(rmINode);//递归删除
    return true;
}

bool Directory::cd(const char fileName[28])
{
    unsigned iNo;
    if (!findDir(fileName, iNo)) {
        printf("can't find %s\n", fileName);
        return false;
    }
    DINode di;
    if (readINode(iNo, &di, sizeof(di)) && di.fileType == 0) {
        printf("you can't cd a file!\n");
        return false;
    }
    if (userID != di.userID) {
        printf("this file doesn't belong to you!\n");
        return false;
    }
    if (saveParABuf()) {//保存当前信息到磁盘
        parentINode = di;//转到对应目录
        m_dirBuf.reset(parentINode);
        if (strcmp(fileName, "..") != 0 && strcmp(fileName, ".") != 0) {//走向下一级目录
            nameStk.push_back(fileName);
        }
        else if (strcmp(fileName, "..") == 0 && nameStk.size() > 1) {//走向上一级，且当前为非根目录
            nameStk.pop_back();
        };
        readBlock(parentINode.getCur(), m_dirBuf.filedir, sizeof(m_dirBuf.filedir));
        m_dirBuf.used = (parentINode.lastBlockUsedByte / 32);
        return true;
    }
    return false;
}


bool Directory::findDir(const char name[28])//规定一个目录项占用32字节，块大小1024B
{
    FileDir dirBuf[32];
    unsigned lastItem = parentINode.lastBlockUsedByte / 32;
    const unsigned size = (parentINode.blockNum - 1) * 32 + lastItem;//总的目录项数目
    for (unsigned i = 0; i < parentINode.blockNum - 1 && i < 4; i++) {//假设目录项的所占块最大为4
        readBlock(parentINode.addr[i], dirBuf, BLOCK_SIZ);
        for (int j = 0; j < 32 && (i * 32 + j) < size; j++) {
            if (strcmp(dirBuf[j].fileName, name) == 0)//找到
                return true;
        }
    }
    if (this->m_dirBuf.used == 0) {//缓冲区中无内容
        readBlock(parentINode.addr[parentINode.blockNum - 1], dirBuf, BLOCK_SIZ);
        for (unsigned j = 0; j < lastItem; j++) {
            if (strcmp(dirBuf[j].fileName, name) == 0)//找到
                return true;
        }
    }
    else//缓冲区中有内容
    {
        //if (parentINode.blockNum == 1) {
        //	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        //		return true;
        //}
        for (int i = 0; i < this->m_dirBuf.used; i++)
            if (strcmp(this->m_dirBuf.filedir[i].fileName, name) == 0)//找到
                return true;
    }
    return false;
}

bool Directory::findDir(const char name[28], unsigned & iNodeNo)
{
    FileDir dirBuf[32];
    unsigned lastItem = parentINode.lastBlockUsedByte / 32;
    const unsigned size = (parentINode.blockNum - 1) * 32 + lastItem;//总的目录项数目
    for (unsigned i = 0; i < parentINode.blockNum - 1 && i < 4; i++) {//假设目录项的所占块最大为4
        readBlock(parentINode.addr[i], dirBuf, BLOCK_SIZ);
        for (int j = 0; j < 32 && (i * 32 + j) < size; j++) {
            if (strcmp(dirBuf[j].fileName, name) == 0) {//找到
                iNodeNo = dirBuf[j].iNodeNo;
                return true;
            }
        }
    }
    if (this->m_dirBuf.used == 0) {//缓冲区中无内容
        readBlock(parentINode.addr[parentINode.blockNum - 1], dirBuf, BLOCK_SIZ);
        for (unsigned j = 0; j < lastItem; j++) {
            if (strcmp(dirBuf[j].fileName, name) == 0) {//找到
                iNodeNo = dirBuf[j].iNodeNo;
                return true;
            }
        }
    }
    else//缓冲区中有内容
    {
        //if (parentINode.blockNum == 1) {
        //	if (strcmp(name, ".") == 0) {
        //		iNodeNo = parentINode.iNodeNo;
        //		return true;
        //	}
        //	if (strcmp(name, "..") == 0) {
        //		iNodeNo = parentINode.parentINo;
        //		return true;
        //	}
        //}
        for (int i = 0; i < this->m_dirBuf.used; i++)
            if (strcmp(this->m_dirBuf.filedir[i].fileName, name) == 0) {//找到
                iNodeNo = this->m_dirBuf.filedir[i].iNodeNo;
                return true;
            }
    }
    return false;
}

void Directory::showInfo(const char name[28])
{
    unsigned iNo;
    if (!findDir(name, iNo)) {
        printf("can't find %s\n", name);
        return;
    }
    DINode di;//创建空白节点
    readINode(iNo, &di, sizeof(di));
    string type = (di.fileType == 1) ? "folder" : "file";
    string prior = (di.priorMode < 4) ? "pub" : "prv";

    printf("|文件名字:%s|文件类型:%s|用户权限:%s|访问权限:%s|占磁盘数:%d|所属用户:%d|链接计数:%d|\n",
           name,//文件名字
           type.c_str(),//文件类型
           prior.c_str(),//用户权限
           modetype[di.priorMode % 4].c_str(),//访问权限
           di.blockNum,//占磁盘数
           di.userID,//所属用户
           di.linkCount);//链接计数
}

void Directory::showDir()
{
    string path(nameStk[0]);
    for (unsigned i = 1; i < nameStk.size(); i++) {
        path.append("/" + nameStk[i]);
    }
    printf("--------------------\n");
    FileDir dirBuf[32];
    const int lastItem = parentINode.lastBlockUsedByte / 32;
    const unsigned size = (parentINode.blockNum - 1) * 32 + lastItem;//总的目录项数目

    for (unsigned i = 0; i < parentINode.blockNum - 1 && i < 4; i++) {//假设目录项的所占块最大为4,取出已满的块
        readBlock(parentINode.addr[i], dirBuf, BLOCK_SIZ);
        for (int j = 0; j < 32 && (i * 32 + j) < size; j++) {
            printf("[%s]>%s\n", path.c_str(), dirBuf[j].fileName);
        }
    }
    if (this->m_dirBuf.used == 0) {//缓冲区中无内容
        readBlock(parentINode.addr[parentINode.blockNum - 1], dirBuf, BLOCK_SIZ);
        for (int j = 0; j < lastItem; j++) {
            printf("[%s]>%s\n", path.c_str(), dirBuf[j].fileName);
        }
    }
    else//缓冲区中有内容
    {
        for (int i = 0; i < this->m_dirBuf.used; i++)
            printf("[%s]>%s\n", path.c_str(), this->m_dirBuf.filedir[i].fileName);
    }
    printf("--------------------\n");
}

bool Directory::saveParABuf()
{
    writeINode(parentINode.iNodeNo, &parentINode, sizeof(parentINode));//保存I节点信息
    if (m_dirBuf.used != 0) {//写当前目录下的目录缓冲
        writeBlock(parentINode.getCur(), m_dirBuf.filedir, sizeof(m_dirBuf.filedir));
        m_dirBuf.used = 0;
    }
    return true;
}

bool Directory::format()
{
    bool sure = false;
    printf("are you sure to format your disk?[1/0]\n");
    cin >> sure;
    if (sure) {
        disk.format();
        parentINode = disk.rootINode;//初始化根节点
        nameStk.clear();
        nameStk.push_back(disk.rootName);//路径名
        m_dirBuf.reset(parentINode);//重置缓冲区
    }
    return sure;
}

bool Directory::getDirNo(unsigned & dirNo)
{
    dirNo = parentINode.getCur();
    return true;
}
bool Directory::mkdir()
{
    unsigned nwBlockNo, nwiNodeNo;
    if (disk.allocBlock(nwBlockNo) && disk.allocINode(nwiNodeNo)) {//分配一个节点和盘块
        m_dirBuf.filedir[m_dirBuf.used - 1].iNodeNo = nwiNodeNo;
        DINode dinode(1, parentINode.iNodeNo, nwiNodeNo, userID);
        dinode.addr[0] = nwBlockNo;
        FileDir filedir[2];
        strcpy(filedir[0].fileName, ".");
        strcpy(filedir[1].fileName, "..");
        filedir[0].iNodeNo = nwiNodeNo;
        filedir[1].iNodeNo = parentINode.iNodeNo;
        writeBlock(nwBlockNo, filedir, sizeof(filedir));//写入磁盘
        writeINode(nwiNodeNo, &dinode, sizeof(dinode));
    }
    return false;
}
bool Directory::touch()
{
    unsigned nwiNodeNo;
    if (disk.allocINode(nwiNodeNo)) {//分配一个节点和盘块
        m_dirBuf.filedir[m_dirBuf.used - 1].iNodeNo = nwiNodeNo;
        DINode dinode(0, parentINode.iNodeNo, nwiNodeNo, userID);
        dinode.blockNum = 0;
        dinode.lastBlockUsedByte = 0;
        writeINode(nwiNodeNo, &dinode, sizeof(dinode));//I节点写入磁盘
    }
    return false;
}
bool Directory::delItem(const char name[28], unsigned & iNodeNo)
{
    //为操作方便先将缓冲区的内容存回磁盘
    saveParABuf();
    unsigned lastItemNum = parentINode.lastBlockUsedByte / 32;
    const unsigned size = (parentINode.blockNum - 1) * 32 + lastItemNum;//总的目录项数目
    FileDir dirBuf[32];
    FileDir theLast;//最后一个目录项
    readBlock(parentINode.getCur(), dirBuf, sizeof(dirBuf));
    theLast = dirBuf[lastItemNum - 1];
    /*if (lastItemNum==1) {//恰好最后一项,回收（因为最后一项用来替换）
        disk.freeINode(parentINode.getCur());
        parentINode.blockNum--;
        parentINode.lastBlockUsedByte = DIR_ITEMSIZ;
    }*/
    if (strcpy(theLast.fileName, name)) {//恰好是最后一项
        iNodeNo = theLast.iNodeNo;
        if (lastItemNum == 1) {//恰好最后一项,回收（因为最后一项用来替换）
            disk.freeINode(parentINode.getCur());
            parentINode.blockNum--;
            parentINode.lastBlockUsedByte = BLOCK_SIZ;
        }
        else
        {
            parentINode.lastBlockUsedByte -= DIR_ITEMSIZ;
        }
        return true;
    }
    for (unsigned i = 0; i < parentINode.blockNum && i < 4; i++) {//假设目录项的所占块最大为4
        readBlock(parentINode.addr[i], dirBuf, BLOCK_SIZ);
        for (int j = 0; j < 32 && (i * 32 + j) < size; j++) {
            if (strcmp(dirBuf[j].fileName, name) == 0) {//找到
                iNodeNo = dirBuf[j].iNodeNo;
                dirBuf[j] = theLast;//替换
                ///处理dirBuf[j],和最后盘块或内存中的内容交换
                writeBlock(parentINode.addr[i], dirBuf, sizeof(dirBuf));
                if (lastItemNum == 1) {//恰好最后一项,回收（因为最后一项用来替换）
                    disk.freeINode(parentINode.getCur());
                    parentINode.blockNum--;
                    parentINode.lastBlockUsedByte = BLOCK_SIZ;
                }
                else
                {
                    parentINode.lastBlockUsedByte -= DIR_ITEMSIZ;
                }
                return true;
            }
        }
    }
    return false;
}
bool Directory::recurDel(DINode parINode)//暂时处理直接寻址的
{
    if (parINode.blockNum <= 4) {//[0-3]直接寻址
        disk.freeINode(parINode.iNodeNo);
        for (unsigned i = 0; i < parINode.blockNum; i++) {
            disk.freeBlock(parINode.addr[i]);
        }
        if (parINode.fileType == 1) {//递归删除文件夹,先遍历里面所有文件
            FileDir dirBuf[32];
            unsigned lastItem = parINode.lastBlockUsedByte / 32;
            const unsigned size = (parINode.blockNum - 1) * 32 + lastItem;//总的目录项数目
            for (unsigned i = 0; i < parINode.blockNum && i < 4; i++) {//假设目录项的所占块最大为4
                readBlock(parINode.addr[i], dirBuf, BLOCK_SIZ);
                for (int j = 0; j < 32 && (i * 32 + j) < size; j++) {
                    if (strcmp(dirBuf[j].fileName, ".") != 0 && strcmp(dirBuf[j].fileName, "..") != 0) {//. 与 ..不占用节点和块
                        DINode di;
                        readINode(dirBuf[j].iNodeNo, &di, sizeof(di));
                        recurDel(di);
                    }
                }
            }
        }
    }
    else if (parINode.blockNum <= oneNum)//一次间接寻址
    {

    }
    return false;
}