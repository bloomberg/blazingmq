#include <bmqa_messageevent.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <z_bmqa_messageevent.h>

int z_bmqa_MessageEvent__delete(z_bmqa_MessageEvent** event_obj)
{
    using namespace BloombergLP;

    bmqa::MessageEvent* event_p = reinterpret_cast<bmqa::MessageEvent*>(
        *event_obj);
    delete event_p;
    *event_obj = NULL;

    return 0;
}

int z_bmqa_MessageEvent__create(z_bmqa_MessageEvent** event_obj)
{
    using namespace BloombergLP;

    bmqa::MessageEvent* event_p = new bmqa::MessageEvent();
    *event_obj = reinterpret_cast<z_bmqa_MessageEvent*>(event_p);

    return 0;
}

int z_bmqa_MessageEvent__messageIterator(const z_bmqa_MessageEvent* event_obj,
                                         z_bmqa_MessageIterator** iterator_obj)
{
    using namespace BloombergLP;
    const bmqa::MessageEvent* event_p =
        reinterpret_cast<const bmqa::MessageEvent*>(event_obj);
    bmqa::MessageIterator* iterator_p = new bmqa::MessageIterator(
        event_p->messageIterator());
    *iterator_obj = reinterpret_cast<z_bmqa_MessageIterator*>(iterator_p);

    return 0;
}

z_bmqt_MessageEventType::Enum
z_bmqa_MessageEvent__type(const z_bmqa_MessageEvent* event_obj)
{
    using namespace BloombergLP;

    const bmqa::MessageEvent* event_p =
        reinterpret_cast<const bmqa::MessageEvent*>(event_obj);
    return static_cast<z_bmqt_MessageEventType::Enum>(event_p->type());
}

int z_bmqa_MessageEvent__toString(const z_bmqa_MessageEvent* event_obj,
                                  char**                     out)
{
    using namespace BloombergLP;

    const bmqa::MessageEvent* event_p =
        reinterpret_cast<const bmqa::MessageEvent*>(event_obj);
    bsl::ostringstream ss;
    event_p->print(ss);
    bsl::string out_str = ss.str();

    *out                  = new char[out_str.length() + 1];
    (*out)[out_str.length()] = '\0';
    strcpy(*out, out_str.c_str());

    return 0;
}