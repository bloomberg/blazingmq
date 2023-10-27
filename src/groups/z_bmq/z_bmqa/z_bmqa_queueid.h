#ifndef INCLUDED_Z_BMQA_QUEUEID
#define INCLUDED_Z_BMQA_QUEUEID

#include <z_bmqt_correlationid.h>
#include <stdint.h>

typedef struct z_bmqa_QueueId z_bmqa_QueueId;

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj);

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, const z_bmqt_CorrelationId* correlationId);

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, const z_bmqa_QueueId* other);

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, int64_t numeric);

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, void* pointer);

int i;



#endif