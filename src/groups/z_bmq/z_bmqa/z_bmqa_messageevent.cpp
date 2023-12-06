#include <z_bmqa_messageevent.h>
#include <bmqa_messageevent.h>

int z_bmqa_MessageEvent__delete(z_bmqa_MessageEvent** event_obj) {
    using namespace BloombergLP;

    bmqa::MessageEvent* event_p = reinterpret_cast<bmqa::MessageEvent*>(*event_obj);
    delete event_p;
    *event_obj = NULL;

    return 0;
}

int z_bmqa_MessageEvent__deleteConst(z_bmqa_MessageEvent const** event_obj) {
    using namespace BloombergLP;

    const bmqa::MessageEvent* event_p = reinterpret_cast<const bmqa::MessageEvent*>(*event_obj);
    delete event_p;
    *event_obj = NULL;

    return 0;
}