#ifndef INCLUDED_Z_BMQA_MESSAGEEVENTBUILDER
#define INCLUDED_Z_BMQA_MESSAGEEVENTBUILDER

#include <z_bmqa_message.h>
#include <z_bmqa_messageevent.h>
#include <z_bmqa_queueid.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_MessageEventBuilder z_bmqa_MessageEventBuilder;

int z_bmqa_MessageEventBuilder__delete(
    z_bmqa_MessageEventBuilder** builder_obj);

int z_bmqa_MessageEventBuilder__deleteConst(
    z_bmqa_MessageEventBuilder const** builder_obj);

int z_bmqa_MessageEventBuilder__create(
    z_bmqa_MessageEventBuilder** builder_obj);

int z_bmqa_MessageEventBuilder__startMessage(
    z_bmqa_MessageEventBuilder* builder_obj,
    z_bmqa_Message**            out_obj);

int z_bmqa_MessageEventBuilder__packMessage(
    z_bmqa_MessageEventBuilder* builder_obj,
    const z_bmqa_QueueId*       queueId);

int z_bmqa_MessageEventBuilder__reset(z_bmqa_MessageEventBuilder* builder_obj);

int z_bmqa_MessageEventBuilder__messageEvent(
    z_bmqa_MessageEventBuilder* builder_obj,
    z_bmqa_MessageEvent const** event_obj);

int z_bmqa_MessageEventBuilder__currentMessage(
    z_bmqa_MessageEventBuilder* builder_obj,
    z_bmqa_Message**            message_obj);

int z_bmqa_MessageEventBuilder__messageCount(
    const z_bmqa_MessageEventBuilder* builder_obj);

int z_bmqa_MessageEventBuilder__messageEventSize(
    const z_bmqa_MessageEventBuilder* builder_obj);

#if defined(__cplusplus)
}
#endif

#endif