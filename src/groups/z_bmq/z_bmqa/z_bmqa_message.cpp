#include <bdlbb_blob.h>
#include <bdlbb_blobutil.h>
#include <bmqa_message.h>
#include <bmqa_messageproperties.h>
#include <bsl_string.h>
#include <z_bmqa_message.h>

int z_bmqa_Message__delete(z_bmqa_Message** message_obj)
{
    using namespace BloombergLP;

    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(*message_obj);
    delete message_p;
    *message_obj = NULL;

    return 0;
}

int z_bmqa_Message__deleteConst(z_bmqa_Message const** message_obj)
{
    using namespace BloombergLP;

    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        *message_obj);
    delete message_p;
    *message_obj = NULL;

    return 0;
}

int z_bmqa_Message__create(z_bmqa_Message** message_obj)
{
    using namespace BloombergLP;

    bmqa::Message* message_p = new bmqa::Message();

    *message_obj = reinterpret_cast<z_bmqa_Message*>(message_p);
    return 0;
}

int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj,
                               const char*     data,
                               int             length)
{
    using namespace BloombergLP;

    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);

    message_p->setDataRef(data, length);
    return 0;
}

int z_bmqa_Message__setPropertiesRef(
    z_bmqa_Message*                 message_obj,
    const z_bmqa_MessageProperties* properties_obj)
{
    using namespace BloombergLP;

    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);
    const bmqa::MessageProperties* properties_p =
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    message_p->setPropertiesRef(properties_p);

    return 0;
}

int z_bmqa_Message__clearPropertiesRef(z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);
    message_p->clearPropertiesRef();

    return 0;
}

int z_bmqa_Message__setCorrelationId(z_bmqa_Message*             message_obj,
                                     const z_bmqt_CorrelationId* correlationId)
{
    using namespace BloombergLP;
    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId);
    message_p->setCorrelationId(*correlationId_p);

    return 0;
}

int z_bmqa_Message__setCompressionAlgorithmType(
    z_bmqa_Message*                       message_obj,
    z_bmqt_CompressionAlgorithmType::Enum value)
{
    using namespace BloombergLP;
    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);
    bmqt::CompressionAlgorithmType::Enum compressionType =
        static_cast<bmqt::CompressionAlgorithmType::Enum>(value);
    message_p->setCompressionAlgorithmType(compressionType);

    return 0;
}

#ifdef BMQ_ENABLE_MSG_GROUPID

int z_bmqa_Message__setGroupId(z_bmqa_Message* message_obj,
                               const char*     groupId)
{
    using namespace BloombergLP;
    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);
    bsl::string    groupId_str(groupId);
    message_p->setGroupId(groupId_str);

    return 0;
}

int z_bmqa_Message__clearGroupId(z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    bmqa::Message* message_p = reinterpret_cast<bmqa::Message*>(message_obj);
    message_p->clearGroupId();

    return 0;
}

#endif

int z_bmqa_Message__clone(const z_bmqa_Message* message_obj,
                          z_bmqa_Message**      other_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);

    bmqa::Message* other_p = new bmqa::Message();
    *other_p               = message_p->clone();
    *other_obj             = reinterpret_cast<z_bmqa_Message*>(other_p);

    return 0;
}

int z_bmqa_Message__queueId(const z_bmqa_Message*  message_obj,
                            z_bmqa_QueueId const** queueId_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    *queueId_obj = reinterpret_cast<const z_bmqa_QueueId*>(
        &(message_p->queueId()));

    return 0;
}

int z_bmqa_Message__correlationId(
    const z_bmqa_Message*        message_obj,
    z_bmqt_CorrelationId const** correlationId_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    *correlationId_obj = reinterpret_cast<const z_bmqt_CorrelationId*>(
        &(message_p->correlationId()));

    return 0;
}

int z_bmqa_Message__subscriptionHandle(
    const z_bmqa_Message*             message_obj,
    z_bmqt_SubscriptionHandle const** subscription_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    *subscription_obj = reinterpret_cast<const z_bmqt_SubscriptionHandle*>(
        &(message_p->subscriptionHandle()));

    return 0;
}

z_bmqt_CompressionAlgorithmType::Enum
z_bmqa_Message__compressionAlgorithmType(const z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);

    return static_cast<z_bmqt_CompressionAlgorithmType::Enum>(
        message_p->compressionAlgorithmType());
}

#ifdef BMQ_ENABLE_MSG_GROUPID

const char* z_bmqa_Message__groupId(const z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);

    return message_p->groupId().c_str();
}

#endif

int z_bmqa_Message__messageGUID(const z_bmqa_Message*      message_obj,
                                z_bmqt_MessageGUID const** messageGUID_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    *messageGUID_obj = reinterpret_cast<const z_bmqt_MessageGUID*>(
        &(message_p->messageGUID()));

    return 0;
}

int z_bmqa_Message__confirmationCookie(
    const z_bmqa_Message*              message_obj,
    z_bmqa_MessageConfirmationCookie** cookie_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    bmqa::MessageConfirmationCookie* cookie_p =
        new bmqa::MessageConfirmationCookie(message_p->confirmationCookie());
    *cookie_obj = reinterpret_cast<z_bmqa_MessageConfirmationCookie*>(
        cookie_p);

    return 0;
}

int z_bmqa_Message__ackStatus(const z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    return message_p->ackStatus();
}

// Add once we figure out how to handle Blobs in C
// int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, z_bdlbb_Blob*
// blob) {
//     using namespace BloombergLP;
// const bmqa::Message* message_p = reinterpret_cast<const
// bmqa::Message*>(message_obj);
// }

int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, char** buffer)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);

    bdlbb::Blob       data;
    int               rc = message_p->getData(&data);
    bsl::stringstream ss;
    ss << bdlbb::BlobUtilHexDumper(&data);
    bsl::string data_str         = ss.str();
    *buffer                      = new char[data_str.length() + 1];
    (*buffer)[data_str.length()] = '\0';
    strcpy(*buffer, data_str.c_str());

    return rc;
}

int z_bmqa_Message__dataSize(const z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);

    return message_p->dataSize();
}

bool z_bmqa_Message__hasProperties(const z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    return message_p->hasProperties();
}

#ifdef BMQ_ENABLE_MSG_GROUPID

bool z_bmqa_Message__hasGroupId(const z_bmqa_Message* message_obj)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    return message_p->hasGroupId();
}

#endif

int z_bmqa_Message__loadProperties(const z_bmqa_Message*     message_obj,
                                   z_bmqa_MessageProperties* buffer)
{
    using namespace BloombergLP;
    const bmqa::Message* message_p = reinterpret_cast<const bmqa::Message*>(
        message_obj);
    bmqa::MessageProperties* buffer_p =
        reinterpret_cast<bmqa::MessageProperties*>(buffer);
    return message_p->loadProperties(buffer_p);
}
