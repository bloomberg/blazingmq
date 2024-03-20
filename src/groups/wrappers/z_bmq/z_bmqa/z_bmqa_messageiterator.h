#ifndef INCLUDED_Z_BMQA_MESSAGEITERATOR
#define INCLUDED_Z_BMQA_MESSAGEITERATOR

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_message.h>

typedef struct z_bmqa_MessageIterator z_bmqa_MessageIterator;

/**
 * @brief Creates a new message iterator object.
 * 
 * This function creates a new message iterator object.
 * 
 * @param messageIterator_obj A pointer to a pointer to the z_bmqa_MessageIterator object to be created.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_MessageIterator__create(
    z_bmqa_MessageIterator** messageIterator_obj);

/**
 * @brief Deletes a message iterator object.
 * 
 * This function deallocates memory for the message iterator object.
 * 
 * @param messageIterator_obj A pointer to a pointer to the z_bmqa_MessageIterator object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageIterator__delete(
    z_bmqa_MessageIterator** messageIterator_obj);

/**
 * @brief Advances to the next message in the iterator.
 * 
 * This function advances to the next message in the iterator.
 * 
 * @param messageIterator_obj A pointer to the z_bmqa_MessageIterator object.
 * 
 * @return Returns true if there is another message in the iterator, otherwise returns false.
 */
bool z_bmqa_MessageIterator__nextMessage(
    z_bmqa_MessageIterator* messageIterator_obj);

/**
 * @brief Retrieves the current message from the iterator.
 * 
 * This function retrieves the current message from the iterator.
 * 
 * @param messageIterator_obj A pointer to the const z_bmqa_MessageIterator object.
 * @param message_obj         A pointer to a pointer to a const z_bmqa_Message object where the retrieved
 *                            message will be stored.
 * 
 * @return Returns 0 upon successful retrieval of the message.
 */
int z_bmqa_MessageIterator__message(
    const z_bmqa_MessageIterator* messageIterator_obj,
    const z_bmqa_Message**        message_obj);

#if defined(__cplusplus)
}
#endif

#endif
