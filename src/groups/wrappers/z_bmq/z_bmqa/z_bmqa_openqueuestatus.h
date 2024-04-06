#ifndef INCLUDED_Z_BMQA_OPENQUEUESTATUS
#define INCLUDED_Z_BMQA_OPENQUEUESTATUS

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_queueid.h>

typedef struct z_bmqa_OpenQueueStatus z_bmqa_OpenQueueStatus;

/**
 * @brief Deletes an open queue status object.
 * 
 * This function deallocates memory for the open queue status object.
 * 
 * @param openQueueStatus A pointer to a pointer to the z_bmqa_OpenQueueStatus object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_OpenQueueStatus__delete(z_bmqa_OpenQueueStatus** openQueueStatus);

/**
 * @brief Creates a new open queue status object.
 * 
 * This function creates a new open queue status object.
 * 
 * @param openQueueStatus A pointer to a pointer to the z_bmqa_OpenQueueStatus object to be created.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_OpenQueueStatus__create(z_bmqa_OpenQueueStatus** openQueueStatus);

/**
 * @brief Creates a copy of an open queue status object.
 * 
 * This function creates a copy of an existing open queue status object.
 * 
 * @param openQueueStatus A pointer to a pointer to the z_bmqa_OpenQueueStatus object where the copy will be stored.
 * @param other      A pointer to the const z_bmqa_OpenQueueStatus object to be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_OpenQueueStatus__createCopy(z_bmqa_OpenQueueStatus** openQueueStatus,
                                       const z_bmqa_OpenQueueStatus* other);

/**
 * @brief Creates an open queue status object with full information.
 * 
 * This function creates an open queue status object with full information.
 * 
 * @param openQueueStatus         A pointer to a pointer to the z_bmqa_OpenQueueStatus object to be created.
 * @param queueId            A pointer to the const z_bmqa_QueueId object representing the queue ID.
 * @param result             The result of the operation.
 * @param errorDescription   The error description.
 * 
 * @return Returns 0 upon successful creation of the object.
 */
int z_bmqa_OpenQueueStatus__createFull(z_bmqa_OpenQueueStatus** openQueueStatus,
                                       const z_bmqa_QueueId*    queueId,
                                       int                      result,
                                       const char*              errorDescription);

/**
 * @brief Converts an open queue status object to a boolean value.
 * 
 * This function converts an open queue status object to a boolean value.
 * 
 * @param openQueueStatus A pointer to the const z_bmqa_OpenQueueStatus object.
 * 
 * @return Returns true if the status object is valid, false otherwise.
 */
bool z_bmqa_OpenQueueStatus__toBool(const z_bmqa_OpenQueueStatus* openQueueStatus);

/**
 * @brief Retrieves the queue ID from an open queue status object.
 * 
 * This function retrieves the queue ID from an open queue status object.
 * 
 * @param openQueueStatus   A pointer to the const z_bmqa_OpenQueueStatus object.
 * @param queueId  A pointer to a pointer to the const z_bmqa_QueueId object to store the queue ID.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_OpenQueueStatus__queueId(const z_bmqa_OpenQueueStatus* openQueueStatus,
                                    const z_bmqa_QueueId**        queueId);

/**
 * @brief Retrieves the result from an open queue status object.
 * 
 * This function retrieves the result from an open queue status object.
 * 
 * @param openQueueStatus A pointer to the const z_bmqa_OpenQueueStatus object.
 * 
 * @return Returns the result of the operation.
 */
int z_bmqa_OpenQueueStatus__result(const z_bmqa_OpenQueueStatus* openQueueStatus);

/**
 * @brief Retrieves the error description from an open queue status object.
 * 
 * This function retrieves the error description from an open queue status object.
 * 
 * @param openQueueStatus A pointer to the const z_bmqa_OpenQueueStatus object.
 * 
 * @return Returns a pointer to the error description string.
 */
const char* z_bmqa_OpenQueueStatus__errorDescription(
    const z_bmqa_OpenQueueStatus* openQueueStatus);

#if defined(__cplusplus)
}
#endif

#endif
