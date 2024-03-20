#ifndef INCLUDED_Z_BMQA_MESSAGE
#define INCLUDED_Z_BMQA_MESSAGE

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_messageproperties.h>
#include <z_bmqa_queueid.h>
#include <z_bmqt_compressionalgorithmtype.h>
#include <z_bmqt_correlationid.h>
#include <z_bmqt_messageguid.h>
#include <z_bmqt_subscription.h>

typedef struct z_bmqa_Message z_bmqa_Message;

typedef struct z_bmqa_MessageConfirmationCookie
    z_bmqa_MessageConfirmationCookie;

/**
 * @brief Deletes a z_bmqa_Message object.
 * 
 * This function deletes the z_bmqa_Message object pointed to by 'message_obj'.
 * Upon successful completion, the memory pointed to by 'message_obj' will be deallocated,
 * and 'message_obj' will be set to NULL.
 * 
 * @param message_obj A pointer to a pointer to a z_bmqa_Message object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_Message__delete(z_bmqa_Message** message_obj);

/**
 * @brief Creates a new z_bmqa_Message object.
 * 
 * This function creates a new z_bmqa_Message object and stores
 * a pointer to it in the memory pointed to by 'message_obj'.
 * 
 * @param message_obj A pointer to a pointer to a z_bmqa_Message object where
 *                   the newly created object will be stored.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_Message__create(z_bmqa_Message** message_obj);

/**
 * @brief Sets the data of a z_bmqa_Message object.
 * 
 * This function sets the data of the z_bmqa_Message object 'message_obj' to the
 * provided 'data' with the given 'length'.
 * 
 * @param message_obj A pointer to a z_bmqa_Message object.
 * @param data        A pointer to the data.
 * @param length      The length of the data.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj,
                               const char*     data,
                               int             length);

/**
 * @brief Sets the properties of a z_bmqa_Message object.
 * 
 * This function sets the properties of the z_bmqa_Message object 'message_obj' to
 * the provided properties object 'properties_obj'.
 * 
 * @param message_obj   A pointer to a z_bmqa_Message object.
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object representing
 *                      the properties.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__setPropertiesRef(
    z_bmqa_Message*                 message_obj,
    const z_bmqa_MessageProperties* properties_obj);

/**
 * @brief Clears the properties of a z_bmqa_Message object.
 * 
 * This function clears the properties of the z_bmqa_Message object 'message_obj'.
 * 
 * @param message_obj A pointer to a z_bmqa_Message object.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__clearPropertiesRef(z_bmqa_Message* message_obj);

/**
 * @brief Sets the correlation ID of a z_bmqa_Message object.
 * 
 * This function sets the correlation ID of the z_bmqa_Message object 'message_obj'
 * to the provided correlation ID 'correlationId'.
 * 
 * @param message_obj    A pointer to a z_bmqa_Message object.
 * @param correlationId  A pointer to a z_bmqt_CorrelationId object representing
 *                      the correlation ID.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__setCorrelationId(
    z_bmqa_Message*             message_obj,
    const z_bmqt_CorrelationId* correlationId);

/**
 * @brief Sets the compression algorithm type of a z_bmqa_Message object.
 * 
 * This function sets the compression algorithm type of the z_bmqa_Message object
 * 'message_obj' to the provided value 'value'.
 * 
 * @param message_obj A pointer to a z_bmqa_Message object.
 * @param value       The compression algorithm type value.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__setCompressionAlgorithmType(
    z_bmqa_Message*                       message_obj,
    z_bmqt_CompressionAlgorithmType::Enum value);

#ifdef BMQ_ENABLE_MSG_GROUPID

/**
 * @brief Sets the group ID of a z_bmqa_Message object.
 * 
 * This function sets the group ID of the z_bmqa_Message object 'message_obj' to
 * the provided 'groupId'.
 * 
 * @param message_obj A pointer to a z_bmqa_Message object.
 * @param groupId     A pointer to a string representing the group ID.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__setGroupId(z_bmqa_Message* message_obj,
                               const char*     groupId);

/**
 * @brief Clears the group ID of a z_bmqa_Message object.
 * 
 * This function clears the group ID of the z_bmqa_Message object 'message_obj'.
 * 
 * @param message_obj A pointer to a z_bmqa_Message object.
 * 
 * @return Returns 0 upon successful operation.
 */
int z_bmqa_Message__clearGroupId(z_bmqa_Message* message_obj);

#endif

