#ifndef INCLUDED_Z_BMQA_MESSAGE
#define INCLUDED_Z_BMQA_MESSAGE

#include <stdbool.h>
#include <z_bmqa_messageproperties.h>
#include <z_bmqa_queueid.h>
#include <z_bmqt_compressionalgorithmtype.h>
#include <z_bmqt_correlationid.h>
#include <z_bmqt_messageguid.h>
#include <z_bmqt_subscription.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_Message z_bmqa_Message;

typedef struct z_bmqa_MessageConfirmationCookie
    z_bmqa_MessageConfirmationCookie;

int z_bmqa_Message__delete(z_bmqa_Message** message_obj);

int z_bmqa_Message__deleteConst(z_bmqa_Message const** message_obj);

int z_bmqa_Message__create(z_bmqa_Message** message_obj);

int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj,
                               const char*     data,
                               int             length);

int z_bmqa_Message__setPropertiesRef(
    z_bmqa_Message*                 message_obj,
    const z_bmqa_MessageProperties* properties_obj);

int z_bmqa_Message__clearPropertiesRef(z_bmqa_Message* message_obj);

int z_bmqa_Message__setCorrelationId(
    z_bmqa_Message*             message_obj,
    const z_bmqt_CorrelationId* correlationId);

int z_bmqa_Message__setCompressionAlgorithmType(
    z_bmqa_Message*                       message_obj,
    z_bmqt_CompressionAlgorithmType::Enum value);

#ifdef BMQ_ENABLE_MSG_GROUPID

int z_bmqa_Message__setGroupId(z_bmqa_Message* message_obj,
                               const char*     groupId);

int z_bmqa_Message__clearGroupId(z_bmqa_Message* message_obj);

#endif

int z_bmqa_Message__clone(const z_bmqa_Message* message_obj,
                          z_bmqa_Message**      other_obj);

int z_bmqa_Message__queueId(const z_bmqa_Message*  message_obj,
                            z_bmqa_QueueId const** queueId_obj);

int z_bmqa_Message__correlationId(
    const z_bmqa_Message*        message_obj,
    z_bmqt_CorrelationId const** correlationId_obj);

int z_bmqa_Message__subscriptionHandle(
    const z_bmqa_Message*             message_obj,
    z_bmqt_SubscriptionHandle const** subscription_obj);

z_bmqt_CompressionAlgorithmType::Enum
z_bmqa_Message__compressionAlgorithmType(const z_bmqa_Message* message_obj);

#ifdef BMQ_ENABLE_MSG_GROUPID

const char* z_bmqa_Message__groupId(const z_bmqa_Message* message_obj);

#endif

int z_bmqa_Message__messageGUID(const z_bmqa_Message*      message_obj,
                                z_bmqt_MessageGUID const** messageGUID_obj);

int z_bmqa_Message__confirmationCookie(
    const z_bmqa_Message*              message_obj,
    z_bmqa_MessageConfirmationCookie** cookie_obj);

int z_bmqa_Message__ackStatus(const z_bmqa_Message* message_obj);

// Add once we figure out how to handle Blobs in C
// int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, z_bdlbb_Blob*
// blob);

int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, char** buffer);

int z_bmqa_Message__dataSize(const z_bmqa_Message* message_obj);

bool z_bmqa_Message__hasProperties(const z_bmqa_Message* message_obj);

#ifdef BMQ_ENABLE_MSG_GROUPID

bool z_bmqa_Message__hasGroupId(const z_bmqa_Message* message_obj);

#endif

int z_bmqa_Message__loadProperties(const z_bmqa_Message*     message_obj,
                                   z_bmqa_MessageProperties* buffer);

#if defined(__cplusplus)
}
#endif

#endif