#ifndef INCLUDED_Z_BMQA_EVENT
#define INCLUDED_Z_BMQA_EVENT

#if defined(__cplusplus)
extern "C" {
#endif

#include <z_bmqa_sessionevent.h>

typedef struct z_bmqa_Event z_bmqa_Event;

int z_bmqa_Event__create(z_bmqa_Event** event_obj);

int z_bmqa_Event__SessionEvent(z_bmqa_Event* event_obj);

#if defined(__cplusplus)
}
#endif

#endif