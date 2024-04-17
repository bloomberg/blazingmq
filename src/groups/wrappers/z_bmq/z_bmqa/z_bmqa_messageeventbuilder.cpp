#include <bmqa_messageevent.h>
#include <bmqa_messageeventbuilder.h>
#include <z_bmqa_messageeventbuilder.h>

int z_bmqa_MessageEventBuilder__delete(
    z_bmqa_MessageEventBuilder** messageEventBuilder)
{
    using namespace BloombergLP;

    BSLS_ASSERT(messageEventBuilder != NULL);

    bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<bmqa::MessageEventBuilder*>(*messageEventBuilder);
    delete builder_p;
    *messageEventBuilder = NULL;

    return 0;
}

int z_bmqa_MessageEventBuilder__create(
    z_bmqa_MessageEventBuilder** messageEventBuilder)
{
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = new bmqa::MessageEventBuilder();
    *messageEventBuilder = reinterpret_cast<z_bmqa_MessageEventBuilder*>(builder_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__startMessage(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    z_bmqa_Message**            messageOut)
{
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<bmqa::MessageEventBuilder*>(messageEventBuilder);
    bmqa::Message* message_p = &builder_p->startMessage();
    *out_obj                 = reinterpret_cast<z_bmqa_Message*>(message_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__packMessage(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    const z_bmqa_QueueId*       queueId)
{
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<bmqa::MessageEventBuilder*>(messageEventBuilder);
    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    return builder_p->packMessage(*queueId_p);
}

int z_bmqa_MessageEventBuilder__reset(z_bmqa_MessageEventBuilder* messageEventBuilder)
{
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<bmqa::MessageEventBuilder*>(messageEventBuilder);
    builder_p->reset();

    return 0;
}

int z_bmqa_MessageEventBuilder__messageEvent(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    const z_bmqa_MessageEvent** messageEvent)
{
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<bmqa::MessageEventBuilder*>(messageEventBuilder);
    const bmqa::MessageEvent* event_p = &(builder_p->messageEvent());
    *event_obj = reinterpret_cast<const z_bmqa_MessageEvent*>(event_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__currentMessage(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    z_bmqa_Message**            message)
{
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<bmqa::MessageEventBuilder*>(messageEventBuilder);
    bmqa::Message* message_p = &(builder_p->currentMessage());
    *message_obj             = reinterpret_cast<z_bmqa_Message*>(message_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__messageCount(
    const z_bmqa_MessageEventBuilder* messageEventBuilder)
{
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<const bmqa::MessageEventBuilder*>(messageEventBuilder);
    return builder_p->messageCount();
}

int z_bmqa_MessageEventBuilder__messageEventSize(
    const z_bmqa_MessageEventBuilder* messageEventBuilder)
{
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_p =
        reinterpret_cast<const bmqa::MessageEventBuilder*>(messageEventBuilder);
    return builder_p->messageEventSize();
}