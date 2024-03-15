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
 * @brief Deletes the memory allocated for a pointer to a bmqa::Message object.
 *
 * This function deallocates the memory pointed to by the input pointer to a bmqa::Message object and sets the pointer to NULL.
 * It ensures that the memory is properly released and the pointer is reset, preventing memory leaks and dangling pointer issues.
 * 
 * @param message_obj A pointer to a pointer to a bmqa::Message object.
 *                    Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */

int z_bmqa_Message__delete(z_bmqa_Message** message_obj);


/**
 * @brief Allocates memory and creates a new bmqa::Message object, setting the input pointer to point to it.
 *
 * This function dynamically allocates memory for a new bmqa::Message object and initializes it. The input pointer is then updated to point to this new object. This allows for the creation of bmqa::Message objects without direct handling of memory allocation by the caller.
 * 
 * @param message_obj A pointer to a pointer to a z_bmqa_Message object. Upon successful completion, this pointer will be updated to point to the newly created bmqa::Message object.
 * @return Returns 0 upon successful creation of the object.
 */
int z_bmqa_Message__create(z_bmqa_Message** message_obj);


/**
 * @brief Sets the data reference for a bmqa::Message object with given data and length.
 *
 * This function assigns a data reference to an existing bmqa::Message object. The data is not copied; instead, the message object is pointed to the provided data, making the operation efficient. It is crucial that the data remains valid for the lifetime of the message object or until the data reference is reset or changed.
 * 
 * @param message_obj A pointer to an existing z_bmqa_Message object to which the data reference will be set.
 * @param data A constant character pointer to the data to be referenced by the message object.
 * @param length The length of the data in bytes.
 * @return Returns 0 upon successful update of the data reference.
 */
int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj,
                               const char*     data,
                               int             length);


/**
 * @brief Sets the properties of a bmqa::Message object using a reference to a z_bmqa_MessageProperties object.
 *
 * This function updates the properties of a specified bmqa::Message object by taking a reference to a z_bmqa_MessageProperties object. It allows for the modification of message properties without directly accessing or modifying the internal structure of the message object, promoting encapsulation and data integrity.
 * 
 * @param message_obj A pointer to the z_bmqa_Message object whose properties are to be set.
 * @param properties_obj A constant pointer to a z_bmqa_MessageProperties object containing the new properties to be applied to the message.
 * @return Returns 0 upon successful update of the message properties.
 */
int z_bmqa_Message__setPropertiesRef(
    z_bmqa_Message*                 message_obj,
    const z_bmqa_MessageProperties* properties_obj);


/**
 * @brief Clears the properties of a bmqa::Message object.
 *
 * This function resets the properties of the specified bmqa::Message object to their default state. It is useful for reusing a message object without carrying over the properties from a previous usage, ensuring that the message is in a clean state before setting new properties or data.
 * 
 * @param message_obj A pointer to the z_bmqa_Message object whose properties are to be cleared.
 * @return Returns 0 upon successful clearing of the message properties.
 */

int z_bmqa_Message__clearPropertiesRef(z_bmqa_Message* message_obj);

/**
 * @brief Sets the correlation ID for a bmqa::Message object using a reference to a z_bmqt_CorrelationId.
 *
 * This function assigns a specific correlation ID to the given bmqa::Message object, facilitating the tracking and matching of messages within a system. By using a correlation ID, applications can easily correlate responses with their corresponding requests, improving the traceability and management of message flows.
 * 
 * @param message_obj A pointer to the z_bmqa_Message object to which the correlation ID is to be set.
 * @param correlationId A constant pointer to a z_bmqt_CorrelationId object that contains the correlation ID to be assigned to the message.
 * @return Returns 0 upon successful assignment of the correlation ID.
 */

int z_bmqa_Message__setCorrelationId(
    z_bmqa_Message*             message_obj,
    const z_bmqt_CorrelationId* correlationId);

