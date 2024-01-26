#ifndef INCLUDED_Z_BMQA_CONFIRMEVENTBUILDER
#define INCLUDED_Z_BMQA_CONFIRMEVENTBUILDER

#include <z_bmqa_message.h>
#include <z_bmqt_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_ConfirmEventBuilder z_bmqa_ConfirmEventBuilder;
typedef struct z_bmqa_MessageConfirmationCookie
    z_bmqa_MessageConfirmationCookie;

int z_bmqa_ConfirmEventBuilder__delete(
    z_bmqa_ConfirmEventBuilder** builder_obj);

int z_bmqa_ConfirmEventBuilder__create(
    z_bmqa_ConfirmEventBuilder** builder_obj);

int z_bmqa_ConfirmEventBuilder__addMessageConfirmation(
    z_bmqa_ConfirmEventBuilder* builder_obj,
    const z_bmqa_Message*       message);

int z_bmqa_ConfirmEventBuilder__addMessageConfirmationWithCookie(
    z_bmqa_ConfirmEventBuilder*             builder_obj,
    const z_bmqa_MessageConfirmationCookie* cookie);

int z_bmqa_ConfirmEventBuilder__reset(z_bmqa_ConfirmEventBuilder* builder_obj);

int z_bmqa_ConfirmEventBuilder__messageCount(
    const z_bmqa_ConfirmEventBuilder* builder_obj);

int z_bmqa_ConfirmEventBuilder__blob(
    const z_bmqa_ConfirmEventBuilder* builder_obj,
    const z_bmqt_Blob**               blob_obj);

#if defined(__cplusplus)
}
#endif

#endif