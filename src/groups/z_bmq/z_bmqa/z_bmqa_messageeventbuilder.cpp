#include <z_bmqa_messageeventbuilder.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageevent.h>



int z_bmqa_MessageEventBuilder__create(z_bmqa_MessageEventBuilder** builder_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_ptr = new bmqa::MessageEventBuilder();
    *builder_obj = reinterpret_cast<z_bmqa_MessageEventBuilder*>(builder_ptr);

    return 0;
}

int z_bmqa_MessageEventBuilder__startMessage(z_bmqa_MessageEventBuilder* builder_obj, z_bmqa_Message** out_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    bmqa::Message* message_ptr = &builder_ptr->startMessage();
    *out_obj = reinterpret_cast<z_bmqa_Message*>(message_ptr);

    return 0;
}

int z_bmqa_MessageEventBuilder__packMessage(z_bmqa_MessageEventBuilder* builder_obj, const z_bmqa_QueueId* queueId) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    const bmqa::QueueId* queueId_ptr = reinterpret_cast<const bmqa::QueueId*>(queueId);
    return builder_ptr->packMessage(*queueId_ptr);
}

int z_bmqa_MessageEventBuilder__reset(z_bmqa_MessageEventBuilder* builder_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    builder_ptr->reset();

    return 0;
}

int z_bmqa_MessageEventBuilder__messageEvent(z_bmqa_MessageEventBuilder* builder_obj, z_bmqa_MessageEvent const** event_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    const bmqa::MessageEvent* event_ptr = &(builder_ptr->messageEvent());
    *event_obj = reinterpret_cast<const z_bmqa_MessageEvent*>(event_ptr);

    return 0;
}

int z_bmqa_MessageEventBuilder__currentMessage(z_bmqa_MessageEventBuilder* builder_obj, z_bmqa_Message** message_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    bmqa::Message* message_ptr = &(builder_ptr->currentMessage());
    *message_obj = reinterpret_cast<z_bmqa_Message*>(message_ptr);

    return 0;
}

int z_bmqa_MessageEventBuilder__messageCount(const z_bmqa_MessageEventBuilder* builder_obj) {
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<const bmqa::MessageEventBuilder*>(builder_obj);
    return builder_ptr->messageCount();
}

int z_bmqa_MessageEventBuilder__messageEventSize(const z_bmqa_MessageEventBuilder* builder_obj) {
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_ptr = reinterpret_cast<const bmqa::MessageEventBuilder*>(builder_obj);
    return builder_ptr->messageEventSize();
}