/**
 * @brief Sets the compression algorithm type for a bmqa::Message object.
 *
 * This function configures the compression algorithm used by a bmqa::Message object, allowing for efficient data transmission by selecting an appropriate compression type. By specifying the compression algorithm, users can optimize the message size and performance for their specific networking or storage requirements.
 * 
 * @param message_obj A pointer to the z_bmqa_Message object for which the compression algorithm is to be set.
 * @param value The compression algorithm type, represented as a value from the z_bmqt_CompressionAlgorithmType::Enum enumeration, to be applied to the message.
 * @return Returns 0 upon successful setting of the compression algorithm type.
 */

int z_bmqa_Message__setCompressionAlgorithmType(
    z_bmqa_Message*                       message_obj,
    z_bmqt_CompressionAlgorithmType::Enum value);

#ifdef BMQ_ENABLE_MSG_GROUPID

/**
 * @brief Assigns a group ID to a bmqa::Message object.
 *
 * This function sets a group ID for the specified bmqa::Message object, allowing messages to be categorized or grouped according to the provided identifier. The use of group IDs facilitates message filtering and processing based on group membership, enhancing the organization and handling of messages within a system.
 * 
 * @param message_obj A pointer to the z_bmqa_Message object to which the group ID is to be assigned.
 * @param groupId A string representing the group ID to be set for the message. This identifier can be used to group messages for processing or categorization purposes.
 * @return Returns 0 upon successful assignment of the group ID.
 */


int z_bmqa_Message__setGroupId(z_bmqa_Message* message_obj,
                               const char*     groupId);

/**
 * @brief Clears the group ID of a bmqa::Message object.
 *
 * This function removes any group ID previously assigned to the specified bmqa::Message object, effectively resetting its group categorization to default. It is useful for repurposing a message object for a different group or removing its association with any group, ensuring the message is in a neutral state for new operations.
 * 
 * @param message_obj A pointer to the z_bmqa_Message object whose group ID is to be cleared.
 * @return Returns 0 upon successful clearing of the group ID.
 */

int z_bmqa_Message__clearGroupId(z_bmqa_Message* message_obj);

#endif

/**
 * @brief Clones a bmqa::Message object, creating a new instance with the same content.
 *
 * This function creates a new bmqa::Message object that is a clone of the specified message object. The new object contains the same data and properties as the original, allowing for independent manipulation or transmission of the cloned message. It is useful in scenarios where a copy of a message needs to be sent or processed separately from the original.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object to be cloned.
 * @param other_obj A pointer to a pointer to a z_bmqa_Message object. Upon successful completion, this pointer will be updated to point to the newly cloned message object.
 * @return Returns 0 upon successful cloning of the message object.
 */

int z_bmqa_Message__clone(const z_bmqa_Message* message_obj,
                          z_bmqa_Message**      other_obj);

/**
 * @brief Retrieves the queue ID associated with a bmqa::Message object.
 *
 * This function provides access to the queue ID of the specified bmqa::Message object. The queue ID is an identifier that can be used to determine the message's intended destination or source within a messaging system. By obtaining the queue ID, applications can make routing decisions or perform analytics based on the message flow.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the queue ID is to be retrieved.
 * @param queueId_obj A pointer to a pointer to a z_bmqa_QueueId. Upon successful completion, this pointer will be updated to point to the queue ID associated with the message.
 * @return Returns 0 upon successful retrieval of the queue ID.
 */

int z_bmqa_Message__queueId(const z_bmqa_Message*  message_obj,
                            const z_bmqa_QueueId** queueId_obj);

/**
 * @brief Retrieves the correlation ID associated with a bmqa::Message object.
 *
 * This function accesses the correlation ID of the specified bmqa::Message object, providing a mechanism to track and associate messages within a system. Correlation IDs are crucial for linking requests with their responses or for grouping related messages together. This feature enhances the ability to monitor and analyze message flows, improving system integration and communication tracking.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the correlation ID is to be retrieved.
 * @param correlationId_obj A pointer to a pointer to a z_bmqt_CorrelationId. Upon successful completion, this pointer will be updated to point to the correlation ID associated with the message.
 * @return Returns 0 upon successful retrieval of the correlation ID.
 */

int z_bmqa_Message__correlationId(
    const z_bmqa_Message*        message_obj,
    const z_bmqt_CorrelationId** correlationId_obj);

