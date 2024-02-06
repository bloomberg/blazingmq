#ifndef INCLUDED_Z_BMQA_MESSAGEITERATOR
#define INCLUDED_Z_BMQA_MESSAGEITERATOR

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqa_message.h>

typedef struct z_bmqa_MessageIterator z_bmqa_MessageIterator;

int z_bmqa_MessageIterator__create(
    z_bmqa_MessageIterator** messageIterator_obj);

int z_bmqa_MessageIterator__delete(
    z_bmqa_MessageIterator** messageIterator_obj);

bool z_bmqa_MessageIterator__nextMessage(
    z_bmqa_MessageIterator* messageIterator_obj);

int z_bmqa_MessageIterator__message(
    const z_bmqa_MessageIterator* messageIterator_obj,
    const z_bmqa_Message**        message_obj);

#if defined(__cplusplus)
}
#endif

#endif