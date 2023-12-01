#include <bmqa_message.h>
#include <z_bmqa_message.h>

int z_bmqa_Message__createEmpty(z_bmqa_Message** message_obj){
    using namespace BloombergLP;

    bmqa::Message* message_ptr = new bmqa::Message();

    *message_obj = reinterpret_cast<z_bmqa_Message*>(message_ptr);
    return 0;
}

int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj, const char* data, int length){
    using namespace BloombergLP;

    bmqa::Message* message_ptr = reinterpret_cast<bmqa::Message*>(message_obj);

    message_ptr->setDataRef(data, length);
    return 0;
}