#pragma once

#define JUICE_STATIC
#include "juice/juice.h"

#include "Config.h"

//#ifdef __cplusplus
//extern "C" {
//#endif
////struct juice_agent_t;
////enum juice_state_t;
//#ifdef __cplusplus
//}
//#endif


namespace app {
namespace net {

class NatHolePuncher {
public:
    NatHolePuncher();
    ~NatHolePuncher();

    s32 init();

protected:
    void onStateChanged1(juice_agent_t* agent, juice_state_t state);
    void onStateChanged2(juice_agent_t* agent, juice_state_t state);

    void onCandidate1(juice_agent_t* agent, const char* sdp);
    void onCandidate2(juice_agent_t* agent, const char* sdp);

    void onGatheringDone1(juice_agent_t* agent);
    void onGatheringDone2(juice_agent_t* agent);

    void onRecv1(juice_agent_t* agent, const char* data, size_t size);
    void onRecv2(juice_agent_t* agent, const char* data, size_t size);


private:
    static void on_state_changed1(juice_agent_t* agent, juice_state_t state, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onStateChanged1(agent, state);
    }
    static void on_state_changed2(juice_agent_t* agent, juice_state_t state, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onStateChanged2(agent, state);
    }

    static void on_candidate1(juice_agent_t* agent, const char* sdp, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onCandidate1(agent, sdp);
    }
    static void on_candidate2(juice_agent_t* agent, const char* sdp, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onCandidate2(agent, sdp);
    }

    static void on_gathering_done1(juice_agent_t* agent, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onGatheringDone1(agent);
    }
    static void on_gathering_done2(juice_agent_t* agent, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onGatheringDone2(agent);
    }

    static void on_recv1(juice_agent_t* agent, const char* data, size_t size, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onRecv1(agent, data, size);
    }
    static void on_recv2(juice_agent_t* agent, const char* data, size_t size, void* user_ptr) {
        NatHolePuncher* nd = reinterpret_cast<NatHolePuncher*>(user_ptr);
        nd->onRecv2(agent, data, size);
    }

    juice_agent_t* mNatAgentA = nullptr;
    juice_agent_t* mNatAgentB = nullptr;
};

} // namespace net
} // namespace app
