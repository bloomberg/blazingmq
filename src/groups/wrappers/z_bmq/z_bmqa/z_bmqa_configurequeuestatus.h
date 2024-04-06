#ifndef INCLUDED_Z_BMQA_CONFIGUREQUEUESTATUS
#define INCLUDED_Z_BMQA_CONFIGUREQUEUESTATUS

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_queueid.h>

typedef struct z_bmqa_ConfigureQueueStatus z_bmqa_ConfigureQueueStatus;

/**
 * @brief Deletes a z_bmqa_ConfigureQueueStatus object.
 * 
 * This function deletes the z_bmqa_ConfigureQueueStatus object pointed to by 'configureQueueStatus'.
 * Upon successful completion, the memory pointed to by 'configureQueueStatus' will be deallocated,
 * and 'configureQueueStatus' will be set to NULL.
 * 
 * @param configureQueueStatus A pointer to a pointer to a z_bmqa_ConfigureQueueStatus object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_ConfigureQueueStatus__delete(
    z_bmqa_ConfigureQueueStatus** configureQueueStatus);

/**
 * @brief Creates a new z_bmqa_ConfigureQueueStatus object.
 * 
 * This function creates a new z_bmqa_ConfigureQueueStatus object and stores
 * a pointer to it in the memory pointed to by 'configureQueueStatus'.
 * 
 * @param configureQueueStatus A pointer to a pointer to a z_bmqa_ConfigureQueueStatus object where
 *                   the newly created object will be stored.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_ConfigureQueueStatus__create(
    z_bmqa_ConfigureQueueStatus** configureQueueStatus);

/**
 * @brief Creates a copy of a z_bmqa_ConfigureQueueStatus object.
 * 
 * This function creates a deep copy of the input z_bmqa_ConfigureQueueStatus object 'other'
 * and stores it in the memory pointed to by 'configureQueueStatus'. Upon successful completion,
 * the pointer 'configureQueueStatus' will point to the newly created copy.
 * 
 * @param configureQueueStatus A pointer to a pointer to a z_bmqa_ConfigureQueueStatus object where
 *                   the copy will be stored. Upon successful completion, this pointer
 *                   will point to the newly created copy.
 * @param other      A pointer to a z_bmqa_ConfigureQueueStatus object which will be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_ConfigureQueueStatus__createCopy(
    z_bmqa_ConfigureQueueStatus**      configureQueueStatus,
    const z_bmqa_ConfigureQueueStatus* other);

/**
 * @brief Creates a z_bmqa_ConfigureQueueStatus object with provided parameters.
 * 
 * This function creates a new z_bmqa_ConfigureQueueStatus object with the provided
 * 'queueId', 'result', and 'errorDescription', and stores a pointer to it in the memory
 * pointed to by 'configureQueueStatus'.
 * 
 * @param configureQueueStatus       A pointer to a pointer to a z_bmqa_ConfigureQueueStatus object where
 *                         the newly created object will be stored.
 * @param queueId          A pointer to a z_bmqa_QueueId object representing the queue ID.
 * @param result           An integer representing the result.
 * @param errorDescription A string representing the error description.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_ConfigureQueueStatus__createFull(
    z_bmqa_ConfigureQueueStatus** configureQueueStatus,
    const z_bmqa_QueueId*         queueId,
    int                           result,
    const char*                   errorDescription);

/**
 * @brief Converts a z_bmqa_ConfigureQueueStatus object to a boolean value.
 * 
 * This function converts the z_bmqa_ConfigureQueueStatus object 'configureQueueStatus' to a boolean value.
 * 
 * @param configureQueueStatus A pointer to a z_bmqa_ConfigureQueueStatus object to be converted.
 * 
 * @return Returns true if the status object represents true, otherwise false.
 */
bool z_bmqa_ConfigureQueueStatus__toBool(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus);

/**
 * @brief Retrieves the queue ID from a z_bmqa_ConfigureQueueStatus object.
 * 
 * This function retrieves the queue ID from the z_bmqa_ConfigureQueueStatus object 'configureQueueStatus'.
 * 
 * @param configureQueueStatus  A pointer to a z_bmqa_ConfigureQueueStatus object from which the queue ID
 *                    will be retrieved.
 * @param queueId_obj A pointer to a pointer to a z_bmqa_QueueId object where the retrieved
 *                    queue ID will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_ConfigureQueueStatus__queueId(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus,
    z_bmqa_QueueId const**             queueId_obj);

/**
 * @brief Retrieves the result from a z_bmqa_ConfigureQueueStatus object.
 * 
 * This function retrieves the result from the z_bmqa_ConfigureQueueStatus object 'configureQueueStatus'.
 * 
 * @param configureQueueStatus A pointer to a z_bmqa_ConfigureQueueStatus object from which the result
 *                   will be retrieved.
 * 
 * @return Returns the result integer.
 */
int z_bmqa_ConfigureQueueStatus__result(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus);

/**
 * @brief Retrieves the error description from a z_bmqa_ConfigureQueueStatus object.
 * 
 * This function retrieves the error description from the z_bmqa_ConfigureQueueStatus object 'configureQueueStatus'.
 * 
 * @param configureQueueStatus A pointer to a z_bmqa_ConfigureQueueStatus object from which the error description
 *                   will be retrieved.
 * 
 * @return Returns a pointer to the error description string.
 */
const char* z_bmqa_ConfigureQueueStatus__errorDescription(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus);

#if defined(__cplusplus)
}
#endif

#endif
