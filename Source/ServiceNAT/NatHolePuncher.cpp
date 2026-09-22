#include "NatHolePuncher.h"
#include "Logger.h"
#include <thread>

#define BUFFER_SIZE 4096


namespace app {
namespace net {

NatHolePuncher::NatHolePuncher() {
}

NatHolePuncher::~NatHolePuncher() {
}


void NatHolePuncher::onStateChanged1(juice_agent_t* agent, juice_state_t state) {
    DLOG(ELL_INFO, "State 1: %s\n", juice_state_to_string(state));
    if (state == JUICE_STATE_CONNECTED) {
        // Agent 1: on connected, send a message
        const char* message = "Hello from 1";
        juice_send(agent, message, strlen(message));
    }
}

void NatHolePuncher::onStateChanged2(juice_agent_t* agent, juice_state_t state) {
    DLOG(ELL_INFO, "State 2: %s\n", juice_state_to_string(state));
    if (state == JUICE_STATE_CONNECTED) {
        // Agent 2: on connected, send a message
        const char* message = "Hello from 2";
        juice_send(agent, message, strlen(message));
    }
}

// Agent 1: on local candidate gathered
void NatHolePuncher::onCandidate1(juice_agent_t* agent, const char* sdp) {
    DLOG(ELL_INFO, "Candidate 1: %s\n", sdp);

    // Agent 2: Receive it from agent 1
    juice_add_remote_candidate(mNatAgentB, sdp);
}

// Agent 2: on local candidate gathered
void NatHolePuncher::onCandidate2(juice_agent_t* agent, const char* sdp) {
    DLOG(ELL_INFO, "Candidate 2: %s\n", sdp);

    // Agent 1: Receive it from agent 2
    juice_add_remote_candidate(mNatAgentA, sdp);
}

// Agent 1: on local candidates gathering done
void NatHolePuncher::onGatheringDone1(juice_agent_t* agent) {
    DLOG(ELL_INFO, "Gathering done 1\n");
    juice_set_remote_gathering_done(mNatAgentB); // optional
}

// Agent 2: on local candidates gathering done
void NatHolePuncher::onGatheringDone2(juice_agent_t* agent) {
    DLOG(ELL_INFO, "Gathering done 2\n");
    juice_set_remote_gathering_done(mNatAgentA); // optional
}

// Agent 1: on message received
void NatHolePuncher::onRecv1(juice_agent_t* agent, const char* data, size_t size) {
    char buffer[BUFFER_SIZE];
    if (size > BUFFER_SIZE - 1)
        size = BUFFER_SIZE - 1;
    memcpy(buffer, data, size);
    buffer[size] = '\0';
    DLOG(ELL_INFO, "Received 1: %s\n", buffer);
}

// Agent 2: on message received
void NatHolePuncher::onRecv2(juice_agent_t* agent, const char* data, size_t size) {
    char buffer[BUFFER_SIZE];
    if (size > BUFFER_SIZE - 1)
        size = BUFFER_SIZE - 1;
    memcpy(buffer, data, size);
    buffer[size] = '\0';
    DLOG(ELL_INFO, "Received 2: %s\n", buffer);
}


s32 NatHolePuncher::init() {
    juice_set_log_level(JUICE_LOG_LEVEL_DEBUG);

    // Agent 1: Create agent
    juice_config_t config1;
    memset(&config1, 0, sizeof(config1));

    // STUN server example
    config1.stun_server_host = "witcore.cn";
    config1.stun_server_port = 55001;
    config1.bind_address = "127.0.0.1";

    config1.cb_state_changed = on_state_changed1;
    config1.cb_candidate = on_candidate1;
    config1.cb_gathering_done = on_gathering_done1;
    config1.cb_recv = on_recv1;
    config1.user_ptr = this;

    mNatAgentA = juice_create(&config1);

    // Agent 2: Create agent
    juice_config_t config2;
    memset(&config2, 0, sizeof(config2));

    // STUN server example
    config2.stun_server_host = "witcore.cn";
    config2.stun_server_port = 55001;
    config2.bind_address = "127.0.0.1";

    // Port range example
    config2.local_port_range_begin = 60000;
    config2.local_port_range_end = 61000;

    config2.cb_state_changed = on_state_changed2;
    config2.cb_candidate = on_candidate2;
    config2.cb_gathering_done = on_gathering_done2;
    config2.cb_recv = on_recv2;
    config2.user_ptr = this;

    mNatAgentB = juice_create(&config2);

    // Agent 1: Generate local description
    char sdp1[JUICE_MAX_SDP_STRING_LEN];
    juice_get_local_description(mNatAgentA, sdp1, JUICE_MAX_SDP_STRING_LEN);
    DLOG(ELL_INFO, "Local description 1:\n%s\n", sdp1);

    // Agent 2: Receive description from agent 1
    juice_set_remote_description(mNatAgentB, sdp1);

    // Agent 2: Generate local description
    char sdp2[JUICE_MAX_SDP_STRING_LEN];
    juice_get_local_description(mNatAgentB, sdp2, JUICE_MAX_SDP_STRING_LEN);
    DLOG(ELL_INFO, "Local description 2:\n%s\n", sdp2);

    // Agent 1: Receive description from agent 2
    juice_set_remote_description(mNatAgentA, sdp2);

    // Agent 1: Gather candidates (and send them to agent 2)
    juice_gather_candidates(mNatAgentA);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Agent 2: Gather candidates (and send them to agent 1)
    juice_gather_candidates(mNatAgentB);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // -- Connection should be finished --

    // Check states
    juice_state_t state1 = juice_get_state(mNatAgentA);
    juice_state_t state2 = juice_get_state(mNatAgentB);
    bool success = (state1 == JUICE_STATE_COMPLETED && state2 == JUICE_STATE_COMPLETED);

    // Retrieve candidates
    char local[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
    char remote[JUICE_MAX_CANDIDATE_SDP_STRING_LEN];
    if (success &= (juice_get_selected_candidates(mNatAgentA, local, JUICE_MAX_CANDIDATE_SDP_STRING_LEN, remote,
                        JUICE_MAX_CANDIDATE_SDP_STRING_LEN)
                    == 0)) {
        DLOG(ELL_INFO, "Local candidate  1: %s\n", local);
        DLOG(ELL_INFO, "Remote candidate 1: %s\n", remote);
        if ((!strstr(local, "typ host") && !strstr(local, "typ prflx"))
            || (!strstr(remote, "typ host") && !strstr(remote, "typ prflx")))
            success = false; // local connection should be possible
    }
    if (success &= (juice_get_selected_candidates(mNatAgentB, local, JUICE_MAX_CANDIDATE_SDP_STRING_LEN, remote,
                        JUICE_MAX_CANDIDATE_SDP_STRING_LEN)
                    == 0)) {
        DLOG(ELL_INFO, "Local candidate  2: %s\n", local);
        DLOG(ELL_INFO, "Remote candidate 2: %s\n", remote);
        if ((!strstr(local, "typ host") && !strstr(local, "typ prflx"))
            || (!strstr(remote, "typ host") && !strstr(remote, "typ prflx")))
            success = false; // local connection should be possible
    }

    // Retrieve addresses
    char localAddr[JUICE_MAX_ADDRESS_STRING_LEN];
    char remoteAddr[JUICE_MAX_ADDRESS_STRING_LEN];
    if (success &= (juice_get_selected_addresses(mNatAgentA, localAddr, JUICE_MAX_ADDRESS_STRING_LEN, remoteAddr,
                        JUICE_MAX_ADDRESS_STRING_LEN)
                    == 0)) {
        DLOG(ELL_INFO, "Local address  1: %s\n", localAddr);
        DLOG(ELL_INFO, "Remote address 1: %s\n", remoteAddr);
    }
    if (success &= (juice_get_selected_addresses(mNatAgentB, localAddr, JUICE_MAX_ADDRESS_STRING_LEN, remoteAddr,
                        JUICE_MAX_ADDRESS_STRING_LEN)
                    == 0)) {
        DLOG(ELL_INFO, "Local address  2: %s\n", localAddr);
        DLOG(ELL_INFO, "Remote address 2: %s\n", remoteAddr);
    }

    // Agent 1: destroy
    juice_destroy(mNatAgentA);

    // Agent 2: destroy
    juice_destroy(mNatAgentB);

    if (success) {
        DLOG(ELL_INFO, "Success\n");
        return 0;
    } else {
        DLOG(ELL_INFO, "Failure\n");
        return -1;
    }
}



} // namespace net
} // namespace app