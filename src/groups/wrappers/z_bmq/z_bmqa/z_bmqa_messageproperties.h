#ifndef INCLUDED_Z_BMQA_MESSAGEPROPERTIES
#define INCLUDED_Z_BMQA_MESSAGEPROPERTIES

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>

typedef struct z_bmqa_MessageProperties {
    static const int k_MAX_NUM_PROPERTIES;
    static const int k_MAX_PROPERTIES_AREA_LENGTH;
    static const int k_MAX_PROPERTY_NAME_LENGTH;
    static const int k_MAX_PROPERTY_VALUE_LENGTH;
} z_bmqa_MessageProperties;

enum Enum {
    e_UNDEFINED = 0,
    e_BOOL      = 1,
    e_CHAR      = 2,
    e_SHORT     = 3,
    e_INT32     = 4,
    e_INT64     = 5,
    e_STRING    = 6,
    e_BINARY    = 7
};

/**
 * @brief Clears the message properties.
 *
 * This function clears the message properties.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object to be cleared.
 */
void z_bmqa_MessageProperties__clear(
    z_bmqa_MessageProperties* properties_obj);

/**
 * @brief Removes a property from the message properties.
 *
 * This function removes a property from the message properties.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property to be removed.
 * @param buffer A pointer to an Enum buffer.
 * @return Returns true if the property is successfully removed, false otherwise.
 */
bool z_bmqa_MessageProperties__remove(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    Enum *buffer);

/**
 * @brief Deletes the message properties object.
 *
 * This function deletes the message properties object.
 *
 * @param properties_obj A pointer to a pointer to a z_bmqa_MessageProperties object.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageProperties__delete(
    z_bmqa_MessageProperties** properties_obj);


/**
 * @brief Creates a new message properties object.
 *
 * This function creates a new message properties object.
 *
 * @param properties_obj A pointer to a pointer to a z_bmqa_MessageProperties object where the new object will be stored.
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_MessageProperties__create(
    z_bmqa_MessageProperties** properties_obj);

/**
 * @brief Creates a copy of a message properties object.
 *
 * This function creates a copy of a message properties object.
 *
 * @param properties_obj A pointer to a pointer to a z_bmqa_MessageProperties object where the copy will be stored.
 * @param other A pointer to a z_bmqa_MessageProperties object to be copied.
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_MessageProperties__createCopy(
    z_bmqa_MessageProperties**      properties_obj,
    const z_bmqa_MessageProperties* other);

/**
 * @brief Sets a property as a boolean value.
 *
 * This function sets a property as a boolean value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The boolean value of the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsBool(
    z_bmqa_MessageProperties* properties_obj,
    const char*               name,
    bool                      value);

/**
 * @brief Sets a property as a character value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The character value of the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsChar(
    z_bmqa_MessageProperties* properties_obj,
    const char* name,
    char value);

/**
 * @brief Sets a property as a short value.
 *
 * This function sets a property as a short value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The short value of the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsShort(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    short value);

/**
 * @brief Sets a property as a 32-bit integer value.
 *
 * This function sets a property as a 32-bit integer value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The 32-bit integer value of the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsInt32(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    int32_t value);

/**
 * @brief Sets a property as a 64-bit integer value.
 *
 * This function sets a property as a 64-bit integer value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The 64-bit integer value of the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsInt64(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    long long value);

/**
 * @brief Sets a property as a string value.
 *
 * This function sets a property as a string value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The string value of the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsString(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value);

/**
 * @brief Sets a property as a binary value.
 *
 * This function sets a property as a binary value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The binary value of the property.
 * @param size The size of the binary value.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsBinary(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value,
    int size);
/**
 * @brief Retrieves the number of properties in the message properties object.
 *
 * This function retrieves the number of properties in the message properties object.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @return Returns the number of properties.
 */
int z_bmqa_MessageProperties__numProperties(
    const z_bmqa_MessageProperties* properties_obj); 

/**
 * @brief Retrieves the total size of the message properties object.
 *
 * This function retrieves the total size of the message properties object.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @return Returns the total size of the message properties object.
 */
int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj);

/**
 * @brief Retrieves a property as a boolean value.
 *
 * This function retrieves a property as a boolean value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the boolean value of the property.
 */
bool z_bmqa_MessageProperties__getPropertyAsBool(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a character value.
 *
 * This function retrieves a property as a character value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the character value of the property.
 */
char z_bmqa_MessageProperties__getPropertyAsChar(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a short value.
 *
 * This function retrieves a property as a short value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the short value of the property.
 */
short z_bmqa_MessageProperties__getPropertyAsShort(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a 32-bit integer value.
 *
 * This function retrieves a property as a 32-bit integer value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the 32-bit integer value of the property.
 */
int32_t z_bmqa_MessageProperties__getPropertyAsInt32(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a 64-bit integer value.
 *
 * This function retrieves a property as a 64-bit integer value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the 64-bit integer value of the property.
 */
long long z_bmqa_MessageProperties__getPropertyAsInt64(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a string value.
 *
 * This function retrieves a property as a string value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the string value of the property.
 */
const char* z_bmqa_MessageProperties__getPropertyAsString(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a binary value.
 *
 * This function retrieves a property as a binary value.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @return Returns the binary value of the property.
 */
const char* z_bmqa_MessageProperties__getPropertyAsBinary(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property as a boolean value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a boolean value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the boolean value of the property if it exists, otherwise returns the default value.
 */
bool z_bmqa_MessageProperties__getPropertyAsBoolOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    bool value); 

/**
 * @brief Retrieves a property as a character value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a character value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the character value of the property if it exists, otherwise returns the default value.
 */
char z_bmqa_MessageProperties__getPropertyAsCharOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    char value); 

/**
 * @brief Retrieves a property as a short value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a short value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the short value of the property if it exists, otherwise returns the default value.
 */
short z_bmqa_MessageProperties__getPropertyAsShortOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    short value); 

/**
 * @brief Retrieves a property as a 32-bit integer value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a 32-bit integer value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the 32-bit integer value of the property if it exists, otherwise returns the default value.
 */
int32_t z_bmqa_MessageProperties__getPropertyAsInt32Or(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    int32_t value); 

/**
 * @brief Retrieves a property as a 64-bit integer value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a 64-bit integer value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the 64-bit integer value of the property if it exists, otherwise returns the default value.
 */
long long z_bmqa_MessageProperties__getPropertyAsInt64Or(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    long long value);

/**
 * @brief Retrieves a property as a string value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a string value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the string value of the property if it exists, otherwise returns the default value.
 */
const char* z_bmqa_MessageProperties__getPropertyAsStringOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value);

/**
 * @brief Retrieves a property as a binary value, or returns a default value if the property does not exist.
 *
 * This function retrieves a property as a binary value, or returns a default value if the property does not exist.
 *
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property.
 * @param value The default value to return if the property does not exist.
 * @return Returns the binary value of the property if it exists, otherwise returns the default value.
 */
const char* z_bmqa_MessageProperties__getPropertyAsBinaryOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value);

#if defined(__cplusplus)
}
#endif

#endif