#include <z_bmqa_session.h>
#include <z_bmqt_sessionoptions.h>
#include <bmqa_session.h>
#include <bmqt_sessionoptions.h>

typedef void* TimeInterval;

namespace BloombergLP {

namespace z_bmqa {

int z_bmqa_session__create(z_bmqa_session* session , z_bmqt_SessionOptions* options) {
    bmqt::SessionOptions* option_ptr = reinterpret_cast<bmqt::SessionOptions*>(options);
    bmqa::Session* session_ptr = new bmqa::Session(*option_ptr);

    *session = reinterpret_cast<z_bmqa_session>(session_ptr);
    return 0;
}

int z_bmqa_session__start(z_bmqa_session* session, int64_t seconds, int nanoseconds) {
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session);
    bsls::TimeInterval timeout(seconds, nanoseconds);
    session_ptr->start(timeout);
    return 0;
}

int z_bmqa_session__stop(z_bmqa_session* session) {
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session);
    session_ptr->stop();

    return 0;
}

}

}
