#include <z_bmqa_confirmeventbuilder.h>
#include <bmqa_confirmeventbuilder.h>


int z_bmqa_ConfirmEventBuilder__create(z_bmqa_ConfirmEventBuilder** builder_obj) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_ptr = new bmqa::ConfirmEventBuilder();
    *builder_obj = reinterpret_cast<z_bmqa_ConfirmEventBuilder*>(builder_ptr);

    return 0;
}

int z_bmqa_ConfirmEventBuilder__addMessageConfirmation(z_bmqa_ConfirmEventBuilder* builder_obj,
                                                       const z_bmqa_Message* message) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_ptr = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder_obj);
    const bmqa::Message* message_ptr = reinterpret_cast<const bmqa::Message*>(message);
    return builder_ptr->addMessageConfirmation(*message_ptr);
}

int z_bmqa_ConfirmEventBuilder__addMessageConfirmationWithCookie(z_bmqa_ConfirmEventBuilder* builder_obj,
                                                                 const z_bmqa_MessageConfirmationCookie* cookie) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_ptr = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder_obj);
    const bmqa::MessageConfirmationCookie* cookie_ptr = reinterpret_cast<const bmqa::MessageConfirmationCookie*>(cookie);
    return builder_ptr->addMessageConfirmation(*cookie_ptr);
}

int z_bmqa_ConfirmEventBuilder__reset(z_bmqa_ConfirmEventBuilder* builder_obj) {
    using namespace BloombergLP;

    bmqa::ConfirmEventBuilder* builder_ptr = reinterpret_cast<bmqa::ConfirmEventBuilder*>(builder_obj);
    builder_ptr->reset();

    return 0;
}

int z_bmqa_ConfirmEventBuilder__messageCount(const z_bmqa_ConfirmEventBuilder* builder_obj) {
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_ptr = reinterpret_cast<const bmqa::ConfirmEventBuilder*>(builder_obj);
    return builder_ptr->messageCount();
}

int z_bmqa_ConfirmEventBuilder__blob(z_bmqa_ConfirmEventBuilder* builder_obj, z_bmqt_Blob const** blob_obj) {
    using namespace BloombergLP;

    const bmqa::ConfirmEventBuilder* builder_ptr = reinterpret_cast<const bmqa::ConfirmEventBuilder*>(builder_obj);

    const bdlbb::Blob* blob_ptr = &(builder_ptr->blob());
    *blob_obj = reinterpret_cast<const z_bmqt_Blob*>(blob_ptr);

    return 0;
}