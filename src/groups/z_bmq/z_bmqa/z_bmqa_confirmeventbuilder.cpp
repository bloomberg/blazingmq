#include <z_bmqa_confirmeventbuilder.h>
#include <bmqa_confirmeventbuilder.h>


int z_bmqa_ConfirmEventBuilder__delete(z_bmqa_ConfirmEventBuilder const** builder_obj) {
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<const bmqa::ConfirmEventBuilder*>(*builder_obj);
    delete builder_p;
    *builder_obj = NULL;

    return 0;
}

int z_bmqa_ConfirmEventBuilder__create(z_bmqa_ConfirmEventBuilder** builder_obj) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p = new bmqa::ConfirmEventBuilder();
    *builder_obj = reinterpret_cast<z_bmqa_ConfirmEventBuilder*>(builder_p);

    return 0;
}

int z_bmqa_ConfirmEventBuilder__addMessageConfirmation(z_bmqa_ConfirmEventBuilder* builder_obj,
                                                       const z_bmqa_Message* message) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder_obj);
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(message);
    return builder_p->addMessageConfirmation(*message_p);
}

int z_bmqa_ConfirmEventBuilder__addMessageConfirmationWithCookie(z_bmqa_ConfirmEventBuilder* builder_obj,
                                                                 const z_bmqa_MessageConfirmationCookie* cookie) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder_obj);
    const bmqa::MessageConfirmationCookie* cookie_p = reinterpret_cast<const bmqa::MessageConfirmationCookie*>(cookie);
    return builder_p->addMessageConfirmation(*cookie_p);
}

int z_bmqa_ConfirmEventBuilder__reset(z_bmqa_ConfirmEventBuilder* builder_obj) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder_obj);
    builder_p->reset();

    return 0;
}

int z_bmqa_ConfirmEventBuilder__messageCount(const z_bmqa_ConfirmEventBuilder* builder_obj) {
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<const bmqa::ConfirmEventBuilder*>(builder_obj);
    return builder_p->messageCount();
}

int z_bmqa_ConfirmEventBuilder__blob(z_bmqa_ConfirmEventBuilder* builder_obj, z_bmqt_Blob const** blob_obj) {
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_p = reinterpret_cast<const bmqa::ConfirmEventBuilder*>(builder_obj);

    const bdlbb::Blob* blob_p = &(builder_p->blob());
    *blob_obj = reinterpret_cast<const z_bmqt_Blob*>(blob_p);

    return 0;
}