#include <z_bmqa_session.h>
#include <z_bmqt_sessionoptions.h>
#include <bmqa_session.h>
#include <bmqt_sessionoptions.h>
#include <bmqa_openqueuestatus.h>


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

// int z_bmqa_Session__getQueueIdWithUri(z_bmqa_Session* session_obj, z_bmqa_QueueId** queueId, const z_bmqt_Uri* uri){
//     using namespace BloombergLP;
    
//     bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
//     bmqa::Q* buffer_ptr = reinterpret_cast<bmqa::MessageProperties*>(uri);

//     session_ptr->getQueueId(buffer_ptr);
//     return 0;
// }


// int z_bmqa_Session__openQueueSync(z_bmqa_Session* session_obj,z_bmqa_QueueId* queueId,const z_bmqt_Uri* uri,uint64_t flags /*,z_bmqa_OpenQueueStatus* out_obj*/){
//     using namespace BloombergLP;

//     bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
//     bmqt::Uri const * uri_ptr = reinterpret_cast< bmqt::Uri const *>(uri);
//     bmqa::QueueId* queueid_ptr = reinterpret_cast<bmqa::QueueId*>(queueId);

//     //must populate out obj in future
//     session_ptr->openQueueSync(queueid_ptr, *uri_ptr, flags);
//     return 0;
// }

int z_bmqa_Session__openQueueSync(z_bmqa_Session* session_obj,z_bmqa_QueueId* queueId,const char * uri,uint64_t flags /*,z_bmqa_OpenQueueStatus* out_obj*/){
    using namespace BloombergLP;

    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
        bmqa::QueueId* queueid_ptr = reinterpret_cast<bmqa::QueueId*>(queueId);

    //must populate out obj in future
    session_ptr->openQueueSync(queueid_ptr, uri, flags);
    return 0;
}

int z_bmqa_Session__closeQueueSync(z_bmqa_Session* session_obj, z_bmqa_QueueId* queueId /*,z_bmqa_CloseQueueStatus**/){
    using namespace BloombergLP;

    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::QueueId* queueid_ptr = reinterpret_cast<bmqa::QueueId*>(queueId);

    //must populate out obj in future
    session_ptr->closeQueueSync(queueid_ptr); // not using timeout (we should definitely add this)
    return 0;
}

int z_bmqa_Session__loadMessageEventBuilder(z_bmqa_Session* session_obj, z_bmqa_MessageEventBuilder** builder){
    using namespace BloombergLP;

    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder);

    session_ptr->loadMessageEventBuilder(builder_ptr);
    return 0;
}

int z_bmqa_Session__post(z_bmqa_Session* session_obj, const z_bmqa_MessageEvent* event){
    using namespace BloombergLP;

    bmqa::Session* session_ptr = reinterpret_cast<bmqa::Session*>(session_obj);
    const bmqa::MessageEvent* event_ptr = reinterpret_cast<const bmqa::MessageEvent*>(event);

    session_ptr->post(*event_ptr);
    return 0;
}


