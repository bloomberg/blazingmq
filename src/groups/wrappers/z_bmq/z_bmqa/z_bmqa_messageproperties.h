```cpp
#ifndef INCLUDED_Z_BMQA_MESSAGEPROPERTIES
#define INCLUDED_Z_BMQA_MESSAGEPROPERTIES

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>

typedef struct z_bmqa_MessageProperties z_bmqa_MessageProperties;

/**
 * @brief Deletes a message properties object.
 * 
 * This function deallocates memory for the message properties object.
 * 
 * @param properties_obj A pointer to a pointer to the z_bmqa_MessageProperties object to be deleted.
 * 
 * @return Returns 0 upon successful deletion.
 */
int z_bmqa_MessageProperties__delete(
    z_bmqa_MessageProperties** properties_obj);

/**
 * @brief Creates a new message properties object.
 * 
 * This function creates a new message properties object.
 * 
 * @param properties_obj A pointer to a pointer to the z_bmqa_MessageProperties object to be created.
 * 
 * @return Returns 0 upon successful creation.
 */
int z_bmqa_MessageProperties__create(
    z_bmqa_MessageProperties** properties_obj);

/**
 * @brief Creates a copy of a message properties object.
 * 
 * This function creates a copy of an existing message properties object.
 * 
 * @param properties_obj A pointer to a pointer to the z_bmqa_MessageProperties object where the copy will be stored.
 * @param other          A pointer to the const z_bmqa_MessageProperties object to be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqa_MessageProperties__createCopy(
    z_bmqa_MessageProperties**      properties_obj,
    const z_bmqa_MessageProperties* other);

/**
 * @brief Sets a property with a boolean value.
 * 
 * This function sets a property in the message properties object with a boolean value.
 * 
 * @param properties_obj A pointer to the z_bmqa_MessageProperties object.
 * @param name           The name of the property.
 * @param value          The boolean value of the property.
 * 
 * @return Returns 0 upon successful setting of the property.
 */
int z_bmqa_MessageProperties__setPropertyAsBool(
    z_bmqa_MessageProperties* properties_obj,
    const char*               name,
    bool                      value);

/**
 * @brief Retrieves the total size of the message properties.
 * 
 * This function retrieves the total size of the message properties.
 * 
 * @param properties_obj A pointer to the const z_bmqa_MessageProperties object.
 * 
 * @return Returns the total size of the message properties.
 */
int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj);

#if defined(__cplusplus)
}
#endif

#endif
```
