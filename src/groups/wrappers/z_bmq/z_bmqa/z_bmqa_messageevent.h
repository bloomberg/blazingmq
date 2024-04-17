#ifndef INCLUDED_Z_BMQA_MESSAGEEVENT
#define INCLUDED_Z_BMQA_MESSAGEEVENT

#if defined(__cplusplus)
extern "C" {
#endif

#include <z_bmqa_messageiterator.h>
#include <z_bmqt_messageeventtype.h>

typedef struct z_bmqa_MessageEvent z_bmqa_MessageEvent;

/**
 * @brief Deletes a z_bmqa_MessageEvent object.
 * 
 * This function deallocates memory for the z_bmqa_MessageEvent object.
 * 
 * @param messageEvent A pointer to a pointer to the z_bmqa_MessageEvent object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageEvent__delete(z_bmqa_MessageEvent** messageEvent);

/**
 * @brief Creates a new z_bmqa_MessageEvent object.
 * 
 * This function creates a new z_bmqa_MessageEvent object.
 * 
 * @param messageEvent A pointer to a pointer to the z_bmqa_MessageEvent object to be created.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_MessageEvent__create(z_bmqa_MessageEvent** messageEvent);

/**
 * @brief Retrieves the message iterator associated with a z_bmqa_MessageEvent object.
 * 
 * This function retrieves the message iterator associated with the z_bmqa_MessageEvent object
 * and stores it in the memory pointed to by 'iterator_obj'.
 * 
 * @param messageEvent    A pointer to a const z_bmqa_MessageEvent object.
 * @param iterator_obj A pointer to a pointer to a z_bmqa_MessageIterator object
 *                     where the message iterator will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_MessageEvent__messageIterator(
    const z_bmqa_MessageEvent* messageEvent,
    z_bmqa_MessageIterator**   iterator_obj);

/**
 * @brief Retrieves the message event type of a z_bmqa_MessageEvent object.
 * 
 * This function retrieves the message event type of the z_bmqa_MessageEvent object.
 * 
 * @param messageEvent A pointer to a const z_bmqa_MessageEvent object.
 * 
 * @return The message event type of the message event.
 */
z_bmqt_MessageEventType::Enum
z_bmqa_MessageEvent__type(const z_bmqa_MessageEvent* messageEvent);

/**
 * @brief Converts a z_bmqa_MessageEvent object to a string.
 * 
 * This function converts the z_bmqa_MessageEvent object 'messageEvent' to a string
 * and stores the string in the memory pointed to by 'out'.
 * 
 * @param messageEvent A pointer to a const z_bmqa_MessageEvent object.
 * @param out       A pointer to a pointer to a character buffer where the string
 *                  representation of the message event will be stored.
 * 
 * @return Returns 0 upon successful conversion.
 */
int z_bmqa_MessageEvent__toString(const z_bmqa_MessageEvent* messageEvent,
                                  char**                     out);

#if defined(__cplusplus)
}
#endif

#endif
