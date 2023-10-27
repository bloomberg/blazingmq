#ifndef INCLUDED_Z_BMQA_CORRELATIONID
#define INCLUDED_Z_BMQA_CORRELATIONID

#include <stdint.h>

typedef struct z_bmqt_CorrelationId z_bmqt_CorrelationId;


enum CorrelationId_Type {
    e_NUMERIC  // the 'CorrelationId' holds a 64-bit integer
    ,
    e_POINTER  // the 'CorrelationId' holds a raw pointer
    ,
    e_SHARED_PTR  // the 'CorrelationId' holds a shared pointer
    ,
    e_AUTO_VALUE  // the 'CorrelationId' holds an auto value
    ,
    e_UNSET  // the 'CorrelationId' is not set
    ,
    e_ERROR // error
};

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj);

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj, int64_t numeric);

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj, void* pointer);

int z_bmqt_CorrelationId__makeUnset(z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__setNumeric(z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__setPointer(z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__isUnset(const z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__isNumeric(const z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__isPointer(const z_bmqt_CorrelationId* correlation_Id_obj);

int z_bmqt_CorrelationId__isSharedPtr(const z_bmqt_CorrelationId* correlationId_obj);

int z_bmqt_CorrelationId__isAutoValue(const z_bmqt_CorrelationId* correlation_Id_obj);

int64_t z_bmqt_CorrelationId__theNumeric(const z_bmqt_CorrelationId* correlation_Id_obj);

void* z_bmqt_CorrelationId__thePointer(const z_bmqt_CorrelationId* correlation_Id_obj);

CorrelationId_Type z_bmqt_CorrelationId__type(const z_bmqt_CorrelationId* correlation_Id_obj);

#endif