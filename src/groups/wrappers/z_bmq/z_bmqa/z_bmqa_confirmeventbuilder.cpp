#include <bmqa_confirmeventbuilder.h>
#include <z_bmqa_confirmeventbuilder.h>

int z_bmqa_ConfirmEventBuilder__delete(
    z_bmqa_ConfirmEventBuilder** confirmEventBuilder)
{
    using namespace BloombergLP;

    BSLS_ASSERT(confirmEventBuilder != NULL);

    bmqa::ConfirmEventBuilder* builder_p =
        reinterpret_cast<bmqa::ConfirmEventBuilder*>(*confirmEventBuilder);
    delete builder_p;
    *confirmEventBuilder = NULL;

    return 0;
}

int z_bmqa_ConfirmEventBuilder__create(
    z_bmqa_ConfirmEventBuilder** confirmEventBuilder)
{
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p = new bmqa::ConfirmEventBuilder();
    *confirmEventBuilder = reinterpret_cast<z_bmqa_ConfirmEventBuilder*>(builder_p);

    return 0;
}

int z_bmqa_ConfirmEventBuilder__addMessageConfirmation(
    z_bmqa_ConfirmEventBuilder* confirmEventBuilder,
    const z_bmqa_Message*       message)
{
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p =
        reinterpret_cast<bmqa::ConfirmEventBuilder*>(confirmEventBuilder);
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message);
    return builder_p->addMessageConfirmation(*message_p);
}

int z_bmqa_ConfirmEventBuilder__addMessageConfirmationWithCookie(
    z_bmqa_ConfirmEventBuilder*             confirmEventBuilder,
    const z_bmqa_MessageConfirmationCookie* cookie)
{
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p =
        reinterpret_cast<bmqa::ConfirmEventBuilder*>(confirmEventBuilder);
    const bmqa::MessageConfirmationCookie* cookie_p =
        reinterpret_cast<const bmqa::MessageConfirmationCookie*>(cookie);
    return builder_p->addMessageConfirmation(*cookie_p);
}

int z_bmqa_ConfirmEventBuilder__reset(z_bmqa_ConfirmEventBuilder* confirmEventBuilder)
{
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p =
        reinterpret_cast<bmqa::ConfirmEventBuilder*>(confirmEventBuilder);
    builder_p->reset();

    return 0;
}

int z_bmqa_ConfirmEventBuilder__messageCount(
    const z_bmqa_ConfirmEventBuilder* confirmEventBuilder)
{
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_p =
        reinterpret_cast<const bmqa::ConfirmEventBuilder*>(confirmEventBuilder);
    return builder_p->messageCount();
}

int z_bmqa_ConfirmEventBuilder__blob(z_bmqa_ConfirmEventBuilder* confirmEventBuilder,
                                     const z_bmqt_Blob**         blob_obj)
{
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_p =
        reinterpret_cast<const bmqa::ConfirmEventBuilder*>(confirmEventBuilder);

    const bdlbb::Blob* blob_p = &(builder_p->blob());
    *blob_obj                 = reinterpret_cast<const z_bmqt_Blob*>(blob_p);

    return 0;
}