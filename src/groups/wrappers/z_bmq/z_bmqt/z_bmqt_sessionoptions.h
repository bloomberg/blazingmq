#ifndef INCLUDED_Z_BMQT_SESSIONOPTIONS
#define INCLUDED_Z_BMQT_SESSIONOPTIONS

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqt_SessionOptions z_bmqt_SessionOptions;

/**
 * @brief Deletes a session options object.
 *
 * This function deallocates the memory allocated for a pointer to a bmqt::SessionOptions object.
 * 
 * @param options_obj A pointer to a pointer to a bmqt::SessionOptions object.
 *                    Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqt_SessionOptions__delete(z_bmqt_SessionOptions** options_obj);

/**
 * @brief Creates a new session options object.
 *
 * This function creates a new bmqt::SessionOptions object and assigns its pointer to the provided pointer.
 * 
 * @param options_obj A pointer to a pointer to a bmqt::SessionOptions object.
 *                    Upon successful completion, this pointer will hold the newly created session options object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_SessionOptions__create(z_bmqt_SessionOptions** options_obj);

/**
 * @brief Retrieves the broker URI from the session options.
 *
 * This function returns the broker URI from the specified session options.
 * 
 * @param options_obj A pointer to a constant z_bmqt_SessionOptions object from which to retrieve the broker URI.
 * @return Returns a pointer to a constant character string representing the broker URI.
 *         The returned pointer may become invalid if the underlying session options object is modified or destroyed.
 */
const char*
z_bmqt_SessionOptions__brokerUri(const z_bmqt_SessionOptions* options_obj);

#if defined(__cplusplus)
}
#endif

#endif