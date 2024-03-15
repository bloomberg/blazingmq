#ifndef INCLUDED_Z_BMQA_MESSAGEITERATOR
#define INCLUDED_Z_BMQA_MESSAGEITERATOR

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_message.h>

typedef struct z_bmqa_MessageIterator z_bmqa_MessageIterator;

/**
 * @brief Creates a new z_bmqa_MessageIterator object.
 *
 * This function dynamically allocates memory for a new z_bmqa_MessageIterator object
 * and assigns its pointer to the provided pointer-to-pointer variable.
 * 
 * @param messageIterator_obj A pointer to a pointer to a z_bmqa_MessageIterator object.
 *                            Upon successful completion, this pointer will point to the newly created object.
 * 
 * @return Returns 0 upon successful creation.
 */

int z_bmqa_MessageIterator__create(
    z_bmqa_MessageIterator** messageIterator_obj);

/**
 * @brief Deletes the memory allocated for a pointer to a bmqa::MessageIterator object.
 *
 * This function deallocates the memory pointed to by the input pointer to a bmqa::MessageIterator object
 * and sets the pointer to NULL.
 * 
 * @param messageIterator_obj A pointer to a pointer to a bmqa::MessageIterator object.
 *                            Upon successful completion, this pointer will be set to NULL.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageIterator__delete(
    z_bmqa_MessageIterator** messageIterator_obj);

/**
 * @brief Moves the iterator to the next message in the sequence.
 *
 * This function advances the iterator to the next message in the sequence
 * and returns true if successful.
 * 
 * @param messageIterator_obj A pointer to a z_bmqa_MessageIterator object.
 * 
 * @returns True if the iterator successfully moves to the next message, false otherwise.
 */
bool z_bmqa_MessageIterator__nextMessage(
    z_bmqa_MessageIterator* messageIterator_obj);

/**
 * @brief Retrieves the message pointed to by a MessageIterator object.
 *
 * This function retrieves the message pointed to by the provided MessageIterator object
 * and assigns it to the provided output pointer. 
 * 
 * @param messageIterator_obj A pointer to a z_bmqa_MessageIterator object.
 * @param message_obj A pointer to a pointer to a z_bmqa_Message object.
 *                    Upon successful completion, this pointer will point to the retrieved message.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_MessageIterator__message(
    const z_bmqa_MessageIterator* messageIterator_obj,
    const z_bmqa_Message**        message_obj);

#if defined(__cplusplus)
}
#endif

#endif