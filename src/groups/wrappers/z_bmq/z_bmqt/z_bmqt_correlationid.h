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

int z_bmqt_CorrelationId__delete(z_bmqt_CorrelationId** correlationId_obj);

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj);

int z_bmqt_CorrelationId__createCopy(z_bmqt_CorrelationId** correlationId_obj,
                                     const z_bmqt_CorrelationId* other_obj);

int z_bmqt_CorrelationId__createFromNumeric(
    z_bmqt_CorrelationId** correlationId_obj,
    int64_t                numeric);

int z_bmqt_CorrelationId__createFromPointer(
    z_bmqt_CorrelationId** correlationId_obj,
    void*                  pointer);

int z_bmqt_CorrelationId__makeUnset(z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__setNumeric(z_bmqt_CorrelationId* correlation_Id_obj,
                                     int64_t               numeric);

int z_bmqt_CorrelationId__setPointer(z_bmqt_CorrelationId* correlation_Id_obj,
                                     void*                 pointer);

bool z_bmqt_CorrelationId__isUnset(
    const z_bmqt_CorrelationId* correlation_Id_obj);

bool z_bmqt_CorrelationId__isNumeric(
    const z_bmqt_CorrelationId* correlation_Id_obj);

bool z_bmqt_CorrelationId__isPointer(
    const z_bmqt_CorrelationId* correlation_Id_obj);

bool z_bmqt_CorrelationId__isAutoValue(
    const z_bmqt_CorrelationId* correlation_Id_obj);

int64_t z_bmqt_CorrelationId__theNumeric(
    const z_bmqt_CorrelationId* correlation_Id_obj);

void* z_bmqt_CorrelationId__thePointer(
    const z_bmqt_CorrelationId* correlation_Id_obj);

z_bmqt_CorrelationId::Type
z_bmqt_CorrelationId__type(const z_bmqt_CorrelationId* correlationId_obj);

int z_bmqt_CorrelationId__autoValue(z_bmqt_CorrelationId** correlationId_obj);

int z_bmqt_CorrelationId__compare(const z_bmqt_CorrelationId* a,
                                  const z_bmqt_CorrelationId* b);

int z_bmqt_CorrelationId__assign(z_bmqt_CorrelationId**      dst,
                                 const z_bmqt_CorrelationId* src);

int z_bmqt_CorrelationId__toString(
    const z_bmqt_CorrelationId* correlationId_obj,
    char**                      out);

#if defined(__cplusplus)
}
#endif

#endif