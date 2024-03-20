```cpp
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

void z_bmqa_MessageProperties__clear(
    z_bmqa_MessageProperties* properties_obj);

bool z_bmqa_MessageProperties__remove(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    Enum *buffer);

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

int z_bmqa_MessageProperties__setPropertyAsChar(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    char value);

int z_bmqa_MessageProperties__setPropertyAsShort(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    short value);

int z_bmqa_MessageProperties__setPropertyAsInt32(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    int32_t value);

int z_bmqa_MessageProperties__setPropertyAsInt64(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    long long value);

int z_bmqa_MessageProperties__setPropertyAsString(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    char* value);

int z_bmqa_MessageProperties__setPropertyAsBinary(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value, 
    int size);

int z_bmqa_MessageProperties__numProperties(
    const z_bmqa_MessageProperties* properties_obj);

int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj);

bool z_bmqa_MessageProperties__getPropertyAsBool(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

char z_bmqa_MessageProperties__getPropertyAsChar(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

short z_bmqa_MessageProperties__getPropertyAsShort(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

int32_t z_bmqa_MessageProperties__getPropertyAsInt32(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

long long z_bmqa_MessageProperties__getPropertyAsInt64(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

const char* z_bmqa_MessageProperties__getPropertyAsString(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

const char* z_bmqa_MessageProperties__getPropertyAsBinary(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name); 

bool z_bmqa_MessageProperties__getPropertyAsBoolOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    bool value); 

char z_bmqa_MessageProperties__getPropertyAsCharOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    char value);

char z_bmqa_MessageProperties__getPropertyAsShortOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    short value);

int32_t z_bmqa_MessageProperties__getPropertyAsInt32Or(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    int32_t value);

long long z_bmqa_MessageProperties__getPropertyAsInt64Or(
    const z_bmqa_MessageProperties* properties_obj, 
    const bsl::string& name, 
    long long value);

const char* z_bmqa_MessageProperties__getPropertyAsStringOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value); 

const char* z_bmqa_MessageProperties__getPropertyAsBinaryOr(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value, 
    int size); 

#if defined(__cplusplus)
}
#endif

#endif
```
