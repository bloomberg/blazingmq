#include <z_bmqt_subscription.h>
#include <bmqt_correlationid.h>
#include <bmqt_subscription.h>
#include <bsl_string.h>

int z_bmqt_SubscriptionHandle__create(z_bmqt_SubscriptionHandle** subscriptionHandle_obj, const z_bmqt_CorrelationId* cid){
    using namespace BloombergLP;

    const bmqt::CorrelationId* correlationId = reinterpret_cast<const bmqt::CorrelationId*>(cid);
    bmqt::SubscriptionHandle* subscriptionHandle_ptr = new bmqt::SubscriptionHandle(*correlationId);
    *subscriptionHandle_obj = reinterpret_cast<z_bmqt_SubscriptionHandle*>(subscriptionHandle_ptr);

    return 0;
}

unsigned int z_bmqt_SubscriptionHandle__id(const z_bmqt_SubscriptionHandle* subscriptionHandle_obj){
    using namespace BloombergLP;

    const bmqt::SubscriptionHandle* subscriptionHandle_ptr = reinterpret_cast<const bmqt::SubscriptionHandle*>(subscriptionHandle_obj);
    return subscriptionHandle_ptr->id();
}

///////

int z_bmqt_SubscriptionExpression__create(z_bmqt_SubscriptionExpression** subscriptionExpression_obj){
    using namespace BloombergLP;

    bmqt::SubscriptionExpression* subscriptionExpression_ptr = new bmqt::SubscriptionExpression();
    *subscriptionExpression_obj = reinterpret_cast<z_bmqt_SubscriptionExpression*>(subscriptionExpression_ptr);

    return 0;
}

int z_bmqt_SubscriptionExpression__createFromString(z_bmqt_SubscriptionExpression** subscriptionExpression_obj, const char* expression, SubscriptionExpressionEnum version){
    using namespace BloombergLP;

    bmqt::SubscriptionExpression::Enum v;
    switch(version) {
        case SubscriptionExpressionEnum::e_NONE: v = bmqt::SubscriptionExpression::e_NONE;
        case SubscriptionExpressionEnum::e_VERSION_1: v = bmqt::SubscriptionExpression::e_VERSION_1;
        default: return 1;
    }

    bmqt::SubscriptionExpression* subscriptionExpression_ptr = new bmqt::SubscriptionExpression(bsl::string(expression), v);

    *subscriptionExpression_obj = reinterpret_cast<z_bmqt_SubscriptionExpression*>(subscriptionExpression_ptr);

    return 0;
}

const char* z_bmqt_SubscriptionExpression__text(const z_bmqt_SubscriptionExpression* subscriptionExpression_obj){
    using namespace BloombergLP;

    const bmqt::SubscriptionExpression* subscriptionExpression_ptr = reinterpret_cast<const bmqt::SubscriptionExpression*>(subscriptionExpression_obj);
    return subscriptionExpression_ptr->text().c_str();
}

SubscriptionExpressionEnum z_bmqt_SubscriptionExpression__version(const z_bmqt_SubscriptionExpression* subscriptionExpression_obj){
    using namespace BloombergLP;

    const bmqt::SubscriptionExpression* subscriptionExpression_ptr = reinterpret_cast<const bmqt::SubscriptionExpression*>(subscriptionExpression_obj);
    switch(subscriptionExpression_ptr->version()) {
        case bmqt::SubscriptionExpression::e_NONE: return SubscriptionExpressionEnum::e_NONE;
        case bmqt::SubscriptionExpression::e_VERSION_1: return SubscriptionExpressionEnum::e_VERSION_1;
        default: break;
    }

    return SubscriptionExpressionEnum::e_SUBSCRIPTIONEXPRESSION_ERROR;
}

