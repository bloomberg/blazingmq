#ifndef INCLUDED_Z_BMQA_SESSIONEVENT
#define INCLUDED_Z_BMQA_SESSIONEVENT

#include <z_bmqa_queueid.h>
#include <z_bmqt_correlationid.h>
#include <z_bmqt_sessioneventtype.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_SessionEvent z_bmqa_SessionEvent;

int z_bmqa_SessionEvent__delete(z_bmqa_SessionEvent** event_obj);

int z_bmqa_SessionEvent__create(z_bmqa_SessionEvent** event_obj);

int z_bmqa_SessionEvent__createCopy(z_bmqa_SessionEvent**      event_obj,
                                    const z_bmqa_SessionEvent* other);

z_bmqt_SessionEventType::Enum
z_bmqa_SessionEvent__type(const z_bmqa_SessionEvent* event_obj);

int z_bmqa_SessionEvent__correlationId(
    const z_bmqa_SessionEvent*   event_obj,
    z_bmqt_CorrelationId const** correlationId_obj);

// Generates NEW queueId
int z_bmqa_SessionEvent__queueId(const z_bmqa_SessionEvent* event_obj,
                                 z_bmqa_QueueId**           queueId_obj);

int z_bmqa_SessionEvent__statusCode(const z_bmqa_SessionEvent* event_obj);

const char*
z_bmqa_SessionEvent__errorDescription(const z_bmqa_SessionEvent* event_obj);

#if defined(__cplusplus)
}
#endif

#endif
