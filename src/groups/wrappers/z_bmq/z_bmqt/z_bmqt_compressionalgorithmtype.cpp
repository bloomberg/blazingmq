#include <bmqt_compressionalgorithmtype.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <string.h>

#include <z_bmqt_compressionalgorithmtype.h>

const char* z_bmqt_CompressionAlgorithmType::toAscii(
    z_bmqt_CompressionAlgorithmType::Enum value)
{
    using namespace BloombergLP;

    return bmqt::CompressionAlgorithmType::toAscii(
        static_cast<bmqt::CompressionAlgorithmType::Enum>(value));
}

bool fromAscii(z_bmqt_CompressionAlgorithmType::Enum* out, const char* str)
{
    using namespace BloombergLP;
    bmqt::CompressionAlgorithmType::Enum type;
    bool result = bmqt::CompressionAlgorithmType::fromAscii(&type, str);
    *out        = static_cast<z_bmqt_CompressionAlgorithmType::Enum>(type);
    return result;
}

bool isValid(const char* str, char** error)
{
    using namespace BloombergLP;
    bsl::ostringstream os;
    bsl::string        temp_str(str);
    bool result = bmqt::CompressionAlgorithmType::isValid(&temp_str, os);

    bsl::string error_str = os.str();

    if (error_str.size() != 0) {
        *error                     = new char[error_str.size() + 1];
        (*error)[error_str.size()] = '\0';
        strncpy(*error, error_str.c_str());
    }
    else {
        *error = NULL;
    }

    return result;
}