//
// Created by SheepCore on 2019-06-25.
//

#include "FSystem.h"
System::System()
{
    memset(hash, NULL, sizeof(hash));
}

System::~System()
{
    saveMINode();
}
MINode * System::ihash(unsigned no)
{
    return hash[no%128];
}
MINode * System::miGet(unsigned iNo)
{
    MINode *p= ihash(iNo);
    while (p != NULL) {
        if (p->mi_dinode.iNodeNo == iNo) {
            break;
        }
        p = p->back;
    }
    return p;
}
void System::miPut(unsigned iNo, MINode * mi)
{
    //将该I节点出入hash链表队首
    mi->back = hash[iNo % 128];
    hash[iNo % 128] = mi;
}
void System::miDel(unsigned iNo, MINode *mi)
{
    if (mi == NULL)return;
    if (hash[iNo%128] == mi) {//队首
        hash[iNo%128] = mi->back;
        delete mi;
        return;
    }
    MINode *p = hash[iNo%128];
    while (p->back != mi)
        p = p->back;
    p->back = mi->back;
    delete mi;
}
bool System::saveMINode()
{
    for (unsigned i = 0; i < 128; i++) {
        MINode *p,*head = hash[i];
        while (head!=NULL && head->state==1)
        {
            writeINode(head->mi_dinode.iNodeNo, &head->mi_dinode, DSK_INODE_SIZ);
            p = head;
            head = p->back;
            delete p;
        }
    }
    return true;
}
void System::freeOldBlock(const DINode & di)
{
    ///回收原来数据所占盘块
    for (unsigned i = 0; i < di.blockNum && i < 4; i++) {//直接
        user.dir.disk.freeBlock(di.addr[i]);
    }
    if (di.blockNum > 4) {//一次
        unsigned d1Buf[256];
        readBlock(di.addr[4], d1Buf, sizeof(d1Buf));
        for (unsigned i = 0; i < (di.blockNum - 4) && i < 256; i++) {
            user.dir.disk.freeBlock(d1Buf[i]);
        }
        user.dir.disk.freeBlock(di.addr[4]);//释放一次地址块
    }
    if (di.blockNum > 4 + 256) {//二次
        unsigned d1Num = (di.blockNum - 4 - 256) / 256 + ((di.blockNum - 4 - 256)%256)!=0;//用于一次间接寻址的块数
        unsigned d2Buf[256];
        readBlock(di.addr[5], d2Buf, sizeof(d2Buf));
        for (unsigned i = 0; i < d1Num && d1Num<256; i++) {
            unsigned d1Buf[256];
            readBlock(d2Buf[i], d1Buf, sizeof(d1Buf));
            for (unsigned j = 0; j < 256 && (i * 256 + j) < (di.blockNum-256-4); j++) {
                user.dir.disk.freeBlock(d1Buf[j]);
            }
            user.dir.disk.freeBlock(d2Buf[i]);//释放一次地址块
        }
        user.dir.disk.freeBlock(di.addr[5]);//释放二次地址块
    }
}
bool System::closeFile(unsigned iNo)
{
    MINode *mi = miGet(iNo);
    if (mi == NULL) {
        printf("close error， can't find iNo!\n");
        return false;
    }
    mi->mi_count--;
    if (mi->mi_count == 0) {//释放当前节点，并根据情况保存信息
        if (mi->state == 1){//I节点及磁盘内容写回
            writeINode(mi->mi_dinode.iNodeNo, &mi->mi_dinode, DSK_INODE_SIZ);
        }
        miDel(iNo,mi);
    }
    return true;
}
bool System::openFile(const char fileName[28], unsigned & iNo)
{
    if (!user.dir.findDir(fileName, iNo)) {
        printf("can't open, %s not exist!\n",fileName);
        return false;
    }
    //打开成功，读取I节点信息到内存I节点
    MINode *mi = miGet(iNo);//获取内存I节点
    if (mi == NULL) {//不再内存中
        mi = new MINode;//记得释放内存
        mi->mi_count = 1;
        mi->state = 0;//初始状态值为0,表示文件和I节点未发生变化
        readINode(iNo, &mi->mi_dinode, sizeof(mi->mi_dinode));
        if (mi->mi_dinode.fileType==1) {//权限检查|| mi->mi_dinode.priorMode==//公|私、读|写
            printf("you can't open it!\n");
            return false;
        }
        miPut(iNo, mi);
    }
    else {//已在内存
        if (mi->mi_dinode.fileType==1) {//权限检查
            printf("you can't open it!\n");
            return false;
        }
        mi->mi_count++;//链接计数加一
    }
    return true;
}
bool System::readFile(unsigned iNo, string & data)
{
    MINode *mi = miGet(iNo);
    if (mi == NULL) {
        return false;
    }
    //if (mi->mi_dinode.blockNum == 0) {
    //	printf("data empty\n");
    //	return false;
    //}
    unsigned useBlkNum = mi->mi_dinode.blockNum;//已使用磁盘数
    char dataBuf[1024+1];//数据缓冲
    dataBuf[1024] = '\0';
    for (unsigned i = 0; i < useBlkNum && i <4; i++) {//直接
        readBlock(mi->mi_dinode.addr[i], dataBuf, BLOCK_SIZ);
        data.append(dataBuf);
    }
    if (useBlkNum > 4) {//一次
        unsigned d1Buf[256];
        readBlock(mi->mi_dinode.addr[4], d1Buf, BLOCK_SIZ);
        for (unsigned i = 0; i < useBlkNum - 4 && i<256; i++) {
            readBlock(d1Buf[i], dataBuf, BLOCK_SIZ);
            data.append(dataBuf);
        }
    }
    if (useBlkNum > (256 + 4)) {//二次
        unsigned d2Buf[256], d1Buf[256];
        unsigned left = useBlkNum - 4 - 256;
        unsigned d1Num = left / 256 + (left % 256) != 0;//二次地址表的有效项数（及一地址所占块数）
        readBlock(mi->mi_dinode.addr[5], d2Buf, BLOCK_SIZ);
        for (unsigned i = 0; i < d1Num && d1Num<256; i++) {
            readBlock(d2Buf[i], d1Buf, BLOCK_SIZ);
            for (unsigned j = 0; j < 256 && (i * 256 + j) < left; j++) {
                readBlock(d1Buf[j], dataBuf, BLOCK_SIZ);
                data.append(dataBuf);
            }
        }
    }
    return true;
}
bool System::writeData(unsigned iNo, const string & data)
{
    MINode *mi = miGet(iNo);
    if (mi == NULL)
        return false;//获取节点失败
    //需要先删除文件数据，然后重新写入
    mi->state = 1;//数据发生改变
    ///回收原来数据所占盘块,但不回收I节点
    freeOldBlock(mi->mi_dinode);

    unsigned ds = data.size();//数据大小
    unsigned useBlkNum = ds/ 1024+(ds%1024!=0);//需使用磁盘数
    unsigned left = useBlkNum - 256 - 4;
    mi->mi_dinode.blockNum = useBlkNum;
    mi->mi_dinode.lastBlockUsedByte = ds%1024;
    char dataBuf[1024+1];//数据缓冲
    unsigned ii = 0;
    for (unsigned i = 0; i < useBlkNum && i < 4; i++) {//直接
        if (ii < useBlkNum) {
            strcpy(dataBuf, data.substr(1024 * ii, 1024).c_str());
            ii++;
        }
        else return false;
        user.dir.disk.allocBlock(mi->mi_dinode.addr[i]);//分配
        writeBlock(mi->mi_dinode.addr[i], dataBuf, BLOCK_SIZ);//写第i块的数据
    }
    if(useBlkNum>4){//一次
        user.dir.disk.allocBlock(mi->mi_dinode.addr[4]);//分配一次间址块
        unsigned d1Buf[256];
        for (unsigned i = 0; i < useBlkNum - 4 && i<256; i++) {
            user.dir.disk.allocBlock(d1Buf[i]);//分配
            if (ii < useBlkNum) {
                strcpy(dataBuf, data.substr(1024 * ii, 1024).c_str());
                ii++;
            }
            else return false;
            writeBlock(d1Buf[i], dataBuf, BLOCK_SIZ);//写第i块的数据
        }
        writeBlock(mi->mi_dinode.addr[4], d1Buf, sizeof(d1Buf));//写一次间接寻址块
    }
    if (useBlkNum > (256 + 4)) {//二次
        unsigned d2Buf[256], d1Buf[256];
        user.dir.disk.allocBlock(mi->mi_dinode.addr[5]);//二次地址块地址
        unsigned d1Num = (useBlkNum - 4 - 256) / 256 + ((useBlkNum - 4 - 256) % 256) != 0;//二次地址的有效项数（及一地址所占块数）
        for (unsigned i = 0; i < d1Num && d1Num<256; i++) {
            user.dir.disk.allocBlock(d2Buf[i]);
            for (unsigned j = 0; j < 256 && (i * 256 + j) < left; j++) {
                user.dir.disk.allocBlock(d1Buf[j]);//分配
                if (ii < useBlkNum) {
                    strcpy(dataBuf, data.substr(1024 * ii, 1024).c_str());
                    ii++;
                }
                else return false;
                writeBlock(d1Buf[j], dataBuf, BLOCK_SIZ);//写第i块的数据
            }
            writeBlock(d2Buf[i], d1Buf, sizeof(d1Buf));
        }
        writeBlock(mi->mi_dinode.addr[5], d2Buf, sizeof(d2Buf));
    }
    return true;
}



bool System::changeUser(User &otherUser) {
    return true;
}

void System::traverseUsers(){
    unsigned CurUserNum = user.dir.disk.getUserNum();
    User_D UserBuffer[USER_NUM];
    //定位到普通用户区
    read(0, &UserBuffer, sizeof(UserBuffer));
    printf("Total User: %d\n", CurUserNum);
    for (int i = 0; i < CurUserNum; ++i) {
        cout << UserBuffer[i].userID << "\t\t";
        cout << UserBuffer[i].userName << "\t\t";
        cout << UserBuffer[i].password << endl;
    }
    cout << endl;
}

bool System::createUser(User_D &newUser) {
}