bool z_bmqt_SubscriptionExpression__isValid(const z_bmqt_SubscriptionExpression* subscriptionExpression_obj){
    using namespace BloombergLP;

    const bmqt::SubscriptionExpression* subscriptionExpression_ptr = reinterpret_cast<const bmqt::SubscriptionExpression*>(subscriptionExpression_obj);
    return subscriptionExpression_ptr->isValid();
}

///////

int z_bmqt_Subscription__create(z_bmqt_Subscription** subscription_obj){
    using namespace BloombergLP;

    bmqt::Subscription* subscription_ptr = new bmqt::Subscription();
    *subscription_obj = reinterpret_cast<z_bmqt_Subscription*>(subscription_ptr);

    return 0;
}

int z_bmqt_Subscription__createCopy(z_bmqt_Subscription** subscription_obj, const z_bmqt_Subscription* other){
    using namespace BloombergLP;

    const bmqt::Subscription* other_ptr = reinterpret_cast<const bmqt::Subscription*>(other);
    bmqt::Subscription* subscription_ptr = new bmqt::Subscription(*other_ptr);
    *subscription_obj = reinterpret_cast<z_bmqt_Subscription*>(subscription_ptr);

    return 0;
}

int z_bmqt_Subscription__setMaxUnconfirmedMessages(z_bmqt_Subscription* subscription_obj, int value){
    using namespace BloombergLP;

    bmqt::Subscription* subscription_ptr = reinterpret_cast<bmqt::Subscription*>(subscription_obj);
    subscription_ptr->setMaxUnconfirmedMessages(value);

    return 0;
}

int z_bmqt_Subscription__setMaxUnconfirmedBytes(z_bmqt_Subscription* subscription_obj, int value){
    using namespace BloombergLP;

    bmqt::Subscription* subscription_ptr = reinterpret_cast<bmqt::Subscription*>(subscription_obj);
    subscription_ptr->setMaxUnconfirmedBytes(value);

    return 0;
}

int z_bmqt_Subscription__setConsumerPriority(z_bmqt_Subscription* subscription_obj, int value){
    using namespace BloombergLP;

    bmqt::Subscription* subscription_ptr = reinterpret_cast<bmqt::Subscription*>(subscription_obj);
    subscription_ptr->setConsumerPriority(value);

    return 0;
}

int z_bmqt_Subscription__setExpression(z_bmqt_Subscription* subscription_obj, const z_bmqt_SubscriptionExpression* value){
    using namespace BloombergLP;

    bmqt::Subscription* subscription_ptr = reinterpret_cast<bmqt::Subscription*>(subscription_obj);
    const bmqt::SubscriptionExpression* expression_ptr = reinterpret_cast<const bmqt::SubscriptionExpression*>(value);
    subscription_ptr->setExpression(*expression_ptr);

    return 0;
}

int z_bmqt_Subscription__maxUnconfirmedMessages(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    return subscription_ptr->maxUnconfirmedMessages();
}

int z_bmqt_Subscription__maxUnconfirmedBytes(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    return subscription_ptr->maxUnconfirmedBytes();
}

int z_bmqt_Subscription__consumerPriority(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    return subscription_ptr->consumerPriority();
}

const z_bmqt_SubscriptionExpression* z_bmqt_Subscription__expression(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    const bmqt::SubscriptionExpression* expression_ptr = &(subscription_ptr->expression());

    return reinterpret_cast<const z_bmqt_SubscriptionExpression*>(expression_ptr);
}

bool z_bmqt_Subscription__hasMaxUnconfirmedMessages(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    return subscription_ptr->hasMaxUnconfirmedMessages();
}

bool z_bmqt_Subscription__hasMaxUnconfirmedBytes(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    return subscription_ptr->hasMaxUnconfirmedBytes();
}

bool z_bmqt_Subscription__hasConsumerPriority(const z_bmqt_Subscription* subscription_obj){
    using namespace BloombergLP;

    const bmqt::Subscription* subscription_ptr = reinterpret_cast<const bmqt::Subscription*>(subscription_obj);
    return subscription_ptr->hasConsumerPriority();
}