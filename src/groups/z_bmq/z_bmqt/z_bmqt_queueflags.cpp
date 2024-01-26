#include <bmqt_queueflags.h>
#include <z_bmqt_queueflags.h>

const char* z_bmqt_QueueFlags::toAscii(z_bmqt_QueueFlags::Enum value)
{
    using namespace BloombergLP;

    return bmqt::QueueFlags::toAscii(static_cast<bmqt::QueueFlags::Enum>(value));
}

bool z_bmqt_QueueFlags::fromAscii(z_bmqt_QueueFlags::Enum* out,
                                  const char*              str)
{
    using namespace BloombergLP;
    bmqt::QueueFlags::Enum p;
    bool result = bmqt::QueueFlags::fromAscii(&p, str);
    *out = static_cast<z_bmqt_QueueFlags::Enum>(p);
    return result;
}