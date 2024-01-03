#include <z_bmqa_messageeventbuilder.h>
#include <bmqa_messageeventbuilder.h>
#include <bmqa_messageevent.h>



int z_bmqa_MessageEventBuilder__delete(z_bmqa_MessageEventBuilder** builder_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(*builder_obj);
    delete builder_p;
    *builder_obj = NULL;

    return 0;
}

int z_bmqa_MessageEventBuilder__deleteConst(z_bmqa_MessageEventBuilder const** builder_obj) {
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_p = reinterpret_cast<const bmqa::MessageEventBuilder*>(*builder_obj);
    delete builder_p;
    *builder_obj = NULL;

    return 0;
}

int z_bmqa_MessageEventBuilder__create(z_bmqa_MessageEventBuilder** builder_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = new bmqa::MessageEventBuilder();
    *builder_obj = reinterpret_cast<z_bmqa_MessageEventBuilder*>(builder_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__startMessage(z_bmqa_MessageEventBuilder* builder_obj, z_bmqa_Message** out_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    bmqa::Message* message_p = &builder_p->startMessage();
    *out_obj = reinterpret_cast<z_bmqa_Message*>(message_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__packMessage(z_bmqa_MessageEventBuilder* builder_obj, const z_bmqa_QueueId* queueId) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(queueId);
    return builder_p->packMessage(*queueId_p);
}

int z_bmqa_MessageEventBuilder__reset(z_bmqa_MessageEventBuilder* builder_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    builder_p->reset();

    return 0;
}

int z_bmqa_MessageEventBuilder__messageEvent(z_bmqa_MessageEventBuilder* builder_obj, z_bmqa_MessageEvent const** event_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    const bmqa::MessageEvent* event_p = &(builder_p->messageEvent());
    *event_obj = reinterpret_cast<const z_bmqa_MessageEvent*>(event_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__currentMessage(z_bmqa_MessageEventBuilder* builder_obj, z_bmqa_Message** message_obj) {
    using namespace BloombergLP;

    bmqa::MessageEventBuilder* builder_p = reinterpret_cast<bmqa::MessageEventBuilder*>(builder_obj);
    bmqa::Message* message_p = &(builder_p->currentMessage());
    *message_obj = reinterpret_cast<z_bmqa_Message*>(message_p);

    return 0;
}

int z_bmqa_MessageEventBuilder__messageCount(const z_bmqa_MessageEventBuilder* builder_obj) {
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_p = reinterpret_cast<const bmqa::MessageEventBuilder*>(builder_obj);
    return builder_p->messageCount();
}

int z_bmqa_MessageEventBuilder__messageEventSize(const z_bmqa_MessageEventBuilder* builder_obj) {
    using namespace BloombergLP;

    const bmqa::MessageEventBuilder* builder_p = reinterpret_cast<const bmqa::MessageEventBuilder*>(builder_obj);
    return builder_p->messageEventSize();
}