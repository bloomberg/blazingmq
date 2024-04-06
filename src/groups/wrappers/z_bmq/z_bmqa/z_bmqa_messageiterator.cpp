#include <bmqa_messageiterator.h>
#include <z_bmqa_messageiterator.h>

int z_bmqa_MessageIterator__delete(
    z_bmqa_MessageIterator** messageIterator)
{
    using namespace BloombergLP;

    BSLS_ASSERT(messageIterator != NULL);

    bmqa::MessageIterator* messageIterator_p =
        reinterpret_cast<bmqa::MessageIterator*>(*messageIterator);
    delete messageIterator_p;
    *messageIterator = NULL;

    return 0;
}

int z_bmqa_MessageIterator__create(
    z_bmqa_MessageIterator** messageIterator)
{
    using namespace BloombergLP;

    bmqa::MessageIterator* messageIterator_p = new bmqa::MessageIterator();

    *messageIterator = reinterpret_cast<z_bmqa_MessageIterator*>(
        messageIterator_p);

    return 0;
}

bool z_bmqa_MessageIterator__nextMessage(
    z_bmqa_MessageIterator* messageIterator)
{
    using namespace BloombergLP;

    bmqa::MessageIterator* messageIterator_p =
        reinterpret_cast<bmqa::MessageIterator*>(messageIterator);

    return messageIterator_p->nextMessage();
}

int z_bmqa_MessageIterator__message(
    const z_bmqa_MessageIterator* messageIterator,
    const z_bmqa_Message**        messageOut)
{
    using namespace BloombergLP;

    const bmqa::MessageIterator* messageIterator_p =
        reinterpret_cast<const bmqa::MessageIterator*>(messageIterator);
    const z_bmqa_Message* message_p = reinterpret_cast<const z_bmqa_Message*>(
        &messageIterator_p->message());

    *messageOut = reinterpret_cast<const z_bmqa_Message*>(message_p);

    return 0;
}