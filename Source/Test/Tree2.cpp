#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "Strings.h"
#include "BinaryHeap.h"
#include "TMap.h"

namespace app {

void AppTestTree2heap() {
    class CUserNode : public Node3 {
    public:
        CUserNode() :mVal(0) { }

        bool operator<(const CUserNode& val)const {
            return mVal < val.mVal;
        }

        static bool lessThan(const Node3* va, const Node3* vb) {
            const CUserNode& tva = *(const CUserNode*)va;
            const CUserNode& tvb = *(const CUserNode*)vb;
            return (tva < tvb);
        }

        int mVal;
    };

    BinaryHeap* hp = new BinaryHeap(CUserNode::lessThan);

    for(; hp->getSize() < 500; ) {
        CUserNode* val = new CUserNode();
        val->mVal = rand();
        hp->insert(val);
    }

    printf("heap size = %llu\n", hp->getSize());

    s32 idx = 0;
    while(hp->getTop()) {
        CUserNode* val = (CUserNode*)hp->removeTop();
        printf("del[%d/%lld] = %d\n", ++idx, hp->getSize(), val->mVal);
        delete val;
    }

    delete hp;
}



void AppTestRBTreeMap() {
    TMap<s32, String> map;
    String str;
    for(s32 i = 0; i < 1000; ++i) {
        str = i;
        map.insert(i, str);
    }
    printf("map size=%llu\n", map.size());

    usz lv, maxlev = 0;
    for(TMap<s32, String>::Iterator it = map.getIterator(); !it.atEnd(); ++it) {
        lv = it->getLevel();
        maxlev = maxlev < lv ? lv : maxlev;
        printf("%d=%s, level=%llu\n", (*it).getKey(), (*it).getValue().c_str(), lv);
    }

    TMap<s32, String>::Node* nd = map.find(55);
    if(nd) {
        printf("node[%d]=%s, level=%llu\n", nd->getKey(), nd->getValue().c_str(), nd->getLevel());
    }

    map[55] = "yes 55";
    str = map[55];
    printf("node[55]=%s\n", str.c_str());
    printf("max level=%llu\n", maxlev);

}

} //namespace app
