#ifndef INCLUDED_Z_BMQA_MESSAGEEVENTBUILDER
#define INCLUDED_Z_BMQA_MESSAGEEVENTBUILDER

#if defined(__cplusplus)
extern "C" {
#endif

#include <z_bmqa_message.h>
#include <z_bmqa_messageevent.h>
#include <z_bmqa_queueid.h>

typedef struct z_bmqa_MessageEventBuilder z_bmqa_MessageEventBuilder;

/**
 * @brief Deletes a z_bmqa_MessageEventBuilder object.
 * 
 * This function deallocates memory for the z_bmqa_MessageEventBuilder object.
 * 
 * @param messageEventBuilder A pointer to a pointer to the z_bmqa_MessageEventBuilder object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageEventBuilder__delete(z_bmqa_MessageEventBuilder** messageEventBuilder);

/**
 * @brief Creates a new z_bmqa_MessageEventBuilder object.
 * 
 * This function creates a new z_bmqa_MessageEventBuilder object.
 * 
 * @param messageEventBuilder A pointer to a pointer to the z_bmqa_MessageEventBuilder object to be created.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_MessageEventBuilder__create(z_bmqa_MessageEventBuilder** messageEventBuilder);

/**
 * @brief Starts a new message in the message event builder.
 * 
 * This function starts a new message in the message event builder and returns a pointer to the
 * created z_bmqa_Message object.
 * 
 * @param messageEventBuilder A pointer to the z_bmqa_MessageEventBuilder object.
 * @param messageOut     A pointer to a pointer to a z_bmqa_Message object where the created message
 *                    will be stored.
 * 
 * @return Returns 0 upon successful creation of the message.
 */
int z_bmqa_MessageEventBuilder__startMessage(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    z_bmqa_Message**            messageOut);

/**
 * @brief Packs the current message in the message event builder.
 * 
 * This function packs the current message in the message event builder with the specified queue ID.
 * 
 * @param messageEventBuilder A pointer to the z_bmqa_MessageEventBuilder object.
 * @param queueId     A pointer to the z_bmqa_QueueId object representing the queue ID to pack
 *                    the message with.
 * 
 * @return Returns 0 upon successful packing of the message.
 */
int z_bmqa_MessageEventBuilder__packMessage(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    const z_bmqa_QueueId*       queueId);

/**
 * @brief Resets the message event builder.
 * 
 * This function resets the message event builder, clearing all previously added messages.
 * 
 * @param messageEventBuilder A pointer to the z_bmqa_MessageEventBuilder object.
 * 
 * @return Returns 0 upon successful reset.
 */
int z_bmqa_MessageEventBuilder__reset(z_bmqa_MessageEventBuilder* messageEventBuilder);

/**
 * @brief Retrieves the message event from the message event builder.
 * 
 * This function retrieves the message event from the message event builder.
 * 
 * @param messageEventBuilder A pointer to the z_bmqa_MessageEventBuilder object.
 * @param messageEvent   A pointer to a pointer to a z_bmqa_MessageEvent object where the retrieved
 *                    message event will be stored.
 * 
 * @return Returns 0 upon successful retrieval of the message event.
 */
int z_bmqa_MessageEventBuilder__messageEvent(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    const z_bmqa_MessageEvent** messageEvent);

/**
 * @brief Retrieves the current message being built in the message event builder.
 * 
 * This function retrieves the current message being built in the message event builder.
 * 
 * @param messageEventBuilder A pointer to the z_bmqa_MessageEventBuilder object.
 * @param messageEvent A pointer to a pointer to a z_bmqa_Message object where the retrieved
 *                    message will be stored.
 * 
 * @return Returns 0 upon successful retrieval of the current message.
 */
int z_bmqa_MessageEventBuilder__currentMessage(
    z_bmqa_MessageEventBuilder* messageEventBuilder,
    z_bmqa_Message**            message_obj);

/**
 * @brief Retrieves the number of messages in the message event builder.
 * 
 * This function retrieves the number of messages in the message event builder.
 * 
 * @param messageEventBuilder A pointer to the const z_bmqa_MessageEventBuilder object.
 * 
 * @return The number of messages in the message event builder.
 */
int z_bmqa_MessageEventBuilder__messageCount(
    const z_bmqa_MessageEventBuilder* messageEventBuilder);

/**
 * @brief Retrieves the size of the message event in bytes.
 * 
 * This function retrieves the size of the message event in bytes.
 * 
 * @param messageEventBuilder A pointer to the const z_bmqa_MessageEventBuilder object.
 * 
 * @return The size of the message event in bytes.
 */
int z_bmqa_MessageEventBuilder__messageEventSize(
    const z_bmqa_MessageEventBuilder* messageEventBuilder);

#if defined(__cplusplus)
}
#endif

#endif
