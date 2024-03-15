#ifndef INCLUDED_Z_BMQA_CORRELATIONID
#define INCLUDED_Z_BMQA_CORRELATIONID

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct z_bmqt_CorrelationId {
    enum Type {
        ec_NUMERIC  // the 'CorrelationId' holds a 64-bit integer
        ,
        ec_POINTER  // the 'CorrelationId' holds a raw pointer
        ,
        ec_SHARED_PTR  // the 'CorrelationId' holds a shared pointer
        ,
        ec_AUTO_VALUE  // the 'CorrelationId' holds an auto value
        ,
        ec_UNSET  // the 'CorrelationId' is not set
    };
} z_bmqt_CorrelationId;

/**
 * @brief Deletes the memory allocated for a pointer to a bmqt::CorrelationId object.
 *
 * @param correlationId_obj A pointer to a pointer to a bmqt::CorrelationId object.
 *                          Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqt_CorrelationId__delete(z_bmqt_CorrelationId** correlationId_obj);

/**
 * @brief Creates a new bmqt::CorrelationId object and assigns its pointer to the provided pointer.
 *
 * This function dynamically allocates memory for a new bmqt::CorrelationId object and assigns its pointer to the provided pointer.
 * 
 * @param correlationId_obj A pointer to a pointer to a bmqt::CorrelationId object.
 *                          Upon successful completion, this pointer will be set to point to the newly allocated bmqt::CorrelationId object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj);

/**
 * @brief Creates a copy of a bmqt::CorrelationId object.
 *
 * This function creates a deep copy of the input bmqt::CorrelationId object 'other_obj'
 * and stores it in the memory pointed to by 'correlationId_obj'. Upon successful completion,
 * the pointer 'correlationId_obj' will point to the newly created copy.
 * 
 * @param correlationId_obj A pointer to a pointer to a bmqt::CorrelationId object where
 *                          the copy will be stored. Upon successful completion, this pointer
 *                          will point to the newly created copy.
 * @param other_obj         A pointer to a bmqt::CorrelationId object which will be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqt_CorrelationId__createCopy(z_bmqt_CorrelationId** correlationId_obj,
                                     const z_bmqt_CorrelationId* other_obj);

/**
 * @brief Creates a new correlation ID from a numeric value.
 *
 * This function allocates memory for a new bmqt::CorrelationId object initialized with the provided numeric value.
 * The pointer to the created object is stored in 'correlationId_obj'.
 * 
 * @param correlationId_obj A pointer to a pointer to a bmqt::CorrelationId object.
 *                          Upon successful completion, this pointer will hold the address of the newly created object.
 * @param numeric The numeric value used to initialize the new correlation ID object.
 * @return Returns 0 upon successful creation of the correlation ID object.
 */
int z_bmqt_CorrelationId__createFromNumeric(
    z_bmqt_CorrelationId** correlationId_obj,
    int64_t                numeric);

/**
 * @brief Creates a bmqt::CorrelationId object from a pointer.
 *
 * This function creates a bmqt::CorrelationId object using the provided pointer and assigns it to the input pointer to a bmqt::CorrelationId object.
 * 
 * @param correlationId_obj A pointer to a pointer to a bmqt::CorrelationId object.
 *                          Upon successful completion, this pointer will point to the newly created bmqt::CorrelationId object.
 * @param pointer The pointer used to create the bmqt::CorrelationId object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_CorrelationId__createFromPointer(
    z_bmqt_CorrelationId** correlationId_obj,
    void*                  pointer);

/**
 * @brief Makes a bmqt::CorrelationId object unset.
 *
 * This function invokes the 'makeUnset' method of the input bmqt::CorrelationId object.
 * 
 * @param correlationId_obj A pointer to a bmqt::CorrelationId object.
 *                          This object will be made unset upon successful completion.
 * 
 * @return Returns 0 upon successful invocation of 'makeUnset' on the bmqt::CorrelationId object.
 */
