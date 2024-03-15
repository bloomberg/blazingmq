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

/**
 * @brief Clears the properties of a bmqa::MessageProperties object.
 *
 * This function clears the properties of the input bmqa::MessageProperties object.
 * 
 * @param properties_obj A pointer to a bmqa::MessageProperties object whose properties are to be cleared.
 * 
 * @returns None.
 */
void z_bmqa_MessageProperties__clear(
    z_bmqa_MessageProperties* properties_obj);

/**
 * @brief Deletes the memory allocated for a pointer to a bmqa::MessageProperties object.
 *
 * This function deallocates the memory pointed to by the input pointer to a bmqa::MessageProperties object and sets the pointer to NULL.
 * 
 * @param properties_obj A pointer to a pointer to a bmqa::MessageProperties object.
 *                       Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageProperties__delete(
    z_bmqa_MessageProperties** properties_obj);

/**
 * @brief Creates a new instance of bmqa::MessageProperties and assigns it to the provided pointer.
 *
 * This function allocates memory for a new instance of bmqa::MessageProperties and assigns it to the provided pointer.
 * 
 * @param properties_obj A pointer to a pointer to a bmqa::MessageProperties object.
 *                       Upon successful completion, this pointer will hold the address of the newly created object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_MessageProperties__create(
    z_bmqa_MessageProperties** properties_obj);

/**
 * @brief Creates a copy of a bmqa::MessageProperties object.
 *
 * This function creates a copy of the input bmqa::MessageProperties object
 * and assigns it to the pointer provided as 'properties_obj'.
 * 
 * @param properties_obj A pointer to a pointer to a bmqa::MessageProperties object.
 *                       Upon successful completion, this pointer will hold the
 *                       address of the newly created copy of the input object.
 * @param other          A pointer to the const bmqa::MessageProperties object
 *                       that needs to be copied.
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_MessageProperties__createCopy(
    z_bmqa_MessageProperties**      properties_obj,
    const z_bmqa_MessageProperties* other);

/**
 * @brief Sets a boolean property in a z_bmqa_MessageProperties object.
 *
 * This function sets a boolean property with the given name and value in the provided z_bmqa_MessageProperties object.
 * 
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object.
 * @param name The name of the property to be set.
 * @param value The boolean value to set for the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsBool(
    z_bmqa_MessageProperties* properties_obj,
    const char*               name,
    bool                      value);

/**
 * @brief Sets a property with a char value in a bmqa::MessageProperties object.
 *
 * This function sets a property named 'name' with the provided 'value' in the bmqa::MessageProperties object specified by 'properties_obj'.
 * 
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property to set.
 * @param value The value to set for the property.
 * @return Returns 0 upon successful property setting.
 */
int z_bmqa_MessageProperties__setPropertyAsChar(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    char value);
/**
 * @brief Sets a property with a short value in a bmqa::MessageProperties object.
 *
 * This function sets the property specified by the given name in the bmqa::MessageProperties object
 * with the provided short value.
 * 
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property to set.
 * @param value The short value to set for the property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsShort(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    short value);

/**
 * @brief Sets the value of a property in a bmqa::MessageProperties object to an int32_t value.
 * 
 * This function sets the value of the property with the given name in the provided bmqa::MessageProperties object
 * to the specified int32_t value.
 * 
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property to set.
 * @param value The int32_t value to set for the property.
 * @return Returns 0 upon successful completion.
 */
int z_bmqa_MessageProperties__setPropertyAsInt32(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    int32_t value);

/**
 * @brief Sets the value of a property in a bmqa::MessageProperties object to a specified 64-bit integer.
 *
 * This function sets the value of the property specified by 'name' in the bmqa::MessageProperties object pointed to by 'properties_obj' to the specified 64-bit integer 'value'.
 *
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property whose value is to be set.
 * @param value The 64-bit integer value to set for the specified property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsInt64(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    long long value);

/**
 * @brief Sets the value of a property in a bmqa::MessageProperties object to a specified string.
 *
 * This function sets the value of the property specified by 'name' in the bmqa::MessageProperties object pointed to by 'properties_obj' to the specified string 'value'.
 *
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property whose value is to be set.
 * @param value The string value to set for the specified property.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsString(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    char* value);

/**
 * @brief Sets the value of a property in a bmqa::MessageProperties object to a specified binary data.
 *
 * This function sets the value of the property specified by 'name' in the bmqa::MessageProperties object pointed to by 'properties_obj' to the binary data specified by 'value' with the specified size.
 *
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property whose value is to be set.
 * @param value A pointer to the binary data to set for the specified property.
 * @param size The size of the binary data pointed to by 'value'.
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsBinary(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value, 
    int size);

/**
 * @brief Retrieves the number of properties in a bmqa::MessageProperties object.
 *
 * This function retrieves the number of properties stored in the provided bmqa::MessageProperties object.
 * 
 * @param properties_obj A pointer to a const bmqa::MessageProperties object.
 *                       This object contains the properties whose count needs to be retrieved.
 * @return Returns the number of properties stored in the bmqa::MessageProperties object.
 */
