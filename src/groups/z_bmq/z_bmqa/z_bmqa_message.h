#ifndef INCLUDED_Z_BMQA_MESSAGE
#define INCLUDED_Z_BMQA_MESSAGE

typedef struct z_bmqa_Message z_bmqa_Message;

int z_bmqa_Message__createEmpty(z_bmqa_Message** message_obj);

int z_bmqa_Message_setDataRef(z_bmqa_Message** message_obj, const char* data, int length);


#endif