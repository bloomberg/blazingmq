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

int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj);

#if defined(__cplusplus)
}
#endif

#endif