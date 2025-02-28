#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "TString.h"
#include "BinaryHeap.h"
#include "TMap.h"
#include "Timer.h"
#include "Converter.h"

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


    
class MyNode : public Node2 {
public:
    MyNode(int id) :mID(id) {}
    int mID;
    static bool lessThen(const Node2* a, const Node2* b) {
        return ((MyNode*)a)->mID < ((MyNode*)b)->mID;
    }
};

class Queue {
public:
    Queue(int mx) {
        sort();
        init(mx);
    }


    ~Queue() {}


    void sort() {
        if (0) {
            Node2* que = Node2::sort(mHead.mNext, &mHead, MyNode::lessThen);
            mHead.clear();
            mHead += (*que);
        } else {
            Node2* que = mHead.mNext;
            mHead.delink();
            que = que->sort(MyNode::lessThen);
            mHead += (*que);
        }
    }

    void show() {
        MyNode* curr = (MyNode*)(mHead.mNext);
        int idx = 0;
        while (curr != &mHead) {
            printf("%p [%3d] = %32d\n", curr, idx++, curr->mID);
            curr = (MyNode*)(curr->getNext());
        }
    }

private:

    void init(int mx) {
        bool order = false;
        if (mx < 0) {
            order = true;
            mx = -mx;
        }
        int val;
        srand(Timer::getRelativeTime());
        MyNode* curr;
        for (int i = 0; i < mx; ++i) {
            val = order ? i : rand();
            curr = new MyNode(val);
            mHead.pushBack(*curr);
            //printf("%d = %d\n", i, val);
        }
    }

    Node2 mHead;
};

s32 AppTestNode(s32 argc, s8** argv){
    printf("argc= %d, %s\n", argc, argv[0]);
    const s32 count = App10StrToS32(argv[2]);
    app::Queue que(count);
    printf("--unsort--------------------------------------------------------------\n");
    que.show();
    printf("--sort--------------------------------------------------------------\n");
    que.sort();
    que.show();
    printf("----------------------------------------------------------------\n");
    return 0;
}

} //namespace app
