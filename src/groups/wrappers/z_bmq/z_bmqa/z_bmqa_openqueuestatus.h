#ifndef INCLUDED_Z_BMQA_OPENQUEUESTATUS
#define INCLUDED_Z_BMQA_OPENQUEUESTATUS

#include <stdbool.h>
#include <z_bmqa_queueid.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_OpenQueueStatus z_bmqa_OpenQueueStatus;

int z_bmqa_OpenQueueStatus__delete(z_bmqa_OpenQueueStatus** status_obj);

int z_bmqa_OpenQueueStatus__create(z_bmqa_OpenQueueStatus** status_obj);

int z_bmqa_OpenQueueStatus__createCopy(z_bmqa_OpenQueueStatus** status_obj,
                                       const z_bmqa_OpenQueueStatus* other);

int z_bmqa_OpenQueueStatus__createFull(z_bmqa_OpenQueueStatus** status_obj,
                                       const z_bmqa_QueueId*    queueId,
                                       int                      result,
                                       const char* errorDescription);

bool z_bmqa_OpenQueueStatus__toBool(const z_bmqa_OpenQueueStatus* status_obj);

int z_bmqa_OpenQueueStatus__queueId(const z_bmqa_OpenQueueStatus* status_obj,
                                    z_bmqa_QueueId const**        queueId_obj);

int z_bmqa_OpenQueueStatus__result(const z_bmqa_OpenQueueStatus* status_obj);

const char* z_bmqa_OpenQueueStatus__errorDescription(
    const z_bmqa_OpenQueueStatus* status_obj);

#if defined(__cplusplus)
}
#endif

#endif