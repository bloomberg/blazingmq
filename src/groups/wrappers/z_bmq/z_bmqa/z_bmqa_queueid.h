#ifndef INCLUDED_Z_BMQA_QUEUEID
#define INCLUDED_Z_BMQA_QUEUEID

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <z_bmqt_correlationid.h>
#include <z_bmqt_queueoptions.h>
#include <z_bmqt_uri.h>

typedef struct z_bmqa_QueueId z_bmqa_QueueId;

/**
 * @brief Deletes a queue ID object.
 * 
 * This function deallocates memory for the queue ID object.
 * 
 * @param queueId A pointer to a pointer to the z_bmqa_QueueId object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_QueueId__delete(z_bmqa_QueueId** queueId);

/**
 * @brief Creates a new queue ID object.
 * 
 * This function creates a new queue ID object.
 * 
 * @param queueId A pointer to a pointer to the z_bmqa_QueueId object to be created.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId);

/**
 * @brief Creates a copy of a queue ID object.
 * 
 * This function creates a copy of an existing queue ID object.
 * 
 * @param queueId A pointer to a pointer to the z_bmqa_QueueId object where the copy will be stored.
 * @param other       A pointer to the const z_bmqa_QueueId object to be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_QueueId__createCopy(z_bmqa_QueueId**      queueId,
                               const z_bmqa_QueueId* other);

/**
 * @brief Creates a queue ID object from a correlation ID.
 * 
 * This function creates a queue ID object from a correlation ID.
 * 
 * @param queueId     A pointer to a pointer to the z_bmqa_QueueId object to be created.
 * @param correlationId   A pointer to the const z_bmqt_CorrelationId object representing the correlation ID.
 * 
 * @return Returns 0 upon successful creation of the object.
 */
int z_bmqa_QueueId__createFromCorrelationId(
    z_bmqa_QueueId**            queueId,
    const z_bmqt_CorrelationId* correlationId);

/**
 * @brief Creates a queue ID object from a numeric value.
 * 
 * This function creates a queue ID object from a numeric value.
 * 
 * @param queueId A pointer to a pointer to the z_bmqa_QueueId object to be created.
 * @param numeric     The numeric value for the queue ID.
 * 
 * @return Returns 0 upon successful creation of the object.
 */
int z_bmqa_QueueId__createFromNumeric(z_bmqa_QueueId** queueId,
                                      int64_t          numeric);

/**
 * @brief Creates a queue ID object from a pointer.
 * 
 * This function creates a queue ID object from a pointer.
 * 
 * @param queueId A pointer to a pointer to the z_bmqa_QueueId object to be created.
 * @param pointer     The pointer value for the queue ID.
 * 
 * @return Returns 0 upon successful creation of the object.
 */
int z_bmqa_QueueId__createFromPointer(z_bmqa_QueueId** queueId,
                                      void*            pointer);

/**
 * @brief Retrieves the correlation ID from a queue ID object.
 * 
 * This function retrieves the correlation ID from a queue ID object.
 * 
 * @param queueId   A pointer to the const z_bmqa_QueueId object.
 * @param queueId   A pointer to a pointer to the const z_bmqt_CorrelationId object to store the correlation ID.
 * 
 * @return Returns 0 upon successful retrieval.
 */
const z_bmqt_CorrelationId*
z_bmqa_QueueId__correlationId(const z_bmqa_QueueId* queueId);

/**
 * @brief Retrieves the flags from a queue ID object.
 * 
 * This function retrieves the flags from a queue ID object.
 * 
 * @param queueId A pointer to the const z_bmqa_QueueId object.
 * 
 * @return Returns the flags associated with the queue ID.
 */
uint64_t z_bmqa_QueueId__flags(const z_bmqa_QueueId* queueId);

/**
 * @brief Retrieves the URI from a queue ID object.
 * 
 * This function retrieves the URI from a queue ID object.
 * 
 * @param queueId A pointer to the const z_bmqa_QueueId object.
 * 
 * @return Returns a pointer to the const z_bmqt_Uri object representing the URI.
 */
const z_bmqt_Uri* z_bmqa_QueueId__uri(const z_bmqa_QueueId* queueId);

/**
 * @brief Retrieves the options from a queue ID object.
 * 
 * This function retrieves the options from a queue ID object.
 * 
 * @param queueId A pointer to the const z_bmqa_QueueId object.
 * 
 * @return Returns a pointer to the const z_bmqt_QueueOptions object representing the options.
 */
const z_bmqt_QueueOptions* options(const z_bmqa_QueueId* queueId);

/**
 * @brief Checks if a queue ID object is valid.
 * 
 * This function checks if a queue ID object is valid.
 * 
 * @param queueId A pointer to the const z_bmqa_QueueId object.
 * 
 * @return Returns 1 if the queue ID is valid, 0 otherwise.
 */
int z_bmqa_QueueId__isValid(const z_bmqa_QueueId* queueId);

#if defined(__cplusplus)
}
#endif

#endif