int z_bmqt_CorrelationId__makeUnset(z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Sets the numeric value for a bmqt::CorrelationId object.
 *
 * @param correlationId_obj A pointer to a bmqt::CorrelationId object.
 * @param numeric The numeric value to be set for the bmqt::CorrelationId object.
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_CorrelationId__setNumeric(z_bmqt_CorrelationId* correlation_Id_obj,
                                     int64_t               numeric);

/**
 * @brief Sets a pointer within a bmqt::CorrelationId object.
 *
 * This function sets a pointer within the given bmqt::CorrelationId object to the specified value.
 * 
 * @param correlationId_obj A pointer to a bmqt::CorrelationId object.
 * @param pointer           The pointer value to be set within the bmqt::CorrelationId object.
 * 
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_CorrelationId__setPointer(z_bmqt_CorrelationId* correlation_Id_obj,
                                     void*                 pointer);

/**
 * @brief Checks if the correlation ID is unset.
 *
 * This function checks if the correlation ID pointed to by the input pointer is unset.
 * 
 * @param correlationId_obj A pointer to a constant z_bmqt_CorrelationId object.
 * @return Returns true if the correlation ID is unset, false otherwise.
 */
bool z_bmqt_CorrelationId__isUnset(
    const z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Checks if the given correlation ID is numeric.
 *
 * This function checks whether the correlation ID pointed to by 'correlationId_obj' is numeric.
 * 
 * @param correlationId_obj A pointer to a z_bmqt_CorrelationId object.
 * @return Returns true if the correlation ID is numeric, false otherwise.
 */
bool z_bmqt_CorrelationId__isNumeric(
    const z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Checks if the given object is a pointer.
 *
 * @param correlationId_obj A pointer to a const z_bmqt_CorrelationId object.
 * @return Returns true if the object is a pointer; otherwise, returns false.
 */
bool z_bmqt_CorrelationId__isPointer(
    const z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Checks if the correlation ID is an auto-generated value.
 *
 * This function checks whether the given correlation ID is an auto-generated value.
 * 
 * @param correlationId_obj A pointer to a constant z_bmqt_CorrelationId object.
 * @return Returns true if the correlation ID is auto-generated, false otherwise.
 */
bool z_bmqt_CorrelationId__isAutoValue(
    const z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Retrieves the numeric value of a CorrelationId.
 *
 * This function retrieves the numeric value of the specified CorrelationId object.
 * 
 * @param correlationId_obj A pointer to a const z_bmqt_CorrelationId object from which to retrieve the numeric value.
 * @return Returns the numeric value of the CorrelationId.
 */
int64_t z_bmqt_CorrelationId__theNumeric(
    const z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Returns the pointer stored within a bmqt::CorrelationId object.
 *
 * This function returns the pointer stored within the input bmqt::CorrelationId object.
 * 
 * @param correlationId_obj A pointer to a bmqt::CorrelationId object.
 * 
 * @return Returns the pointer stored within the bmqt::CorrelationId object.
 */
void* z_bmqt_CorrelationId__thePointer(
    const z_bmqt_CorrelationId* correlation_Id_obj);

/**
 * @brief Retrieves the type of a z_bmqt_CorrelationId object.
 *
 * This function returns the type of the z_bmqt_CorrelationId object specified by 'correlationId_obj'.
 * 
 * @param correlationId_obj A pointer to a z_bmqt_CorrelationId object.
 * @return Returns the type of the z_bmqt_CorrelationId object.
 */
z_bmqt_CorrelationId::Type
z_bmqt_CorrelationId__type(const z_bmqt_CorrelationId* correlationId_obj);

/**
 * @brief Generates an auto value for a bmqt::CorrelationId object and assigns it to a pointer.
 *
 * This function creates a new bmqt::CorrelationId object with an auto-generated value and assigns it to the pointer provided.
 * 
 * @param correlationId_obj A pointer to a pointer to a bmqt::CorrelationId object.
 *                          Upon successful completion, this pointer will hold the auto-generated correlation ID.
 * @return Returns 0 upon successful generation of the auto value.
 */
int z_bmqt_CorrelationId__autoValue(z_bmqt_CorrelationId** correlationId_obj);

/**
 * @brief Compares two z_bmqt_CorrelationId objects.
 *
 * This function compares two z_bmqt_CorrelationId objects and returns an integer value indicating their relative order.
 * 
 * @param a Pointer to the first z_bmqt_CorrelationId object.
 * @param b Pointer to the second z_bmqt_CorrelationId object.
 * @return Returns 0 if the z_bmqt_CorrelationId objects are equal, a negative value if 'a' is less than 'b', and a positive value if 'a' is greater than 'b'.
 */
int z_bmqt_CorrelationId__compare(const z_bmqt_CorrelationId* a,
                                  const z_bmqt_CorrelationId* b);

/**
 * @brief Assigns the value of one bmqt::CorrelationId object to another.
 *
 * This function assigns the value of the bmqt::CorrelationId object pointed to by 'src' to the bmqt::CorrelationId object pointed to by 'dst'.
 * 
 * @param dst A pointer to a pointer to a bmqt::CorrelationId object where the value will be assigned.
 * @param src A pointer to a const bmqt::CorrelationId object from which the value will be copied.
 * 
 * @return Returns 0 upon successful assignment.
 */
int z_bmqt_CorrelationId__assign(z_bmqt_CorrelationId**      dst,
                                 const z_bmqt_CorrelationId* src);

/**
 * @brief Converts a bmqt::CorrelationId object to a string representation.
 *
 * This function converts the given bmqt::CorrelationId object to a string representation and stores it in the output buffer.
 * The memory for the output buffer is allocated dynamically and should be deallocated by the caller.
 * 
 * @param correlationId_obj A pointer to a bmqt::CorrelationId object to be converted to a string.
 * @param out               A pointer to a pointer to a character buffer where the string representation will be stored.
 *                          Upon successful completion, this pointer will point to the dynamically allocated buffer containing the string.
 *                          The caller is responsible for deallocating the memory allocated for this buffer.
 * @return Returns 0 upon successful conversion.
 */
int z_bmqt_CorrelationId__toString(
    const z_bmqt_CorrelationId* correlationId_obj,
    char**                      out);

#if defined(__cplusplus)
}
#endif

#endif