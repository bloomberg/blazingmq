#include <bmqa_message.h>
#include <z_bmqa_message.h>

int z_bmqa_Message__delete(z_bmqa_Message** message_obj) {
    using namespace BloombergLP;

    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(*message_obj);
    delete message_p;
    *message_obj = NULL;

    return 0;
}

int z_bmqa_Message__deleteConst(z_bmqa_Message const** message_obj) {
    using namespace BloombergLP;

    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(*message_obj);
    delete message_p;
    *message_obj = NULL;

    return 0;
}

int z_bmqa_Message__createEmpty(z_bmqa_Message** message_obj){
    using namespace BloombergLP;

    bmqa::Message* message_p = new bmqa::Message();

    *message_obj = reinterpret_cast<z_bmqa_Message*>(message_p);
    return 0;
}

int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj, const char* data, int length){
    using namespace BloombergLP;

    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);

    message_p->setDataRef(data, length);
    return 0;
}