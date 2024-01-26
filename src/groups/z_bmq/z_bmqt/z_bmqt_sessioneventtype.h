#ifndef INCLUDED_Z_BMQT_SESSIONEVENTTYPE
#define INCLUDED_Z_BMQT_SESSIONEVENTTYPE

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct z_bmqt_SessionEventType {
    // TYPES
    enum Enum {
        ec_ERROR = -1  // Generic error
        ,
        ec_TIMEOUT = -2  // Time out of the operation
        ,
        ec_CANCELED = -3  // The operation was canceled
        ,
        ec_UNDEFINED = 0,
        ec_CONNECTED = 1  // Session started
        ,
        ec_DISCONNECTED = 2  // Session terminated
        ,
        ec_CONNECTION_LOST = 3  // Lost connection to the broker
        ,
        ec_RECONNECTED = 4  // Reconnected with the broker
        ,
        ec_STATE_RESTORED = 5  // Client's state has been restored
        ,
        ec_CONNECTION_TIMEOUT = 6  // The connection to broker timedOut
        ,
        ec_QUEUE_OPEN_RESULT = 7  // Result of openQueue operation
        ,
        ec_QUEUE_REOPEN_RESULT = 8  // Result of re-openQueue operation
        ,
        ec_QUEUE_CLOSE_RESULT = 9  // Result of closeQueue operation
        ,
        ec_SLOWCONSUMER_NORMAL = 10  // EventQueue is at lowWatermark
        ,
        ec_SLOWCONSUMER_HIGHWATERMARK = 11  // EventQueue is at highWatermark
        ,
        ec_QUEUE_CONFIGURE_RESULT = 12  // Result of configureQueue
        ,
        ec_HOST_UNHEALTHY = 13  // Host has become unhealthy
        ,
        ec_HOST_HEALTH_RESTORED = 14  // Host's health has been restored
        ,
        ec_QUEUE_SUSPENDED = 15  // Queue has suspended operation
        ,
        ec_QUEUE_RESUMED = 16  // Queue has resumed operation
    };

    static const char* toAscii(z_bmqt_SessionEventType::Enum value);

    /// Return true and fills the specified `out` with the enum value
    /// corresponding to the specified `str`, if valid, or return false and
    /// leave `out` untouched if `str` doesn't correspond to any value of
    /// the enum.
    static bool fromAscii(z_bmqt_SessionEventType::Enum* out, const char* str);
};

#if defined(__cplusplus)
}
#endif

#endif