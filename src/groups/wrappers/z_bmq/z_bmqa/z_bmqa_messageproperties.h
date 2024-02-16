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

int z_bmqa_MessageProperties__delete(
    z_bmqa_MessageProperties** properties_obj);

int z_bmqa_MessageProperties__create(
    z_bmqa_MessageProperties** properties_obj);

int z_bmqa_MessageProperties__createCopy(
    z_bmqa_MessageProperties**      properties_obj,
    const z_bmqa_MessageProperties* other);

int z_bmqa_MessageProperties__(z_bmqa_MessageProperties* properties_obj);

int z_bmqa_MessageProperties__(z_bmqa_MessageProperties* properties_obj);

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
    const char* value); 

#if defined(__cplusplus)
}
#endif

#endif