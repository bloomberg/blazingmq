#include <bmqt_queueoptions.h>
#include <bmqt_subscription.h>
#include <bsl_string.h>
#include <string.h>
#include <z_bmqt_queueoptions.h>

int z_bmqt_QueueOptions__create(z_bmqt_QueueOptions** queueOptions_obj) {
    using namespace BloombergLP;
    bmqt::QueueOptions* queueOptions_ptr = new bmqt::QueueOptions;

    *queueOptions_obj = reinterpret_cast<z_bmqt_QueueOptions*>(queueOptions_ptr);

    return 0;
}

int z_bmqt_QueueOptions__createCopy(z_bmqt_QueueOptions** queueOptions_obj, const z_bmqt_QueueOptions* other) {
    using namespace BloombergLP;
    const bmqt::QueueOptions* other_ptr = reinterpret_cast<const bmqt::QueueOptions*>(other);
    bmqt::QueueOptions* queueOptions_ptr = new bmqt::QueueOptions(*other_ptr);

    *queueOptions_obj = reinterpret_cast<z_bmqt_QueueOptions*>(queueOptions_ptr);

    return 0;
}


//Modifiers
int z_bmqt_QueueOptions__setMaxUnconfirmedMessages(z_bmqt_QueueOptions* queueOptions_obj, int value) {
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);

    queueOptions_ptr->setMaxUnconfirmedMessages(value);
    return 0;
}

int z_bmqt_QueueOptions__setMaxUnconfirmedBytes(z_bmqt_QueueOptions* queueOptions_obj, int value) {
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    queueOptions_ptr->setMaxUnconfirmedBytes(value);
    return 0;
}

int z_bmqt_QueueOptions__setConsumerPriority(z_bmqt_QueueOptions* queueOptions_obj, int value) {
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    queueOptions_ptr->setConsumerPriority(value);
    return 0;
}

int z_bmqt_QueueOptions__setSuspendsOnBadHostHealth(z_bmqt_QueueOptions* queueOptions_obj, int value) {
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    queueOptions_ptr->setSuspendsOnBadHostHealth(value);
    return 0;
}

int z_bmqt_QueueOptions__merge(z_bmqt_QueueOptions* queueOptions_obj, const z_bmqt_QueueOptions* other) {
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    const bmqt::QueueOptions* other_ptr = reinterpret_cast<const bmqt::QueueOptions*>(other);

    queueOptions_ptr->merge(*other_ptr);
    return 0;
}

int z_bmqt_QueueOptions__addOrUpdateSubscription(z_bmqt_QueueOptions* queueOptions_obj, char** errorDescription, const z_bmqt_SubscriptionHandle* handle, const z_bmqt_Subscription* subscription){
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    const bmqt::SubscriptionHandle* handle_ptr = reinterpret_cast<const bmqt::SubscriptionHandle*>(handle);
    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription);

    bsl::string error;

    queueOptions_ptr->addOrUpdateSubscription(&error, *handle_ptr, *subscription_ptr);

    if(error.empty()) {
        *errorDescription = NULL;
        return 0;
    } else {
        *errorDescription = static_cast<char*>(calloc(error.size()+1, sizeof(char)));
        strcpy(*errorDescription, error.c_str());

        return 1;
    }
}

int z_bmqt_QueueOptions__removeSubscription(z_bmqt_QueueOptions* queueOptions_obj, const z_bmqt_SubscriptionHandle* handle){
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    const bmqt::SubscriptionHandle* handle_ptr = reinterpret_cast<const bmqt::SubscriptionHandle*>(handle);
    queueOptions_ptr->removeSubscription(*handle_ptr);

    return 0;
}

int z_bmqt_QueueOptions__removeAllSubscriptions(z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<bmqt::QueueOptions*>(queueOptions_obj);
    queueOptions_ptr->removeAllSubscriptions();
    return 0;
}

//Accessors
int z_bmqt_QueueOptions__maxUnconfirmedMessages(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->maxUnconfirmedMessages();
}

int z_bmqt_QueueOptions__maxUnconfirmedBytes(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->maxUnconfirmedBytes();
}

int z_bmqt_QueueOptions__consumerPriority(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->consumerPriority();
}

bool z_bmqt_QueueOptions__suspendsOnBadHostHealth(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->suspendsOnBadHostHealth();
}


bool z_bmqt_QueueOptions__hasMaxUnconfirmedMessages(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->hasMaxUnconfirmedMessages();
}

bool z_bmqt_QueueOptions__hasMaxUnconfirmedBytes(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->hasMaxUnconfirmedBytes();
}

bool z_bmqt_QueueOptions__hasConsumerPriority(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->hasConsumerPriority();
}

bool z_bmqt_QueueOptions__hasSuspendsOnBadHostHealth(const z_bmqt_QueueOptions* queueOptions_obj) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    return queueOptions_ptr->hasSuspendsOnBadHostHealth();
}


//Experimental (Modifiers)
bool z_bmqt_QueueOptions__loadSubscription(const z_bmqt_QueueOptions* queueOptions_obj, z_bmqt_Subscription** subscription, const z_bmqt_SubscriptionHandle* handle) {
    using namespace BloombergLP;

    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    const bmqt::SubscriptionHandle* handle_ptr = reinterpret_cast<const bmqt::SubscriptionHandle*>(handle);
    bmqt::Subscription* subscription_ptr = new bmqt::Subscription();

    bool success = queueOptions_ptr->loadSubscription(subscription_ptr, *handle_ptr);
    
    if(success) {
        *subscription = reinterpret_cast<z_bmqt_Subscription*>(subscription_ptr);
    } else {
        delete subscription_ptr;
        *subscription = NULL;
    }

    return success;
}

int z_bmqt_QueueOptions__loadSubscriptions(const z_bmqt_QueueOptions* queueOptions_obj, z_bmqt_SubscrptionsSnapshot* snapshot) {
    using namespace BloombergLP;

    bmqt::QueueOptions::SubscriptionsSnapshot vector;
    const bmqt::QueueOptions* queueOptions_ptr = reinterpret_cast<const bmqt::QueueOptions*>(queueOptions_obj);
    queueOptions_ptr->loadSubscriptions(&vector);

    snapshot->size = vector.size();
    snapshot->subscriptions = static_cast<z_bmqt_HandleAndSubscrption*>(calloc(vector.size(), sizeof(z_bmqt_HandleAndSubscrption)));

    for(size_t i = 0; i < vector.size(); ++i) {
        bmqt::SubscriptionHandle* handle_ptr = new bmqt::SubscriptionHandle(vector[i].first);
        bmqt::Subscription* subscription_ptr = new bmqt::Subscription(vector[i].second);

        snapshot->subscriptions[i].handle = reinterpret_cast<z_bmqt_SubscriptionHandle*>(handle_ptr);
        snapshot->subscriptions[i].subscrption = reinterpret_cast<z_bmqt_Subscription*>(subscription_ptr);
    }

    return 0;
}