/**
 * @brief Creates a copy of a z_bmqa_Message object.
 * 
 * This function creates a copy of the z_bmqa_Message object 'message_obj' and stores
 * it in the memory pointed to by 'other_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object to be copied.
 * @param other_obj   A pointer to a pointer to a z_bmqa_Message object where
 *                    the copy will be stored.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_Message__clone(const z_bmqa_Message* message_obj,
                          z_bmqa_Message**      other_obj);

/**
 * @brief Retrieves the queue ID from a z_bmqa_Message object.
 * 
 * This function retrieves the queue ID from the z_bmqa_Message object 'message_obj'
 * and stores it in the memory pointed to by 'queueId_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * @param queueId_obj A pointer to a pointer to a const z_bmqa_QueueId object
 *                    where the queue ID will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_Message__queueId(const z_bmqa_Message*  message_obj,
                            const z_bmqa_QueueId** queueId_obj);

/**
 * @brief Retrieves the correlation ID from a z_bmqa_Message object.
 * 
 * This function retrieves the correlation ID from the z_bmqa_Message object 'message_obj'
 * and stores it in the memory pointed to by 'correlationId_obj'.
 * 
 * @param message_obj      A pointer to a const z_bmqa_Message object.
 * @param correlationId_obj A pointer to a pointer to a const z_bmqt_CorrelationId object
 *                          where the correlation ID will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_Message__correlationId(
    const z_bmqa_Message*        message_obj,
    const z_bmqt_CorrelationId** correlationId_obj);

/**
 * @brief Retrieves the subscription handle from a z_bmqa_Message object.
 * 
 * This function retrieves the subscription handle from the z_bmqa_Message object 'message_obj'
 * and stores it in the memory pointed to by 'subscription_obj'.
 * 
 * @param message_obj     A pointer to a const z_bmqa_Message object.
 * @param subscription_obj A pointer to a pointer to a const z_bmqt_SubscriptionHandle object
 *                         where the subscription handle will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_Message__subscriptionHandle(
    const z_bmqa_Message*             message_obj,
    const z_bmqt_SubscriptionHandle** subscription_obj);

/**
 * @brief Retrieves the compression algorithm type from a z_bmqa_Message object.
 * 
 * This function retrieves the compression algorithm type from the z_bmqa_Message object 'message_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * 
 * @return The compression algorithm type of the message.
 */
z_bmqt_CompressionAlgorithmType::Enum
z_bmqa_Message__compressionAlgorithmType(const z_bmqa_Message* message_obj);

#ifdef BMQ_ENABLE_MSG_GROUPID

/**
 * @brief Retrieves the group ID from a z_bmqa_Message object.
 * 
 * This function retrieves the group ID from the z_bmqa_Message object 'message_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * 
 * @return A pointer to a string representing the group ID.
 */
const char* z_bmqa_Message__groupId(const z_bmqa_Message* message_obj);

#endif

/**
 * @brief Retrieves the message GUID from a z_bmqa_Message object.
 * 
 * This function retrieves the message GUID from the z_bmqa_Message object 'message_obj'
 * and stores it in the memory pointed to by 'messageGUID_obj'.
 * 
 * @param message_obj     A pointer to a const z_bmqa_Message object.
 * @param messageGUID_obj A pointer to a pointer to a const z_bmqt_MessageGUID object
 *                        where the message GUID will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_Message__messageGUID(const z_bmqa_Message*      message_obj,
                                const z_bmqt_MessageGUID** messageGUID_obj);

/**
 * @brief Retrieves the confirmation cookie from a z_bmqa_Message object.
 * 
 * This function retrieves the confirmation cookie from the z_bmqa_Message object 'message_obj'
 * and stores it in the memory pointed to by 'cookie_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * @param cookie_obj  A pointer to a pointer to a z_bmqa_MessageConfirmationCookie object
 *                    where the confirmation cookie will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_Message__confirmationCookie(
    const z_bmqa_Message*              message_obj,
    z_bmqa_MessageConfirmationCookie** cookie_obj);

/**
 * @brief Retrieves the acknowledgment status from a z_bmqa_Message object.
 * 
 * This function retrieves the acknowledgment status from the z_bmqa_Message object 'message_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * 
 * @return The acknowledgment status of the message.
 */
int z_bmqa_Message__ackStatus(const z_bmqa_Message* message_obj);

/**
 * @brief Retrieves the data from a z_bmqa_Message object.
 * 
 * This function retrieves the data from the z_bmqa_Message object 'message_obj'
 * and stores it in the memory pointed to by 'buffer'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * @param buffer      A pointer to a pointer to a character buffer where the data
 *                    will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, char** buffer);

/**
 * @brief Retrieves the size of the data from a z_bmqa_Message object.
 * 
 * This function retrieves the size of the data from the z_bmqa_Message object 'message_obj'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * 
 * @return The size of the data.
 */
int z_bmqa_Message__dataSize(const z_bmqa_Message* message_obj);

/**
 * @brief Checks if a z_bmqa_Message object has properties.
 * 
 * This function checks if the z_bmqa_Message object 'message_obj' has properties.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * 
 * @return Returns true if the message has properties, false otherwise.
 */
bool z_bmqa_Message__hasProperties(const z_bmqa_Message* message_obj);

#ifdef BMQ_ENABLE_MSG_GROUPID

/**
 * @brief Checks if a z_bmqa_Message object has a group ID.
 * 
 * This function checks if the z_bmqa_Message object 'message_obj' has a group ID.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * 
 * @return Returns true if the message has a group ID, false otherwise.
 */
bool z_bmqa_Message__hasGroupId(const z_bmqa_Message* message_obj);

#endif

/**
 * @brief Loads the properties from a z_bmqa_Message object into a buffer.
 * 
 * This function loads the properties from the z_bmqa_Message object 'message_obj'
 * into the buffer pointed to by 'buffer'.
 * 
 * @param message_obj A pointer to a const z_bmqa_Message object.
 * @param buffer      A pointer to a z_bmqa_MessageProperties object where
 *                    the properties will be loaded.
 * 
 * @return Returns 0 upon successful loading of properties.
 */
int z_bmqa_Message__loadProperties(const z_bmqa_Message*     message_obj,
                                   z_bmqa_MessageProperties* buffer);

#if defined(__cplusplus)
}
#endif

#endif
