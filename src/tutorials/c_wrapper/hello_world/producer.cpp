#include <string.h>
#include <z_bmqa_closequeuestatus.h>
#include <z_bmqa_message.h>
#include <z_bmqa_messageeventbuilder.h>
#include <z_bmqa_openqueuestatus.h>
#include <z_bmqa_queueid.h>
#include <z_bmqa_session.h>

const int  K_QUEUE_ID     = 1;
const char K_QUEUE_URI[]  = "bmq://bmq.test.mem.priority/test-queue";
const int  K_NUM_MESSAGES = 5;

enum QueueFlags {
    e_ADMIN = (1 << 0)  // The queue is opened in admin mode (Valid only
                        // for BlazingMQ admin tasks)
    ,
    e_READ = (1 << 1)  // The queue is opened for consuming messages
    ,
    e_WRITE = (1 << 2)  // The queue is opened for posting messages
    ,
    e_ACK = (1 << 3)  // Set to indicate interested in receiving
                      // 'ACK' events for all message posted
};

void postEvent(const char*     text,
               z_bmqa_QueueId* queueId,
               z_bmqa_Session* session)
{
    z_bmqa_MessageEventBuilder* builder;
    z_bmqa_MessageEventBuilder__create(&builder);

    z_bmqa_Session__loadMessageEventBuilder(session, builder);

    z_bmqa_Message* message;

    z_bmqa_MessageEventBuilder__startMessage(builder, &message);

    z_bmqa_Message__setDataRef(message, text, (int)strlen(text));

    z_bmqa_MessageEventBuilder__packMessage(builder, queueId);

    const z_bmqa_MessageEvent* messageEvent;
    z_bmqa_MessageEventBuilder__messageEvent(builder, &messageEvent);

    z_bmqa_Session__post(session, messageEvent);

    z_bmqa_MessageEventBuilder__delete(&builder);
}

void produce(z_bmqa_Session* session)
{
    z_bmqa_QueueId*          queueId;
    z_bmqa_OpenQueueStatus*  openStatus;
    z_bmqa_CloseQueueStatus* closeStatus;

    z_bmqa_QueueId__createFromNumeric(&queueId, K_QUEUE_ID);
    z_bmqa_Session__openQueueSync(session,
                                  queueId,
                                  K_QUEUE_URI,
                                  e_WRITE,
                                  &openStatus);

    const char* messages[] = {"Hello world!",
                              "message 1",
                              "message 2",
                              "message 3",
                              "Good Bye!"};
    for (int idx = 0; idx < 5; ++idx) {
        postEvent(messages[idx], queueId, session);
    }

    z_bmqa_Session__closeQueueSync(session, queueId, 0, &closeStatus);

    z_bmqa_QueueId__delete(&queueId);
    z_bmqa_OpenQueueStatus__delete(&openStatus);
    z_bmqa_CloseQueueStatus__delete(&closeStatus);
}

int main()
{
    z_bmqa_Session*        session;
    z_bmqt_SessionOptions* options;

    z_bmqt_SessionOptions__create(&options);
    z_bmqa_Session__create(&session, options);

    // start the session
    z_bmqa_Session__start(session, 1000);

    produce(session);

    // stop the session
    z_bmqa_Session__stop(session);

    z_bmqa_Session__delete(&session);
    z_bmqt_SessionOptions__delete(&options);

    return 0;
}
