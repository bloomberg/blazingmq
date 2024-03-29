#include <bmqt_sessioneventtype.h>
#include <z_bmqt_sessioneventtype.h>

const char*
z_bmqt_SessionEventType::toAscii(z_bmqt_SessionEventType::Enum value)
{
    using namespace BloombergLP;

    return bmqt::SessionEventType::toAscii(
        static_cast<bmqt::SessionEventType::Enum>(value));
}

bool z_bmqt_SessionEventType::fromAscii(z_bmqt_SessionEventType::Enum* out,
                                        const char*                    str)
{
    using namespace BloombergLP;
    bmqt::SessionEventType::Enum p;
    bool result = bmqt::SessionEventType::fromAscii(&p, str);
    *out        = static_cast<z_bmqt_SessionEventType::Enum>(p);
    return result;
}