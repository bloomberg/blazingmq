#ifndef INCLUDED_Z_BMQT_QUEUEOPTIONS
#define INCLUDED_Z_BMQT_QUEUEOPTIONS

typedef struct z_bmqt_QueueOptions z_bmqt_QueueOptions;

#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

int z_bmqt_QueueOptions__create(z_bmqt_QueueOptions** queueOptions_obj);

int z_bmqt_QueueOptions__createCopy(z_bmqt_QueueOptions** queueOptions_obj, const z_bmqt_QueueOptions* other);


//Modifiers
int z_bmqt_QueueOptions__setMaxUnconfirmedMessages(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__setMaxUnconfirmedBytes(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__setConsumerPriority(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__setSuspendsOnBadHostHealth(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__merge(z_bmqt_QueueOptions* queueOptions_obj, const z_bmqt_QueueOptions* other);

// int z_bmqt_QueueOptions__addOrUpdateSubscription(z_bmqt_QueueOptions* queueOptions_obj);

// int z_bmqt_QueueOptions__removeSubscription(z_bmqt_QueueOptions* queueOptions_obj);

int z_bmqt_QueueOptions__removeAllSubscriptions(z_bmqt_QueueOptions* queueOptions_obj);

//Accessors
int z_bmqt_QueueOptions__maxUnconfirmedMessages(const z_bmqt_QueueOptions* queueOptions_obj);

int z_bmqt_QueueOptions__maxUnconfirmedBytes(const z_bmqt_QueueOptions* queueOptions_obj);

int z_bmqt_QueueOptions__consumerPriority(const z_bmqt_QueueOptions* queueOptions_obj);

bool z_bmqt_QueueOptions__suspendsOnBadHostHealth(const z_bmqt_QueueOptions* queueOptions_obj);


bool z_bmqt_QueueOptions__hasMaxUnconfirmedMessages(const z_bmqt_QueueOptions* queueOptions_obj);

bool z_bmqt_QueueOptions__hasMaxUnconfirmedBytes(const z_bmqt_QueueOptions* queueOptions_obj);

bool z_bmqt_QueueOptions__hasConsumerPriority(const z_bmqt_QueueOptions* queueOptions_obj);

bool z_bmqt_QueueOptions__hasSuspendsOnBadHostHealth(const z_bmqt_QueueOptions* queueOptions_obj);


//Experimental (Modifiers)
// bool z_bmqt_QueueOptions__loadSubcription(z_bmqt_QueueOptions* queueOptions_obj);

// int z_bmqt_QueueOptions__loadSubscriptions(z_bmqt_QueueOptions* queueOptions_obj);

#if defined(__cplusplus)
}
#endif

#endif