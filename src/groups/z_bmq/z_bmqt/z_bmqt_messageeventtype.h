#ifndef INCLUDED_Z_BMQA_z_bmqt_MESSAGEEVENTTYPE
#define INCLUDED_Z_BMQA_MESSAGEEVENTTYPE

#if defined(__cplusplus)
extern "C" {
#endif

struct z_bmqt_MessageEventType {
    // TYPES
    enum Enum { ec_UNDEFINED = 0, ec_PUT = 1, ec_PUSH = 2, ec_ACK = 3 };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that an
    /// z_bmqt_MessageEventType field is a supported type.
    static const int k_LOWEST_SUPPORTED_EVENT_TYPE = ec_PUT;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify an
    /// z_bmqt_MessageEventType field is a supported type.
    static const int k_HIGHEST_SUPPORTED_EVENT_TYPE = ec_ACK;

    // CLASS METHODS
    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `BMQT_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(z_bmqt_MessageEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(z_bmqt_MessageEventType::Enum*  out,
                          const char* str);
};

#if defined(__cplusplus)
}
#endif

#endif