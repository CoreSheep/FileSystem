//
// Created by SheepCore on 2019-06-25.
//
#include "define.h"

BlockState ui_blockstate[DSK_MAX_NUM];//磁盘块状态数组，便于可视化
///下面是磁盘操作的快捷方法
bool m_fseek(FILE *m_stream, long offset, int origin) {
    if (fseek(m_stream, offset, origin) != 0) {//磁盘寻址
        fclose(m_stream);
        return false;
    }
    return true;
}

bool write(unsigned offset, const void * buffer, size_t size)
{
    FILE *fp = fopen("system", "rb+");
    if (fp == NULL) {
        printf("system open false\n");
        return false;
    }
    if (!m_fseek(fp, offset)) {
        printf("seek false\n");
        return false;
    }
    if (fwrite(buffer, size, 1, fp) != 1) {//将已满的磁盘号，写回磁盘
        printf("seek false\n");
        return false;
    }
    fclose(fp);
    return true;
}


bool read(unsigned offset, void * buffer, size_t size)
{
    FILE *fp = fopen("system", "rb+");
    if (fp == NULL) {
        printf("system open false\n");
        return false;
    }
    if (!m_fseek(fp, offset)) {
        printf("seek false\n");
        return false;
    }
    if (fread(buffer, size, 1, fp) != 1) {//将已满的磁盘号，写回磁盘
        printf("seek false\n");
        return false;
    }
    fclose(fp);
    return true;
}

bool writeBlock(unsigned blockNo, const void * buffer, size_t size){
    FILE *fp = fopen("system", "rb+");
    if (fp == NULL) {
        printf("system open false\n");
        return false;
    }
    if (!m_fseek(fp, BLOCK_SIZ*blockNo)) {
        printf("seek false\n");
        return false;
    }
    if (fwrite(buffer, size, 1, fp) != 1) {//将已满的磁盘号，写回磁盘
        printf("seek false\n");
        return false;
    }
    fclose(fp);
    return true;
}

bool readBlock(unsigned blockNo, void * buffer, size_t size){
    FILE *fp = fopen("system", "rb");
    if (fp == NULL) {
        printf("system open false\n");
        return false;
    }
    if (!m_fseek(fp, BLOCK_SIZ*blockNo)) {
        printf("seek false\n");
        return false;
    }
    if (fread(buffer, size, 1, fp) != 1) {//读取
        printf("read false\n");
        return false;
    }
    fclose(fp);
    return true;
}

bool writeINode(unsigned iNodeNo, const void * buffer, size_t size){
    FILE *fp = fopen("system", "rb+");
    if (fp == NULL) {
        printf("system open false\n");
        return false;
    }
    if (!m_fseek(fp, INODE_START + iNodeNo * DSK_INODE_SIZ)) {
        printf("seek false\n");
        return false;
    }
    if (fwrite(buffer, size, 1, fp) != 1) {//将已满的磁盘号，写回磁盘
        printf("seek false\n");
        return false;
    }
    fclose(fp);
    return true;
}

bool readINode(unsigned iNodeNo, void * buffer, size_t size) {
    FILE *fp = fopen("system", "rb");
    if (fp == NULL) {
        printf("system open false\n");
        return false;
    }
    if (!m_fseek(fp, INODE_START + iNodeNo * DSK_INODE_SIZ)) {
        printf("seek false\n");
        return false;
    }
    if (fread(buffer, size, 1, fp) != 1) {//读取
        fclose(fp);
        printf("read false\n");
        return false;
    }
    fclose(fp);
    return true;
}
