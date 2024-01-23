#ifndef INCLUDED_Z_BMQA_SESSION
#define INCLUDED_Z_BMQA_SESSION

#include <stdint.h>
#include <z_bmqa_closequeuestatus.h>
#include <z_bmqa_configurequeuestatus.h>
#include <z_bmqa_confirmeventbuilder.h>
#include <z_bmqa_event.h>
#include <z_bmqa_message.h>
#include <z_bmqa_messageevent.h>
#include <z_bmqa_messageeventbuilder.h>
#include <z_bmqa_messageproperties.h>
#include <z_bmqa_openqueuestatus.h>
#include <z_bmqa_queueid.h>
#include <z_bmqt_sessionoptions.h>
#include <z_bmqt_uri.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_SessionEventHandler z_bmqa_SessionEventHandler;

typedef void (*z_bmqa_OnSessionEventCb)(
    const z_bmqa_SessionEvent* sessionEvent,
    void*                      data);
typedef void (*z_bmqa_OnMessageEventCb)(
    const z_bmqa_MessageEvent* messageEvent,
    void*                      data);

int z_bmqa_SessionEventHandler__create(
    z_bmqa_SessionEventHandler** eventHandler_obj,
    z_bmqa_OnSessionEventCb      onSessionEvent,
    z_bmqa_OnMessageEventCb      onMessageEvent);

int z_bmqa_SessionEventHandler__delete(
    z_bmqa_SessionEventHandler** eventHandler_obj);

typedef struct z_bmqa_MessageConfirmationCookie
    z_bmqa_MessageConfirmationCookie;

typedef struct z_bmqa_Session z_bmqa_Session;

int z_bmqa_Session__delete(z_bmqa_Session** session_obj);

int z_bmqa_Session__create(z_bmqa_Session**             session_obj,
                           const z_bmqt_SessionOptions* options);

int z_bmqa_Session__createAsync(z_bmqa_Session**             session_obj,
                                z_bmqa_SessionEventHandler*  eventHandler,
                                const z_bmqt_SessionOptions* options);

int z_bmqa_Session__start(z_bmqa_Session* session_obj, int64_t timeoutMs);

int z_bmqa_Session__startAsync(z_bmqa_Session* session_obj, int64_t timeoutMs);

int z_bmqa_Session__stop(z_bmqa_Session* session_obj);

int z_bmqa_Session__stopAsync(z_bmqa_Session* session_obj);

int z_bmqa_Session__finalizeStop(z_bmqa_Session* session_obj);

int z_bmqa_Session__loadMessageEventBuilder(
    z_bmqa_Session*              session_obj,
    z_bmqa_MessageEventBuilder** builder);

int z_bmqa_Session__loadConfirmEventBuilder(
    z_bmqa_Session*              session_obj,
    z_bmqa_ConfirmEventBuilder** builder);

int z_bmqa_Session__loadMessageProperties(z_bmqa_Session* session_obj,
                                          z_bmqa_MessageProperties** buffer);

// int z_bmqa_Session__getQueueIdWithUri(z_bmqa_Session*   session_obj,
//                                       z_bmqa_QueueId**  queueId,
//                                       const z_bmqt_Uri* uri);

int z_bmqa_Session__getQueueIdWithCorrelationId(
    z_bmqa_Session*             session_obj,
    z_bmqa_QueueId**            queueId,
    const z_bmqt_CorrelationId* correlationId);

int z_bmqa_Session__openQueueSync(z_bmqa_Session*          session_obj,
                                  z_bmqa_QueueId*          queueId,
                                  const char*              uri,
                                  uint64_t                 flags,
                                  z_bmqa_OpenQueueStatus** status);

int z_bmqa_Session__configureQueueSync(z_bmqa_Session*            session_obj,
                                       z_bmqa_QueueId*            queueId,
                                       const z_bmqt_QueueOptions* options,
                                       int64_t                    timeoutMs,
                                       z_bmqa_ConfigureQueueStatus** status);

int z_bmqa_Session__closeQueueSync(z_bmqa_Session*           session_obj,
                                   z_bmqa_QueueId*           queueId,
                                   int64_t                   timeoutMs,
                                   z_bmqa_CloseQueueStatus** status);

int z_bmqa_Session__closeQueueAsync(z_bmqa_Session* session_obj,
                                    z_bmqa_QueueId* queueId,
                                    int64_t         timeout);

int z_bmqa_Session_nextEvent(z_bmqa_Session* session_obj,
                             z_bmqa_Event**  event_obj,
                             int64_t         timeout);

int z_bmqa_Session__post(z_bmqa_Session*            session_obj,
                         const z_bmqa_MessageEvent* event);

int z_bmqa_Session__confirmMessage(z_bmqa_Session*       session_obj,
                                   const z_bmqa_Message* message);

int z_bmqa_Session__confirmMessageWithCookie(
    z_bmqa_Session*                         session_obj,
    const z_bmqa_MessageConfirmationCookie* cookie);

int z_bmqa_Session__confirmMessages(z_bmqa_Session*             session_obj,
                                    z_bmqa_ConfirmEventBuilder* builder);

#if defined(__cplusplus)
}

#include <bmqa_messageevent.h>
#include <bmqa_session.h>
#include <bmqa_sessionevent.h>
#include <bslmt_mutex.h>

class z_bmqa_CustomSessionEventHandler
: BloombergLP::bmqa::SessionEventHandler {
  private:
    z_bmqa_OnSessionEventCb onSessionEventCb;
    z_bmqa_OnMessageEventCb onMessageEventCb;
    void*                   data;
    uint64_t                mSize;

    BloombergLP::bslmt::Mutex mutex;

  public:
    z_bmqa_CustomSessionEventHandler(z_bmqa_OnSessionEventCb onSessionEventCb,
                                     z_bmqa_OnMessageEventCb onMessageEventCb);

    ~z_bmqa_CustomSessionEventHandler();

    void onSessionEvent(const BloombergLP::bmqa::SessionEvent& sessionEvent);

    void onMessageEvent(const BloombergLP::bmqa::MessageEvent& messageEvent);

    void* getData() { return this->data; }

    void lock();

    void unlock();

    void tryLock();
};
#endif

#endif