/**
 * @brief Retrieves the subscription handle associated with a bmqa::Message object.
 *
 * This function provides access to the subscription handle of the specified bmqa::Message object, facilitating the management and identification of subscriptions in messaging systems. Subscription handles are used to uniquely identify a subscription, allowing for operations such as modifying, querying, or canceling subscriptions based on the handle. This feature is particularly useful in systems that support complex subscription models and need to efficiently manage multiple message streams.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the subscription handle is to be retrieved.
 * @param subscription_obj A pointer to a pointer to a z_bmqt_SubscriptionHandle. Upon successful completion, this pointer will be updated to point to the subscription handle associated with the message.
 * @return Returns 0 upon successful retrieval of the subscription handle.
 */

int z_bmqa_Message__subscriptionHandle(
    const z_bmqa_Message*             message_obj,
    const z_bmqt_SubscriptionHandle** subscription_obj);

/**
 * @brief Retrieves the compression algorithm type set for a bmqa::Message object.
 *
 * This function returns the compression algorithm type that has been configured for the specified bmqa::Message object. It enables the caller to understand how the message data is compressed, facilitating appropriate decompression or processing steps on the receiver's side. This information is crucial for applications that need to handle message payloads efficiently, especially when working with large data sets or in bandwidth-constrained environments.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the compression algorithm type is to be retrieved.
 * @return Returns the compression algorithm type as a value of the z_bmqt_CompressionAlgorithmType::Enum enumeration, indicating how the message data is compressed.
 */

z_bmqt_CompressionAlgorithmType::Enum
z_bmqa_Message__compressionAlgorithmType(const z_bmqa_Message* message_obj);

#ifdef BMQ_ENABLE_MSG_GROUPID

/**
 * @brief Retrieves the group ID associated with a bmqa::Message object.
 *
 * This function returns the group ID of the specified bmqa::Message object as a string. The group ID is an identifier used to categorize or group messages for processing or routing purposes. By accessing the group ID, applications can perform actions based on the group to which a message belongs, such as filtering messages, performing group-specific processing, or routing messages to appropriate destinations based on their group ID.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the group ID is to be retrieved.
 * @return Returns a pointer to a constant character string representing the group ID of the message. This string remains valid as long as the message object is not destroyed or modified.
 */

const char* z_bmqa_Message__groupId(const z_bmqa_Message* message_obj);

#endif

/**
 * @brief Retrieves the message GUID (Globally Unique Identifier) associated with a bmqa::Message object.
 *
 * This function provides access to the message GUID of the specified bmqa::Message object. A message GUID is a unique identifier for a message, ensuring that each message can be distinctly identified across different components and systems. This feature is particularly useful in tracking, logging, and auditing message flows, as it allows for precise identification and correlation of messages.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the message GUID is to be retrieved.
 * @param messageGUID_obj A pointer to a pointer to a z_bmqt_MessageGUID. Upon successful completion, this pointer will be updated to point to the message GUID associated with the message.
 * @return Returns 0 upon successful retrieval of the message GUID.
 */

int z_bmqa_Message__messageGUID(const z_bmqa_Message*      message_obj,
                                const z_bmqt_MessageGUID** messageGUID_obj);

/**
 * @brief Retrieves and duplicates the confirmation cookie associated with a bmqa::Message object.
 *
 * This function allocates a new confirmation cookie, copying the contents from the confirmation cookie associated with the specified bmqa::Message object. A confirmation cookie is typically used to acknowledge the receipt or processing of a message, providing a mechanism for tracking message status and ensuring reliable communication. By duplicating the confirmation cookie, this function allows for the safe transmission or storage of the cookie independently of the original message.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the confirmation cookie is to be duplicated.
 * @param cookie_obj A pointer to a pointer to a z_bmqa_MessageConfirmationCookie. Upon successful completion, this pointer will be updated to point to the newly allocated confirmation cookie.
 * @return Returns 0 upon successful duplication of the confirmation cookie.
 */

int z_bmqa_Message__confirmationCookie(
    const z_bmqa_Message*              message_obj,
    z_bmqa_MessageConfirmationCookie** cookie_obj);

