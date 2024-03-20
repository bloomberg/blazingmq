#ifndef INCLUDED_Z_BMQA_SESSIONEVENT
#define INCLUDED_Z_BMQA_SESSIONEVENT

#if defined(__cplusplus)
extern "C" {
#endif

#include <z_bmqa_queueid.h>
#include <z_bmqt_correlationid.h>
#include <z_bmqt_sessioneventtype.h>

/**
 * @brief Represents a session event.
 */
typedef struct z_bmqa_SessionEvent z_bmqa_SessionEvent;

/**
 * @brief Deletes a session event object.
 * 
 * @param event_obj A pointer to a pointer to the session event object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_SessionEvent__delete(z_bmqa_SessionEvent** event_obj);

/**
 * @brief Creates a new session event object.
 * 
 * @param event_obj A pointer to a pointer to store the created session event object.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_SessionEvent__create(z_bmqa_SessionEvent** event_obj);

/**
 * @brief Creates a copy of a session event object.
 * 
 * @param event_obj A pointer to a pointer to store the copied session event object.
 * @param other     A pointer to the session event object to be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_SessionEvent__createCopy(z_bmqa_SessionEvent**      event_obj,
                                    const z_bmqa_SessionEvent* other);

/**
 * @brief Retrieves the type of the session event.
 * 
 * @param event_obj A pointer to the session event object.
 * 
 * @return Returns the type of the session event.
 */
z_bmqt_SessionEventType::Enum
z_bmqa_SessionEvent__type(const z_bmqa_SessionEvent* event_obj);

/**
 * @brief Retrieves the correlation ID associated with the session event.
 * 
 * @param event_obj          A pointer to the session event object.
 * @param correlationId_obj A pointer to a pointer to store the retrieved correlation ID.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_SessionEvent__correlationId(
    const z_bmqa_SessionEvent*   event_obj,
    z_bmqt_CorrelationId const** correlationId_obj);

/**
 * @brief Retrieves the queue ID associated with the session event.
 * 
 * @param event_obj   A pointer to the session event object.
 * @param queueId_obj A pointer to a pointer to store the retrieved queue ID.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_SessionEvent__queueId(const z_bmqa_SessionEvent* event_obj,
                                 z_bmqa_QueueId**           queueId_obj);

/**
 * @brief Retrieves the status code associated with the session event.
 * 
 * @param event_obj A pointer to the session event object.
 * 
 * @return Returns the status code of the session event.
 */
int z_bmqa_SessionEvent__statusCode(const z_bmqa_SessionEvent* event_obj);

/**
 * @brief Retrieves the error description associated with the session event.
 * 
 * @param event_obj A pointer to the session event object.
 * 
 * @return Returns a pointer to the error description string.
 */
const char*
z_bmqa_SessionEvent__errorDescription(const z_bmqa_SessionEvent* event_obj);

/**
 * @brief Converts the session event object to a string representation.
 * 
 * @param event_obj A pointer to the session event object.
 * @param out       A pointer to a pointer to store the string representation.
 * 
 * @return Returns 0 upon successful conversion.
 */
int z_bmqa_SessionEvent__toString(const z_bmqa_SessionEvent* event_obj,
                                  char**                     out);

#if defined(__cplusplus)
}
#endif

#endif
`