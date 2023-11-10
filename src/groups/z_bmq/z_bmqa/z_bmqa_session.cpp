#include <z_bmqa_session.h>
#include <z_bmqt_sessionoptions.h>
#include <bmqa_session.h>
#include <bmqt_sessionoptions.h>


int z_bmqa_Session__create(z_bmqa_Session** session_obj , const z_bmqt_SessionOptions* options) {
    using namespace BloombergLP;

    const bmqt::SessionOptions* options_ptr = reinterpret_cast<const bmqt::SessionOptions*>(options);
    bmqa::Session* session_ptr = new bmqa::Session(*options_ptr);
    *session_obj = reinterpret_cast<z_bmqa_Session*>(session_ptr);
    return 0;
}

int z_bmqa_Session__start(z_bmqa_Session* session_obj, int64_t timeoutMs) {
    using namespace BloombergLP;

    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    if(timeoutMs != 0) {
        bsls::TimeInterval timeout;
        timeout.addMilliseconds(timeoutMs);
        return session_ptr->start(timeout);
    } else {
        return session_ptr->start();
    }
}

int z_bmqa_Session__stop(z_bmqa_Session* session_obj) {
    using namespace BloombergLP;

    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    session_ptr->stop();

    return 0;
}
