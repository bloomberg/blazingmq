#include <bmqt_propertytype.h>
#include <z_bmqt_propertytype.h>

const char* z_bmqt_PropertyType::toAscii(z_bmqt_PropertyType::Enum value)
{
    using namespace BloombergLP;

    return bmqt::PropertyType::toAscii(
        static_cast<bmqt::PropertyType::Enum>(value));
}

bool z_bmqt_PropertyType::fromAscii(z_bmqt_PropertyType::Enum* out,
                                    const char*                str)
{
    using namespace BloombergLP;

    bmqt::PropertyType::Enum type;
    bool                     result = bmqt::PropertyType::fromAscii(&type, str);
    *out = static_cast<z_bmqt_PropertyType::Enum>(type);

    return result;
}
