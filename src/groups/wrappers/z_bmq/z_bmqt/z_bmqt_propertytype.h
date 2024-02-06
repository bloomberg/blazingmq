#ifndef INCLUDED_Z_BMQT_PROPERTYTYPE
#define INCLUDED_Z_BMQT_PROPERTYTYPE

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>

struct z_bmqt_PropertyType {
    // TYPES
    enum Enum {
        ec_UNDEFINED = 0,
        ec_BOOL      = 1,
        ec_CHAR      = 2,
        ec_SHORT     = 3,
        ec_INT32     = 4,
        ec_INT64     = 5,
        ec_STRING    = 6,
        ec_BINARY    = 7
    };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// Property's `type` field is a supported type.
    static const int k_LOWEST_SUPPORTED_PROPERTY_TYPE = ec_BOOL;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify a
    /// Property's `type` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_PROPERTY_TYPE = ec_BINARY;

    // CLASS METHODS

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `ec_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(z_bmqt_PropertyType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(z_bmqt_PropertyType::Enum* out, const char* str);
};

#if defined(__cplusplus)
}
#endif

#endif