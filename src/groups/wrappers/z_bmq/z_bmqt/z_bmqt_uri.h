#ifndef INCLUDED_Z_BMQT_URI
#define INCLUDED_Z_BMQT_URI

#if defined(__cplusplus)
extern "C" {
#endif

// =========
// class Uri
// =========

typedef struct z_bmqt_Uri z_bmqt_Uri;

/**
 * @brief Creates a copy of a URI object.
 *
 * This function creates a copy of the specified z_bmqt_Uri object and assigns its pointer to the provided pointer.
 * 
 * @param uri_obj A pointer to a pointer to a z_bmqt_Uri object.
 *                Upon successful completion, this pointer will hold the newly created copy of the URI object.
 * @param to_copy A pointer to a z_bmqt_Uri object to copy.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_Uri_create(z_bmqt_Uri** uri_obj, z_bmqt_Uri* to_copy);

// ================
// class UriBuilder
// ================

typedef struct z_bmqt_UriBuilder z_bmqt_UriBuilder;

/**
 * @brief Creates a new URI builder object.
 *
 * This function creates a new z_bmqt_UriBuilder object and assigns its pointer to the provided pointer.
 * 
 * @param uribuilder_obj A pointer to a pointer to a z_bmqt_UriBuilder object.
 *                       Upon successful completion, this pointer will hold the newly created URI builder object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_UriBuilder__create(z_bmqt_UriBuilder** uribuilder_obj);

/**
 * @brief Creates a URI builder object from an existing URI.
 *
 * This function creates a new z_bmqt_UriBuilder object initialized with the specified URI and assigns its pointer to the provided pointer.
 * 
 * @param uribuilder_obj A pointer to a pointer to a z_bmqt_UriBuilder object.
 *                       Upon successful completion, this pointer will hold the newly created URI builder object.
 * @param uri_obj A pointer to a constant z_bmqt_Uri object from which to create the URI builder.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_UriBuilder__createFromUri(z_bmqt_UriBuilder** uribuilder_obj,
                                     const z_bmqt_Uri*   uri_obj);
/**
 * @brief Sets the domain for a URI builder.
 *
 * This function sets the domain for the specified URI builder.
 * 
 * @param uribuilder_obj A pointer to a z_bmqt_UriBuilder object for which to set the domain.
 * @param value A pointer to a constant character string representing the domain to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_UriBuilder__setDomain(z_bmqt_UriBuilder* uribuilder_obj,
                                 const char*        value);

/**
 * @brief Sets the tier for a URI builder.
 *
 * This function sets the tier for the specified URI builder.
 * 
 * @param uribuilder_obj A pointer to a z_bmqt_UriBuilder object for which to set the tier.
 * @param value A pointer to a constant character string representing the tier to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_UriBuilder__setTier(z_bmqt_UriBuilder* uribuilder_obj,
                               const char*        value);

/**
 * @brief Sets the qualified domain for a URI builder.
 *
 * This function sets the qualified domain for the specified URI builder.
 * 
 * @param uribuilder_obj A pointer to a z_bmqt_UriBuilder object for which to set the qualified domain.
 * @param value A pointer to a constant character string representing the qualified domain to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_UriBuilder__setQualifiedDomain(z_bmqt_UriBuilder* uribuilder_obj,
                                          const char*        value);

/**
 * @brief Sets the queue name for a URI builder.
 *
 * This function sets the queue name for the specified URI builder.
 * 
 * @param uribuilder_obj A pointer to a z_bmqt_UriBuilder object for which to set the queue name.
 * @param value A pointer to a constant character string representing the queue name to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_UriBuilder__setQueue(z_bmqt_UriBuilder* uribuilder_obj,
                                const char*        value);

/**
 * @brief Retrieves the URI built by a URI builder.
 *
 * This function retrieves the URI built by the specified URI builder.
 * 
 * @param uribuilder_obj A pointer to a z_bmqt_UriBuilder object to build the URI from.
 * @param uri_obj A pointer to a pointer to a z_bmqt_Uri object.
 *                Upon successful completion, this pointer will hold the newly created URI object.
 * @return Returns 0 upon successful retrieval of the URI.
 */
int z_bmqt_UriBuilder_uri(z_bmqt_UriBuilder* uribuilder_obj,
                          z_bmqt_Uri**       uri_obj);

#if defined(__cplusplus)
}
#endif

#endif