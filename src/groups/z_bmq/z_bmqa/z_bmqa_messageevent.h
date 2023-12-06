#ifndef INCLUDED_Z_BMQA_MESSAGEEVENT
#define INCLUDED_Z_BMQA_MESSAGEEVENT

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_MessageEvent z_bmqa_MessageEvent;

int z_bmqa_MessageEvent__delete(z_bmqa_MessageEvent** event_obj);

int z_bmqa_MessageEvent__deleteConst(z_bmqa_MessageEvent const** event_obj);

#if defined(__cplusplus)
}
#endif

#endif
