#include <z_bmqa_session.h>
#include <z_bmqt_sessionoptions.h>
#include <bmqa_session.h>
#include <bmqt_sessionoptions.h>

typedef void* TimeInterval;

namespace BloombergLP {

namespace z_bmqa {

int z_bmqa_session__create(z_bmqa_Session** session , z_bmqt_SessionOptions* options) {
    bmqt::SessionOptions* option_ptr = reinterpret_cast<bmqt::SessionOptions*>(options);
    bmqa::Session* session_ptr = new bmqa::Session(*option_ptr);
    *session = reinterpret_cast<z_bmqa_Session*>(session_ptr);
    return 0;
}

int z_bmqa_Session__start(z_bmqa_Session* session, int64_t timeoutMs) {
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session);
    bsls::TimeInterval timeout;
    timeout.addMilliseconds(timeoutMs);
    int rc = session_ptr->start(timeout);
    return rc;
}

int z_bmqa_Session__stop(z_bmqa_Session* session) {
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session);
    session_ptr->stop();

    return 0;
}

}

}
