#ifndef INCLUDED_Z_BMQA_QUEUEID
#define INCLUDED_Z_BMQA_QUEUEID

#include <z_bmqt_correlationid.h>
#include <z_bmqt_queueoptions.h>
#include <z_bmqt_uri.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_QueueId z_bmqa_QueueId;

int z_bmqa_QueueId__delete(z_bmqa_QueueId** queueId_obj);

int z_bmqa_QueueId__deleteConst(z_bmqa_QueueId const** queueId_obj);

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj);

int z_bmqa_QueueId__createCopy(z_bmqa_QueueId** queueId_obj, const z_bmqa_QueueId* other);

int z_bmqa_QueueId__createFromCorrelationId(z_bmqa_QueueId** queueId_obj, const z_bmqt_CorrelationId* correlationId);

int z_bmqa_QueueId__createFromNumeric(z_bmqa_QueueId** queueId_obj, int64_t numeric);

int z_bmqa_QueueId__createFromPointer(z_bmqa_QueueId** queueId_obj, void* pointer);

const z_bmqt_CorrelationId* z_bmqa_QueueId__correlationId(const z_bmqa_QueueId* queueId_obj);

uint64_t z_bmqa_QueueId__flags(const z_bmqa_QueueId* queueId_obj);

const z_bmqt_Uri* z_bmqa_QueueId__uri(const z_bmqa_QueueId* queueId_obj);

const z_bmqt_QueueOptions* options(const z_bmqa_QueueId* queueId_obj);

int z_bmqa_QueueId__isValid(const z_bmqa_QueueId* queueId_obj);

#if defined(__cplusplus)
}
#endif

#endif