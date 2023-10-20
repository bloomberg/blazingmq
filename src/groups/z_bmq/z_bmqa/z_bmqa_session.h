#ifndef INCLUDED_Z_BMQA_SESSION
#define INCLUDED_Z_BMQA_SESSION

#include <z_bmqt_sessionoptions.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_Session z_bmqa_Session;

int z_bmqa_Session__create(z_bmqa_Session** session, z_bmqt_SessionOptions* options);

int z_bmqa_Session__start(z_bmqa_Session* session, int64_t milliseconds);

int z_bmqa_Session__stop(z_bmqa_Session* session);

#if defined(__cplusplus)
}
#endif

#endif
