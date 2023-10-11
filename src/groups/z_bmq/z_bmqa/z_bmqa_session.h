#ifndef INCLUDED_Z_BMQA_SESSION
#define INCLUDED_Z_BMQA_SESSION

#include <z_bmqt_sessionoptions.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void* z_bmqa_session;

int z_bmqa_session__create(z_bmqa_session* session , z_bmqt_SessionOptions* options);

int z_bmqa_session__start(z_bmqa_session* session, int64_t seconds, int nanoseconds);

int z_bmqa_session__stop(z_bmqa_session* session);

#if defined(__cplusplus)
}
#endif

#endif