/**
 * @brief Retrieves the acknowledgment status of a bmqa::Message object.
 *
 * This function returns the acknowledgment status for the specified bmqa::Message object. The acknowledgment status indicates whether the message has been acknowledged by the receiver, which can be crucial for ensuring message delivery and handling in systems that require confirmation of message processing. This information helps in managing the flow of messages, particularly in reliable messaging systems where it's important to track the delivery status of each message.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the acknowledgment status is to be retrieved.
 * @return Returns an integer representing the acknowledgment status of the message. The specific meanings of different values are defined by the implementation and should be documented accordingly.
 */

int z_bmqa_Message__ackStatus(const z_bmqa_Message* message_obj);


/**
 * @brief Retrieves the data contained in a bmqa::Message object, formatted as a hexadecimal string.
 *
 * This function extracts the data from the specified bmqa::Message object and converts it into a hexadecimal string representation. This is useful for debugging or logging purposes, where viewing the raw message data in a human-readable format can be invaluable. The function allocates memory for the hexadecimal string and sets the provided buffer pointer to point to this memory. The caller is responsible for freeing this memory after use.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the data is to be retrieved.
 * @param buffer A pointer to a character pointer. Upon successful completion, this pointer will be updated to point to a newly allocated string containing the hexadecimal representation of the message data. The caller is responsible for freeing this memory.
 * @return Returns an integer indicating the result of the data retrieval operation. A return value of 0 typically indicates success, while other values may indicate errors.
 */

// Add once we figure out how to handle Blobs in C
// int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, z_bdlbb_Blob*
// blob);

int z_bmqa_Message__getData(const z_bmqa_Message* message_obj, char** buffer);

/**
 * @brief Retrieves the size of the data contained within a bmqa::Message object.
 *
 * This function returns the size, in bytes, of the data payload within the specified bmqa::Message object. It allows for the assessment of the message's data payload size, which can be crucial for understanding the message's footprint, especially in contexts where bandwidth or storage efficiency is a concern.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which the data size is to be determined.
 * @return Returns an integer representing the size of the data contained within the message, measured in bytes.
 */

int z_bmqa_Message__dataSize(const z_bmqa_Message* message_obj);

/**
 * @brief Checks if a bmqa::Message object contains properties.
 *
 * This function determines whether the specified bmqa::Message object has properties set. Properties in a message can include metadata such as headers, routing information, or any other key-value pairs that provide context or control how the message should be handled. Knowing whether a message includes such properties is useful for conditional processing or routing decisions.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object to be checked for properties.
 * @return Returns true if the message object contains properties; otherwise, returns false.
 */

bool z_bmqa_Message__hasProperties(const z_bmqa_Message* message_obj);

#ifdef BMQ_ENABLE_MSG_GROUPID

/**
 * @brief Checks if a bmqa::Message object has a group ID assigned.
 *
 * This function determines whether the specified bmqa::Message object has a group ID assigned to it. A group ID is used to categorize or group messages for various purposes, such as routing, filtering, or processing. Identifying whether a message is part of a specific group can be crucial for systems that implement group-based messaging logic.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object to be checked for a group ID.
 * @return Returns true if the message object has a group ID assigned; otherwise, returns false.
 */

bool z_bmqa_Message__hasGroupId(const z_bmqa_Message* message_obj);

#endif

/**
 * @brief Loads the properties of a bmqa::Message object into a provided buffer.
 *
 * This function extracts the properties from the specified bmqa::Message object and loads them into a user-provided buffer. The properties might include metadata like headers, routing information, or other key-value pairs that provide additional context or instructions on how the message should be handled. This mechanism allows for the direct manipulation or inspection of message properties outside of the message object itself.
 * 
 * @param message_obj A constant pointer to the z_bmqa_Message object from which properties are to be loaded.
 * @param buffer A pointer to a z_bmqa_MessageProperties structure where the message properties will be stored. The caller must ensure that this buffer is appropriately allocated before calling this function.
 * @return Returns an integer indicating the result of the properties loading operation. A return value of 0 typically indicates success, while other values may indicate errors.
 */

int z_bmqa_Message__loadProperties(const z_bmqa_Message*     message_obj,
                                   z_bmqa_MessageProperties* buffer);

#if defined(__cplusplus)
}
#endif

#endif