#ifndef INCLUDED_Z_BMQA_MESSAGE
#define INCLUDED_Z_BMQA_MESSAGE

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqa_Message z_bmqa_Message;

int z_bmqa_Message__createEmpty(z_bmqa_Message** message_obj);

int z_bmqa_Message__setDataRef(z_bmqa_Message* message_obj, const char* data, int length);


#if defined(__cplusplus)
}
#endif

#endif