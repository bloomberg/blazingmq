#ifndef INCLUDED_Z_BMQA_CONFIRMEVENTBUILDER
#define INCLUDED_Z_BMQA_CONFIRMEVENTBUILDER

#if defined(__cplusplus)
extern "C" {
#endif

#include <z_bmqa_message.h>
#include <z_bmqt_types.h>

typedef struct z_bmqa_ConfirmEventBuilder z_bmqa_ConfirmEventBuilder;
typedef struct z_bmqa_MessageConfirmationCookie
    z_bmqa_MessageConfirmationCookie;

/**
 * @brief Deletes a z_bmqa_ConfirmEventBuilder object.
 * 
 * This function deletes the z_bmqa_ConfirmEventBuilder object pointed to by 'confirmEventBuilder'.
 * Upon successful completion, the memory pointed to by 'confirmEventBuilder' will be deallocated,
 * and 'confirmEventBuilder' will be set to NULL.
 * 
 * @param confirmEventBuilder A pointer to a pointer to a z_bmqa_ConfirmEventBuilder object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_ConfirmEventBuilder__delete(
    z_bmqa_ConfirmEventBuilder** confirmEventBuilder);

/**
 * @brief Creates a new z_bmqa_ConfirmEventBuilder object.
 * 
 * This function creates a new z_bmqa_ConfirmEventBuilder object and stores
 * a pointer to it in the memory pointed to by 'confirmEventBuilder'.
 * 
 * @param confirmEventBuilder A pointer to a pointer to a z_bmqa_ConfirmEventBuilder object where
 *                   the newly created object will be stored.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_ConfirmEventBuilder__create(
    z_bmqa_ConfirmEventBuilder** confirmEventBuilder);

/**
 * @brief Adds a message confirmation to the z_bmqa_ConfirmEventBuilder object.
 * 
 * This function adds a message confirmation represented by the z_bmqa_Message object 'message'
 * to the z_bmqa_ConfirmEventBuilder object 'confirmEventBuilder'.
 * 
 * @param confirmEventBuilder A pointer to a z_bmqa_ConfirmEventBuilder object to which the message
 *                    confirmation will be added.
 * @param message     A pointer to a z_bmqa_Message object representing the message confirmation.
 * 
 * @return Returns 0 upon successful addition of the message confirmation.
 */
int z_bmqa_ConfirmEventBuilder__addMessageConfirmation(
    z_bmqa_ConfirmEventBuilder* confirmEventBuilder,
    const z_bmqa_Message*       message);

/**
 * @brief Adds a message confirmation with a cookie to the z_bmqa_ConfirmEventBuilder object.
 * 
 * This function adds a message confirmation represented by the z_bmqa_MessageConfirmationCookie
 * object 'cookie' to the z_bmqa_ConfirmEventBuilder object 'confirmEventBuilder'.
 * 
 * @param confirmEventBuilder A pointer to a z_bmqa_ConfirmEventBuilder object to which the message
 *                    confirmation will be added.
 * @param cookie      A pointer to a z_bmqa_MessageConfirmationCookie object representing
 *                    the message confirmation with a cookie.
 * 
 * @return Returns 0 upon successful addition of the message confirmation.
 */
int z_bmqa_ConfirmEventBuilder__addMessageConfirmationWithCookie(
    z_bmqa_ConfirmEventBuilder*             confirmEventBuilder,
    const z_bmqa_MessageConfirmationCookie* cookie);

/**
 * @brief Resets the z_bmqa_ConfirmEventBuilder object.
 * 
 * This function resets the z_bmqa_ConfirmEventBuilder object 'confirmEventBuilder' to its initial state.
 * 
 * @param confirmEventBuilder A pointer to a z_bmqa_ConfirmEventBuilder object to be reset.
 * 
 * @return Returns 0 upon successful reset.
 */
int z_bmqa_ConfirmEventBuilder__reset(z_bmqa_ConfirmEventBuilder* confirmEventBuilder);

/**
 * @brief Retrieves the message count from the z_bmqa_ConfirmEventBuilder object.
 * 
 * This function retrieves the count of message confirmations from the
 * z_bmqa_ConfirmEventBuilder object 'confirmEventBuilder'.
 * 
 * @param confirmEventBuilder A pointer to a z_bmqa_ConfirmEventBuilder object from which the message count
 *                    will be retrieved.
 * 
 * @return Returns the count of message confirmations.
 */
int z_bmqa_ConfirmEventBuilder__messageCount(
    const z_bmqa_ConfirmEventBuilder* confirmEventBuilder);

/**
 * @brief Retrieves the blob from the z_bmqa_ConfirmEventBuilder object.
 * 
 * This function retrieves the blob from the z_bmqa_ConfirmEventBuilder object 'confirmEventBuilder'.
 * 
 * @param confirmEventBuilder A pointer to a z_bmqa_ConfirmEventBuilder object from which the blob
 *                    will be retrieved.
 * @param blob_obj    A pointer to a pointer to a z_bmqt_Blob object where the retrieved blob
 *                    will be stored.
 * 
 * @return Returns 0 upon successful retrieval.
 */
int z_bmqa_ConfirmEventBuilder__blob(z_bmqa_ConfirmEventBuilder* confirmEventBuilder,
                                     const z_bmqt_Blob**         blob_obj);

#if defined(__cplusplus)
}
#endif

#endif
