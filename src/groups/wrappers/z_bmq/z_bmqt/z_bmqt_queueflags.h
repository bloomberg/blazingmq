#ifndef INCLUDED_Z_BMQT_z_bmqt_QUEUEFLAGS
#define INCLUDED_Z_BMQT_z_bmqt_QUEUEFLAGS

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>

/// This enum represents queue flags
struct z_bmqt_QueueFlags {
    // TYPES
    enum Enum {
        ec_ADMIN = (1 << 0)  // The queue is opened in admin mode (Valid only
                             // for BlazingMQ admin tasks)
        ,
        ec_READ = (1 << 1)  // The queue is opened for consuming messages
        ,
        ec_WRITE = (1 << 2)  // The queue is opened for posting messages
        ,
        ec_ACK = (1 << 3)  // Set to indicate interested in receiving
                           // 'ACK' events for all message posted
    };

    // PUBLIC CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// z_bmqt_QueueFlags field is a supported type.
    static const int k_LOWEST_SUPPORTED_QUEUEc_FLAG = ec_ADMIN;

    /// NOTE: This value must always be equal to the highest *supported*
    /// type in the enum because it is being used to verify a z_bmqt_QueueFlags
    /// field is a supported type.
    static const int k_HIGHEST_SUPPORTED_QUEUEc_FLAG = ec_ACK;

    // CLASS METHODS

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the `ec_` prefix elided.  Note
    /// that specifying a `value` that does not match any of the enumerators
    /// will result in a string representation that is distinct from any of
    /// those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(z_bmqt_QueueFlags::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(z_bmqt_QueueFlags::Enum* out, const char* str);
};

#if defined(__cplusplus)
}
#endif

#endif