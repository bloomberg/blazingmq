#include <bmqa_sessionevent.h>
#include <z_bmqa_sessionevent.h>

int z_bmqa_SessionEvent__delete(z_bmqa_SessionEvent** sessionEvent)
{
    using namespace BloombergLP;

    BSLS_ASSERT(sessionEvent != NULL);

    bmqa::SessionEvent* event_p = reinterpret_cast<bmqa::SessionEvent*>(
        *sessionEvent);
    delete event_p;
    *sessionEvent = NULL;

    return 0;
}

int z_bmqa_SessionEvent__create(z_bmqa_SessionEvent** sessionEvent)
{
    using namespace BloombergLP;

    bmqa::SessionEvent* event_p = new bmqa::SessionEvent();
    *sessionEvent = reinterpret_cast<z_bmqa_SessionEvent*>(event_p);

    return 0;
}

int z_bmqa_SessionEvent__createCopy(z_bmqa_SessionEvent**      sessionEvent,
                                    const z_bmqa_SessionEvent* other)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* other_p =
        reinterpret_cast<const bmqa::SessionEvent*>(other);
    bmqa::SessionEvent* event_p = new bmqa::SessionEvent(*other_p);
    *sessionEvent = reinterpret_cast<z_bmqa_SessionEvent*>(event_p);

    return 0;
}

z_bmqt_SessionEventType::Enum
z_bmqa_SessionEvent__type(const z_bmqa_SessionEvent* sessionEvent)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* event_p =
        reinterpret_cast<const bmqa::SessionEvent*>(sessionEvent);
    return static_cast<z_bmqt_SessionEventType::Enum>(event_p->type());
}

int z_bmqa_SessionEvent__correlationId(
    const z_bmqa_SessionEvent*   sessionEvent,
    z_bmqt_CorrelationId const** correlationId_obj)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* event_p =
        reinterpret_cast<const bmqa::SessionEvent*>(sessionEvent);
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(
            &(event_p->correlationId()));
    *correlationId_obj = reinterpret_cast<const z_bmqt_CorrelationId*>(
        correlationId_p);

    return 0;
}

int z_bmqa_SessionEvent__queueId(const z_bmqa_SessionEvent* sessionEvent,
                                 z_bmqa_QueueId**           queueId_obj)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* event_p =
        reinterpret_cast<const bmqa::SessionEvent*>(sessionEvent);
    bmqa::QueueId* queueId_p = new bmqa::QueueId(event_p->queueId());
    *queueId_obj             = reinterpret_cast<z_bmqa_QueueId*>(queueId_p);

    return 0;
}

int z_bmqa_SessionEvent__statusCode(const z_bmqa_SessionEvent* sessionEvent)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* event_p =
        reinterpret_cast<const bmqa::SessionEvent*>(sessionEvent);
    return event_p->statusCode();
}

const char*
z_bmqa_SessionEvent__errorDescription(const z_bmqa_SessionEvent* sessionEvent)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* event_p =
        reinterpret_cast<const bmqa::SessionEvent*>(sessionEvent);
    return event_p->errorDescription().c_str();
}

int z_bmqa_SessionEvent__toString(const z_bmqa_SessionEvent* sessionEvent,
                                  char**                     out)
{
    using namespace BloombergLP;

    const bmqa::SessionEvent* event_p =
        reinterpret_cast<const bmqa::SessionEvent*>(sessionEvent);
    bsl::ostringstream ss;
    ss << *event_p;
    bsl::string out_str = ss.str();

    *out = new char[out_str.length() + 1];
    strcpy(*out, out_str.c_str());

    return 0;
}