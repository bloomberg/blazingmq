#ifndef INCLUDED_Z_BMQA_CLOSEQUEUESTATUS
#define INCLUDED_Z_BMQA_CLOSEQUEUESTATUS

#include <z_bmqa_queueid.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_CloseQueueStatus z_bmqa_CloseQueueStatus;

int z_bmqa_CloseQueueStatus__create(z_bmqa_CloseQueueStatus** status_obj);

int z_bmqa_CloseQueueStatus__createCopy(z_bmqa_CloseQueueStatus** status_obj, const z_bmqa_CloseQueueStatus* other);

int z_bmqa_CloseQueueStatus__createFull(z_bmqa_CloseQueueStatus** status_obj,
                                       const z_bmqa_QueueId* queueId,
                                       int result,
                                       const char* errorDescription);

bool  z_bmqa_CloseQueueStatus__toBool(const z_bmqa_CloseQueueStatus* status_obj);

int z_bmqa_CloseQueueStatus__queueId(const z_bmqa_CloseQueueStatus* status_obj, z_bmqa_QueueId const** queueId_obj);

int z_bmqa_CloseQueueStatus__result(const z_bmqa_CloseQueueStatus* status_obj);

const char* z_bmqa_CloseQueueStatus__errorDescription(const z_bmqa_CloseQueueStatus* status_obj);



#if defined(__cplusplus)
}
#endif

#endif