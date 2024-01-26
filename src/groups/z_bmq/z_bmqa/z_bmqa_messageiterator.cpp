#include <bmqa_messageiterator.h>
#include <z_bmqa_messageiterator.h>

int z_bmqa_MessageIterator__create(
    z_bmqa_MessageIterator** messageIterator_obj)
{
    using namespace BloombergLP;

    bmqa::MessageIterator* messageIterator_p = new bmqa::MessageIterator();

    *messageIterator_obj = reinterpret_cast<z_bmqa_MessageIterator*>(
        messageIterator_p);

    return 0;
}

int z_bmqa_MessageIterator__delete(
    z_bmqa_MessageIterator** messageIterator_obj)
{
    using namespace BloombergLP;

    bmqa::MessageIterator* messageIterator_p =
        reinterpret_cast<bmqa::MessageIterator*>(messageIterator_obj);
    delete messageIterator_p;
    *messageIterator_obj = NULL;

    return 0;
}

bool z_bmqa_MessageIterator__nextMessage(
    z_bmqa_MessageIterator* messageIterator_obj)
{
    using namespace BloombergLP;

    bmqa::MessageIterator* messageIterator_p =
        reinterpret_cast<bmqa::MessageIterator*>(messageIterator_obj);

    return messageIterator_p->nextMessage();
}

int z_bmqa_MessageIterator__message(
    const z_bmqa_MessageIterator* messageIterator_obj,
    z_bmqa_Message const**        message_obj)
{
    using namespace BloombergLP;

    const bmqa::MessageIterator* messageIterator_p =
        reinterpret_cast<const bmqa::MessageIterator*>(messageIterator_obj);
    const z_bmqa_Message* message_p = reinterpret_cast<const z_bmqa_Message*>(
        &messageIterator_p->message());

    *message_obj = reinterpret_cast<const z_bmqa_Message*>(message_p);

    return 0;
}