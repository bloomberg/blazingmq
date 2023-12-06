#include <z_bmqa_session.h>
#include <z_bmqt_sessionoptions.h>
#include <bmqa_session.h>
#include <bmqt_sessionoptions.h>
#include <bmqa_openqueuestatus.h>


int z_bmqa_Session__delete(z_bmqa_Session** session_obj) {
    using namespace BloombergLP;

    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(*session_obj);
    delete session_p;
    *session_obj = NULL;

    return 0;
}

int z_bmqa_Session__create(z_bmqa_Session** session_obj , const z_bmqt_SessionOptions* options) {
    using namespace BloombergLP;

    const bmqt::SessionOptions* options_p = reinterpret_cast<const bmqt::SessionOptions*>(options);
    bmqa::Session* session_p = new bmqa::Session(*options_p);
    *session_obj = reinterpret_cast<z_bmqa_Session*>(session_p);
    return 0;
}

int z_bmqa_Session__start(z_bmqa_Session* session_obj, int64_t timeoutMs) {
    using namespace BloombergLP;

    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    if(timeoutMs != 0) {
        bsls::TimeInterval timeout;
        timeout.addMilliseconds(timeoutMs);
        return session_p->start(timeout);
    } else {
        return session_p->start();
    }
}

int z_bmqa_Session__stop(z_bmqa_Session* session_obj) {
    using namespace BloombergLP;

    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    session_p->stop();

    return 0;
} 


int z_bmqa_Session__finalizeStop(z_bmqa_Session* session_obj){
    using namespace BloombergLP;
    
    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    session_p->finalizeStop();
    return 0;
}



int z_bmqa_Session__loadMessageEventBuilder(z_bmqa_Session* session_obj, z_bmqa_MessageEventBuilder** builder){
    using namespace BloombergLP;
    
    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(*builder);

    session_p->loadMessageEventBuilder(builder_p);
    return 0;
}

int z_bmqa_Session__loadConfirmEventBuilder(z_bmqa_Session* session_obj, z_bmqa_ConfirmEventBuilder** builder){
    using namespace BloombergLP;
    
    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder);

    session_p->loadConfirmEventBuilder(builder_p);
    return 0;
}

int z_bmqa_Session__loadMessageProperties(z_bmqa_Session* session_obj, z_bmqa_MessageProperties** buffer){
    using namespace BloombergLP;
    
    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::MessageProperties* buffer_p = reinterpret_cast<bmqa::MessageProperties*>(buffer);

    session_p->loadMessageProperties(buffer_p);
    return 0;
}

// int z_bmqa_Session__getQueueIdWithUri(z_bmqa_Session* session_obj, z_bmqa_QueueId** queueId, const z_bmqt_Uri* uri){
//     using namespace BloombergLP;
    
//     bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
//     bmqa::Q* buffer_p = reinterpret_cast<bmqa::MessageProperties*>(uri);

//     session_p->getQueueId(buffer_p);
//     return 0;
// }


// int z_bmqa_Session__openQueueSync(z_bmqa_Session* session_obj,z_bmqa_QueueId* queueId,const z_bmqt_Uri* uri,uint64_t flags /*,z_bmqa_OpenQueueStatus* out_obj*/){
//     using namespace BloombergLP;

//     bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
//     bmqt::Uri const * uri_p = reinterpret_cast< bmqt::Uri const *>(uri);
//     bmqa::QueueId* queueid_p = reinterpret_cast<bmqa::QueueId*>(queueId);

//     //must populate out obj in future
//     session_p->openQueueSync(queueid_p, *uri_p, flags);
//     return 0;
// }

int z_bmqa_Session__openQueueSync(z_bmqa_Session* session_obj,z_bmqa_QueueId* queueId,const char * uri,uint64_t flags /*,z_bmqa_OpenQueueStatus* out_obj*/){
    using namespace BloombergLP;

    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
        bmqa::QueueId* queueid_p = reinterpret_cast<bmqa::QueueId*>(queueId);

    //must populate out obj in future
    session_p->openQueueSync(queueid_p, uri, flags);
    return 0;
}

int z_bmqa_Session__closeQueueSync(z_bmqa_Session* session_obj, z_bmqa_QueueId* queueId /*,z_bmqa_CloseQueueStatus**/){
    using namespace BloombergLP;

    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    bmqa::QueueId* queueid_p = reinterpret_cast<bmqa::QueueId*>(queueId);

    //must populate out obj in future
    session_p->closeQueueSync(queueid_p); // not using timeout (we should definitely add this)
    return 0;
}


int z_bmqa_Session__post(z_bmqa_Session* session_obj, const z_bmqa_MessageEvent* event){
    using namespace BloombergLP;

    bmqa::Session* session_p = reinterpret_cast<bmqa::Session*>(session_obj);
    const bmqa::MessageEvent* event_p = reinterpret_cast<const bmqa::MessageEvent*>(event);

    session_p->post(*event_p);
    return 0;
}


