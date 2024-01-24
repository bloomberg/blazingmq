#ifndef INCLUDED_Z_BMQA_MESSAGEEVENT
#define INCLUDED_Z_BMQA_MESSAGEEVENT

#include <z_bmqa_messageiterator.h>
#include <z_bmqt_messageeventtype.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_MessageEvent z_bmqa_MessageEvent;

int z_bmqa_MessageEvent__delete(z_bmqa_MessageEvent** event_obj);

int z_bmqa_MessageEvent__create(z_bmqa_MessageEvent** event_obj);

int z_bmqa_MessageEvent__messageIterator(
    const z_bmqa_MessageEvent* event_obj,
    z_bmqa_MessageIterator**   iterator_obj);

z_bmqt_MessageEventType::Enum z_bmqa_MessageEvent__type(const z_bmqa_MessageEvent* event_obj);

int z_bmqa_MessageEvent__toString(const z_bmqa_MessageEvent* event_obj,
                                  char**                     out);

#if defined(__cplusplus)
}
#endif

#endif