int z_bmqa_MessageProperties__numProperties(
    const z_bmqa_MessageProperties* properties_obj);

/**
 * @brief Calculates the total size of a bmqa::MessageProperties object.
 *
 * This function calculates the total size of the given bmqa::MessageProperties object.
 * 
 * @param properties_obj A pointer to a const bmqa::MessageProperties object.
 * @return Returns the total size of the bmqa::MessageProperties object.
 */
int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj);

/**
 * @brief Retrieves a boolean property from a bmqa::MessageProperties object.
 *
 * This function retrieves a boolean property from a bmqa::MessageProperties object
 * based on the provided name.
 * 
 * @param properties_obj A pointer to a bmqa::MessageProperties object from which
 *                       the property will be retrieved.
 * @param name The name of the property to retrieve.
 * @return Returns the boolean value of the property.
 */
bool z_bmqa_MessageProperties__getPropertyAsBool(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves the value of a property as a character.
 *
 * This function retrieves the value of a specified property from the given
 * 'properties_obj' and returns it as a character.
 *
 * @param properties_obj A pointer to a const z_bmqa_MessageProperties object
 *                       from which the property value will be retrieved.
 * @param name The name of the property whose value is to be retrieved.
 * 
 * @return The value of the specified property as a character.
 */
char z_bmqa_MessageProperties__getPropertyAsChar(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property value as a short integer from a bmqa::MessageProperties object.
 * 
 * This function retrieves the value of the specified property as a short integer from the provided bmqa::MessageProperties object.
 * 
 * @param properties_obj A pointer to a const bmqa::MessageProperties object from which the property value will be retrieved.
 * @param name The name of the property whose value is to be retrieved.
 * @return The value of the specified property as a short integer.
 */
short z_bmqa_MessageProperties__getPropertyAsShort(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves the value associated with the given property name as an int32_t.
 *
 * This function retrieves the value associated with the specified property name from the provided
 * bmqa::MessageProperties object and returns it as an int32_t.
 * 
 * @param properties_obj A pointer to a constant bmqa::MessageProperties object from which to retrieve the property value.
 * @param name The name of the property whose value is to be retrieved.
 * @return Returns the value associated with the specified property name as an int32_t.
 */
int32_t z_bmqa_MessageProperties__getPropertyAsInt32(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves the value of a specified property as a 64-bit integer from a given MessageProperties object.
 *
 * This function retrieves the value of the property identified by the given name from the provided MessageProperties object 
 * and returns it as a 64-bit integer.
 * 
 * @param properties_obj A pointer to a const z_bmqa_MessageProperties object from which the property value will be retrieved.
 * @param name A pointer to a null-terminated string representing the name of the property whose value is to be retrieved.
 * @return Returns the value of the specified property as a 64-bit integer.
 */
long long z_bmqa_MessageProperties__getPropertyAsInt64(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property value as a C-style string from a bmqa::MessageProperties object.
 * 
 * This function retrieves the value of a property specified by the input name as a C-style string from the provided bmqa::MessageProperties object.
 * @param properties_obj A pointer to a const bmqa::MessageProperties object from which to retrieve the property value.
 * @param name The name of the property whose value is to be retrieved.
 * @return Returns a C-style string representing the value of the specified property.
 */
const char* z_bmqa_MessageProperties__getPropertyAsString(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves a property from a bmqa::MessageProperties object as a binary string.
 *
 * This function retrieves the specified property from the provided bmqa::MessageProperties object as a binary string.
 * The returned binary string is represented as a null-terminated character array, which is dynamically allocated.
 * It is the caller's responsibility to free the allocated memory using free() when it is no longer needed.
 *
 * @param properties_obj A pointer to a const z_bmqa_MessageProperties object from which the property will be retrieved.
 * @param name The name of the property to retrieve.
 * @return Returns a null-terminated character array representing the binary string of the property.
 *         The caller is responsible for freeing this memory using free().
 */
const char* z_bmqa_MessageProperties__getPropertyAsBinary(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

/**
 * @brief Retrieves the value associated with a given property name as a boolean, or returns a default value if not found.
 *
 * This function retrieves the boolean value associated with the specified property name from the provided
 * 'properties_obj'. If the property is not found, it returns the provided default value.
 * 
 * @param properties_obj A pointer to a constant z_bmqa_MessageProperties object containing the properties.
 * @param name The name of the property to retrieve.
 * @param value The default boolean value to return if the property is not found.
 * @return Returns the boolean value associated with the specified property name if found, 
 *         otherwise returns the provided default value.
 */
bool z_bmqa_MessageProperties__getPropertyAsBoolOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    bool value); 

/**
 * @brief Retrieves the value of a property as a char or returns a default value if the property is not found.
 * 
 * This function retrieves the value of a property specified by the given name as a char from the provided bmqa::MessageProperties object.
 * If the property is not found, it returns the default value specified by the 'value' parameter.
 * 
 * @param properties_obj A pointer to a constant bmqa::MessageProperties object from which to retrieve the property value.
 * @param name The name of the property to retrieve.
 * @param value The default value to return if the property is not found.
 * @return Returns the value of the property specified by 'name' if found, otherwise returns the default value specified by 'value'.
 */
char z_bmqa_MessageProperties__getPropertyAsCharOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    char value);

/**
 * @brief Retrieves the value of a property with the specified name as a short integer, or returns a default value if the property is not found.
 * 
 * This function retrieves the value of a property with the specified name from the provided 'properties_obj' and returns it as a short integer. 
 * If the property is not found, the function returns the provided default value. 
 * 
 * @param properties_obj A pointer to a const z_bmqa_MessageProperties object representing the message properties.
 * @param name A constant reference to a bsl::string object specifying the name of the property to retrieve.
 * @param value The default short integer value to return if the specified property is not found.
 * @return Returns the value of the property with the specified name if found, otherwise returns the provided default value.
 */
char z_bmqa_MessageProperties__getPropertyAsShortOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    short value);

/**
 * @brief Retrieves the value associated with a given property name as an int32_t from a bmqa::MessageProperties object, 
 *        or returns a default value if the property does not exist.
 * 
 * This function retrieves the value associated with the specified property name from the provided bmqa::MessageProperties object. 
 * If the property exists and is of type int32_t, its value is returned. If the property does not exist, the default value specified 
 * by the 'value' parameter is returned instead.
 * 
 * @param properties_obj A pointer to a const z_bmqa_MessageProperties object from which to retrieve the property value.
 * @param name The name of the property to retrieve.
 * @param value The default value to return if the property does not exist or is not of type int32_t.
 * @return Returns the value associated with the specified property name as an int32_t if the property exists and is of type int32_t,
 *         otherwise returns the default value provided by the 'value' parameter.
 */
int32_t z_bmqa_MessageProperties__getPropertyAsInt32Or(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    int32_t value);

/**
 * @brief Retrieves the value associated with a specified property name as a 64-bit integer, or returns a default value if the property does not exist.
 *
 * This function retrieves the value associated with the specified property name from the given bmqa::MessageProperties object.
 * If the property does not exist, the function returns the provided default value.
 * 
 * @param properties_obj A pointer to a const z_bmqa_MessageProperties object.
 * @param name The name of the property whose value is to be retrieved.
 * @param value The default value to return if the property does not exist.
 * @return Returns the value associated with the specified property name if found, otherwise returns the provided default value.
 */
long long z_bmqa_MessageProperties__getPropertyAsInt64Or(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    long long value);

/**
 * @brief Retrieves the property value as a string or returns a default value if not found.
 *
 * This function retrieves the property value associated with the given name from the provided bmqa::MessageProperties object.
 * If the property is not found, it returns the default value provided.
 *
 * @param properties_obj A pointer to a bmqa::MessageProperties object.
 * @param name The name of the property whose value is to be retrieved.
 * @param value The default value to return if the property is not found.
 * @return Returns a pointer to a C-style string containing the property value. 
 *         If the property is not found, it returns the default value as a C-style string.
 */
const char* z_bmqa_MessageProperties__getPropertyAsStringOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value); 

/**
 * @brief Retrieves the property value as a binary vector or a default value if the property is not found.
 *
 * This function retrieves the value of a property identified by its name as a binary vector from the given message properties object. 
 * If the property is not found, it returns the specified default value. Memory is allocated for the resulting binary vector, and the caller
 * is responsible for freeing it.
 * 
 * @param properties_obj A pointer to a z_bmqa_MessageProperties object from which to retrieve the property.
 * @param name The name of the property to retrieve.
 * @param value A pointer to the default value to be returned if the property is not found.
 * @param size The size of the default value buffer.
 * @return Returns a pointer to a dynamically allocated buffer containing the binary representation of the property value or the default value.
 *         The caller is responsible for freeing this memory using free().
 */
const char* z_bmqa_MessageProperties__getPropertyAsBinaryOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value, 
    int size); 

#if defined(__cplusplus)
}
#endif

#endif