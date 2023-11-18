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


int z_bmqa_Session__finalizeStop(z_bmqa_Session* session_obj){
    using namespace BloombergLP;
    
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    session_ptr->finalizeStop();
    return 0;
}


int z_bmqa_Session__finalizeStop(z_bmqa_Session* session_obj){
    using namespace BloombergLP;
    
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    session_ptr->finalizeStop();
    return 0;
}

int z_bmqa_Session__loadMessageEventBuilder(z_bmqa_Session* session_obj, z_bmqa_MessageEventBuilder** builder){
    using namespace BloombergLP;
    
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder);

    session_ptr->loadMessageEventBuilder(builder_ptr);
    return 0;
}

int z_bmqa_Session__loadConfirmEventBuilder(z_bmqa_Session* session_obj, z_bmqa_ConfirmEventBuilder** builder){
    using namespace BloombergLP;
    
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::ConfirmEventBuilder* builder_ptr = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder);

    session_ptr->loadConfirmEventBuilder(builder_ptr);
    return 0;
}

int z_bmqa_Session__loadMessageProperties(z_bmqa_Session* session_obj, z_bmqa_MessageProperties** buffer){
    using namespace BloombergLP;
    
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::MessageProperties* buffer_ptr = reinterpret_cast<bmqa::MessageProperties*>(buffer);

    session_ptr->loadMessageProperties(buffer_ptr);
    return 0;
}

int z_bmqa_Session__getQueueIdWithUri(z_bmqa_Session* session_obj, z_bmqa_QueueId** queueId, const z_bmqt_Uri* uri){
    using namespace BloombergLP;
    
    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::Q* buffer_ptr = reinterpret_cast<bmqa::MessageProperties*>(uri);

    session_ptr->getQueueId(buffer_ptr);
    return 0;
}