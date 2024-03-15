#ifndef INCLUDED_Z_BMQT_MESSAGEGUID
#define INCLUDED_Z_BMQT_MESSAGEGUID

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqt_MessageGUID z_bmqt_MessageGUID;

/**
 * @brief Deletes the memory allocated for a pointer to a bmqt::MessageGUID object.
 *
 * @param messageGUID_obj A pointer to a pointer to a bmqt::MessageGUID object.
 *                        Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqt_MessageGUID__delete(z_bmqt_MessageGUID** messageGUID_obj);

/**
 * @brief Allocates memory for a pointer to a bmqt::MessageGUID object.
 *
 * This function allocates memory for a bmqt::MessageGUID object and assigns the pointer to the input pointer 'messageGUID_obj'.
 * 
 * @param messageGUID_obj A pointer to a pointer to a bmqt::MessageGUID object.
 *                        Upon successful completion, this pointer will point to the newly allocated bmqt::MessageGUID object.
 * @return Returns 0 upon successful memory allocation.
 */
int z_bmqt_MessageGUID__create(z_bmqt_MessageGUID** messageGUID_obj);

/**
 * @brief Converts binary data to a bmqt::MessageGUID object.
 *
 * @param messageGUID_obj A pointer to a bmqt::MessageGUID object where the converted data will be stored.
 * @param buffer The binary data buffer to convert into a bmqt::MessageGUID object.
 * @return Returns 0 upon successful conversion.
 */
int z_bmqt_MessageGUID__fromBinary(z_bmqt_MessageGUID*  messageGUID_obj,
                                   const unsigned char* buffer);

/**
 * @brief Converts a hexadecimal string to a bmqt::MessageGUID object.
 *
 * This function converts a hexadecimal string representation of a bmqt::MessageGUID object
 * into the corresponding bmqt::MessageGUID object pointed to by 'messageGUID_obj'.
 * 
 * @param messageGUID_obj A pointer to a bmqt::MessageGUID object where the converted
 *                        MessageGUID will be stored.
 * @param buffer A null-terminated string containing the hexadecimal representation
 *               of the MessageGUID.
 * 
 * @return Returns 0 upon successful conversion.
 */
int z_bmqt_MessageGUID__fromHex(z_bmqt_MessageGUID* messageGUID_obj,
                                const char*         buffer);

/**
 * @brief Checks if a bmqt::MessageGUID object is unset.
 *
 * @param messageGUID_obj A pointer to a bmqt::MessageGUID object.
 * @return Returns true if the given bmqt::MessageGUID object is unset, false otherwise.
 */
bool z_bmqt_MessageGUID__isUnset(const z_bmqt_MessageGUID* messageGUID_obj);

/**
 * @brief Converts a bmqt::MessageGUID object to its binary representation.
 *
 * @param messageGUID_obj A pointer to a bmqt::MessageGUID object to be converted to binary.
 * @param destination A pointer to an unsigned char buffer where the binary representation will be stored.
 * 
 * @returns Returns 0 upon successful conversion.
 */
int z_bmqt_MessageGUID__toBinary(const z_bmqt_MessageGUID* messageGUID_obj,
                                 unsigned char*            destination);

/**
 * @brief Converts a bmqt::MessageGUID object to its hexadecimal representation.
 *
 * This function converts the given bmqt::MessageGUID object to its hexadecimal representation and stores it in the provided destination buffer.
 * 
 * @param messageGUID_obj A pointer to a bmqt::MessageGUID object to be converted.
 * @param destination A pointer to the destination buffer where the hexadecimal representation will be stored.
 * @return Returns 0 upon successful conversion.
 */
int z_bmqt_MessageGUID__toHex(const z_bmqt_MessageGUID* messageGUID_obj,
                              char*                     destination);

/**
 * @brief Converts a bmqt::MessageGUID object to a string representation.
 *
 * This function converts the given bmqt::MessageGUID object to a string representation and stores it in the output buffer.
 * 
 * @param messageGUID_obj A pointer to a bmqt::MessageGUID object to be converted.
 * @param out             A pointer to a pointer to a character buffer where the string representation will be stored.
 *                        Upon successful completion, this pointer will be set to point to the allocated memory containing the string.
 *                        The caller is responsible for freeing the memory allocated for the string.
 * @return Returns 0 upon successful conversion.
 */
int z_bmqt_MessageGUID__toString(const z_bmqt_MessageGUID* messageGUID_obj,
                                 char**                    out);

#if defined(__cplusplus)
}
#endif

#endif