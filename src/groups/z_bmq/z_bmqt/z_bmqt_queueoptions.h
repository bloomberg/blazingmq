#ifndef INCLUDED_Z_BMQT_QUEUEOPTIONS
#define INCLUDED_Z_BMQT_QUEUEOPTIONS

#include <stdbool.h>
#include <z_bmqt_types.h>
#include <z_bmqt_subscription.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqt_QueueOptions z_bmqt_QueueOptions;

typedef struct z_bmqt_HandleAndSubscrption {
    z_bmqt_SubscriptionHandle* handle;
    z_bmqt_Subscription* subscrption;
} z_bmqt_HandleAndSubscrption;

typedef struct z_bmqt_SubscrptionsSnapshot {
    uint64_t size;
    z_bmqt_HandleAndSubscrption* subscriptions;
} z_bmqt_SubscrptionsSnapshot;

int z_bmqt_QueueOptions__create(z_bmqt_QueueOptions** queueOptions_obj);

int z_bmqt_QueueOptions__createCopy(z_bmqt_QueueOptions** queueOptions_obj, const z_bmqt_QueueOptions* other);


//Modifiers
int z_bmqt_QueueOptions__setMaxUnconfirmedMessages(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__setMaxUnconfirmedBytes(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__setConsumerPriority(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__setSuspendsOnBadHostHealth(z_bmqt_QueueOptions* queueOptions_obj, int value);

int z_bmqt_QueueOptions__merge(z_bmqt_QueueOptions* queueOptions_obj, const z_bmqt_QueueOptions* other);

//Pass in an an uninitialized char** for errorDescription, the pointer will be set to NULL if there is no error, otherwise it will set to an appropriately sized string that is allocated on the heap
int z_bmqt_QueueOptions__addOrUpdateSubscription(z_bmqt_QueueOptions* queueOptions_obj, char** errorDescription, const z_bmqt_SubscriptionHandle* handle, const z_bmqt_Subscription* subscription);

int z_bmqt_QueueOptions__removeSubscription(z_bmqt_QueueOptions* queueOptions_obj, const z_bmqt_SubscriptionHandle* handle);

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
bool z_bmqt_QueueOptions__loadSubscription(const z_bmqt_QueueOptions* queueOptions_obj);

int z_bmqt_QueueOptions__loadSubscriptions(const z_bmqt_QueueOptions* queueOptions_obj, z_bmqt_SubscrptionsSnapshot* snapshot);

#if defined(__cplusplus)
}
#endif

#endif