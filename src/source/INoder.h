//
// Created by SheepCore on 2019-06-25.
//

#ifndef FILESYSTEM_INODER_H
#define FILESYSTEM_INODER_H
#include "define.h"
class INoder
{
public:
    INoder();
    ~INoder();
    bool addINode(MINode &miNode);
    bool getCurDir(MINode &miNode);
    bool delINode(unsigned iNodeNo);
private:
    unsigned size;
    vector<MINode> miNodes;
};

INoder::INoder()
{
    size = miNodes.size();
}

INoder::~INoder()
{
}


#endif //FILESYSTEM_INODER_H
