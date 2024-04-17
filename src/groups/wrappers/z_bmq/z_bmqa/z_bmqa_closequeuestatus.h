#ifndef INCLUDED_Z_BMQA_CLOSEQUEUESTATUS
#define INCLUDED_Z_BMQA_CLOSEQUEUESTATUS

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_queueid.h>

typedef struct z_bmqa_CloseQueueStatus z_bmqa_CloseQueueStatus;

/**
 * @brief Deletes the memory allocated for a pointer to a bmqa::CloseQueueStatus object.
 *
 * This function deallocates the memory pointed to by the input pointer to a bmqa::CloseQueueStatus object and sets the pointer to NULL.
 * 
 * @param closeQueueStatus A pointer to a pointer to a bmqa::CloseQueueStatus object.
 *                   Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_CloseQueueStatus__delete(z_bmqa_CloseQueueStatus** closeQueueStatus);

/**
 * @brief Creates a new bmqa::CloseQueueStatus object.
 *
 * This function creates a new bmqa::CloseQueueStatus object and assigns its pointer to the provided pointer.
 * 
 * @param closeQueueStatus A pointer to a pointer to a bmqa::CloseQueueStatus object.
 *                   Upon successful completion, this pointer will hold the newly created CloseQueueStatus object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_CloseQueueStatus__create(z_bmqa_CloseQueueStatus** closeQueueStatus);

/**
 * @brief Creates a copy of a bmqa::CloseQueueStatus object.
 *
 * This function creates a copy of the specified bmqa::CloseQueueStatus object and assigns its pointer to the provided pointer.
 * 
 * @param closeQueueStatus A pointer to a pointer to a bmqa::CloseQueueStatus object.
 *                   Upon successful completion, this pointer will hold the newly created copy of the CloseQueueStatus object.
 * @param other A pointer to a constant bmqa::CloseQueueStatus object to copy.
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_CloseQueueStatus__createCopy(z_bmqa_CloseQueueStatus** closeQueueStatus,
                                        const z_bmqa_CloseQueueStatus* other);

/**
 * @brief Creates a bmqa::CloseQueueStatus object with specified parameters.
 *
 * This function creates a new bmqa::CloseQueueStatus object with the specified parameters and assigns its pointer to the provided pointer.
 * 
 * @param closeQueueStatus A pointer to a pointer to a bmqa::CloseQueueStatus object.
 *                   Upon successful completion, this pointer will hold the newly created CloseQueueStatus object.
 * @param queueId A pointer to a constant z_bmqa_QueueId object representing the queue identifier.
 * @param result An integer representing the result of the close operation.
 * @param errorDescription A pointer to a constant character string representing the error description.
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_CloseQueueStatus__createFull(z_bmqa_CloseQueueStatus** closeQueueStatus,
                                        const z_bmqa_QueueId*     queueId,
                                        int                       result,
                                        const char* errorDescription);

/**
 * @brief Converts a CloseQueueStatus object to a boolean value.
 *
 * This function converts the specified CloseQueueStatus object to a boolean value.
 * 
 * @param closeQueueStatus A pointer to a constant z_bmqa_CloseQueueStatus object to convert.
 * @return Returns true if the CloseQueueStatus object indicates success, false otherwise.
 */
bool z_bmqa_CloseQueueStatus__toBool(
    const z_bmqa_CloseQueueStatus* closeQueueStatus);

/**
 * @brief Retrieves the queue identifier from a CloseQueueStatus object.
 *
 * This function retrieves the queue identifier from the specified CloseQueueStatus object.
 * 
 * @param closeQueueStatus A pointer to a constant z_bmqa_CloseQueueStatus object.
 * @param queueId_obj A pointer to a pointer to a constant z_bmqa_QueueId object.
 *                    Upon successful completion, this pointer will hold the queue identifier.
 * @return Returns 0 upon successful retrieval of the queue identifier.
 */
int z_bmqa_CloseQueueStatus__queueId(const z_bmqa_CloseQueueStatus* closeQueueStatus,
                                     z_bmqa_QueueId const** queueId_obj);

/**
 * @brief Retrieves the result of a close queue operation from a CloseQueueStatus object.
 *
 * This function retrieves the result of the close queue operation from the specified CloseQueueStatus object.
 * 
 * @param closeQueueStatus A pointer to a constant z_bmqa_CloseQueueStatus object.
 * @return Returns an integer representing the result of the close queue operation.
 */
int z_bmqa_CloseQueueStatus__result(const z_bmqa_CloseQueueStatus* closeQueueStatus);

/**
 * @brief Retrieves the error description from a CloseQueueStatus object.
 *
 * This function retrieves the error description from the specified CloseQueueStatus object.
 * 
 * @param closeQueueStatus A pointer to a constant z_bmqa_CloseQueueStatus object.
 * @return Returns a pointer to a constant character string representing the error description.
 */
const char* z_bmqa_CloseQueueStatus__errorDescription(
    const z_bmqa_CloseQueueStatus* closeQueueStatus);

#if defined(__cplusplus)
}
#endif

#endif