#include <bmqt_messageeventtype.h>
#include <z_bmqt_messageeventtype.h>

const char*
z_bmqt_MessageEventType::toAscii(z_bmqt_MessageEventType::Enum value)
{
    using namespace BloombergLP;

    return bmqt::MessageEventType::toAscii(
        static_cast<bmqt::MessageEventType::Enum>(value));
}

bool z_bmqt_MessageEventType::fromAscii(z_bmqt_MessageEventType::Enum* out, const char* str)
{
    using namespace BloombergLP;
    bmqt::MessageEventType::Enum p;
    bool result = bmqt::MessageEventType::fromAscii(&p, str);
    *out        = static_cast<z_bmqt_MessageEventType::Enum>(p);
    return result;
}