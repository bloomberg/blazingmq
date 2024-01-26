#ifndef INCLUDED_Z_BMQA_CONFIGUREQUEUESTATUS
#define INCLUDED_Z_BMQA_CONFIGUREQUEUESTATUS

#include <stdbool.h>
#include <z_bmqa_queueid.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_ConfigureQueueStatus z_bmqa_ConfigureQueueStatus;

int z_bmqa_ConfigureQueueStatus__delete(
    z_bmqa_ConfigureQueueStatus** status_obj);

int z_bmqa_ConfigureQueueStatus__create(
    z_bmqa_ConfigureQueueStatus** status_obj);

int z_bmqa_ConfigureQueueStatus__createCopy(
    z_bmqa_ConfigureQueueStatus**      status_obj,
    const z_bmqa_ConfigureQueueStatus* other);

int z_bmqa_ConfigureQueueStatus__createFull(
    z_bmqa_ConfigureQueueStatus** status_obj,
    const z_bmqa_QueueId*         queueId,
    int                           result,
    const char*                   errorDescription);

bool z_bmqa_ConfigureQueueStatus__toBool(
    const z_bmqa_ConfigureQueueStatus* status_obj);

int z_bmqa_ConfigureQueueStatus__queueId(
    const z_bmqa_ConfigureQueueStatus* status_obj,
    z_bmqa_QueueId const**             queueId_obj);

int z_bmqa_ConfigureQueueStatus__result(
    const z_bmqa_ConfigureQueueStatus* status_obj);

const char* z_bmqa_ConfigureQueueStatus__errorDescription(
    const z_bmqa_ConfigureQueueStatus* status_obj);

#if defined(__cplusplus)
}
#endif

#endif