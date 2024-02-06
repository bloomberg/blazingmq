#ifndef INCLUDED_Z_BMQT_SUBSCRIPTION
#define INCLUDED_Z_BMQT_SUBSCRIPTION

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqt_correlationid.h>

// ========================
// class SubscriptionHandle
// ========================

typedef struct z_bmqt_SubscriptionHandle z_bmqt_SubscriptionHandle;

int z_bmqt_SubscriptionHandle__create(
    z_bmqt_SubscriptionHandle** subscriptionHandle_obj,
    const z_bmqt_CorrelationId* cid);

unsigned int z_bmqt_SubscriptionHandle__id(
    const z_bmqt_SubscriptionHandle* subscriptionHandle_obj);

// ============================
// class SubscriptionExpression
// ============================

typedef struct z_bmqt_SubscriptionExpression z_bmqt_SubscriptionExpression;

enum SubscriptionExpressionEnum {
    // Enum representing criteria format
    e_NONE = 0  // EMPTY
    ,
    e_VERSION_1 = 1  // Simple Evaluator
    ,
    e_SUBSCRIPTIONEXPRESSION_ERROR
};

int z_bmqt_SubscriptionExpression__create(
    z_bmqt_SubscriptionExpression** subscriptionExpression_obj);

int z_bmqt_SubscriptionExpression__createFromString(
    z_bmqt_SubscriptionExpression** subscriptionExpression_obj,
    const char*                     expression,
    SubscriptionExpressionEnum      version);

const char* z_bmqt_SubscriptionExpression__text(
    const z_bmqt_SubscriptionExpression* subscriptionExpression_obj);

SubscriptionExpressionEnum z_bmqt_SubscriptionExpression__version(
    const z_bmqt_SubscriptionExpression* subscriptionExpression_obj);

bool z_bmqt_SubscriptionExpression__isValid(
    const z_bmqt_SubscriptionExpression* subscriptionExpression_obj);

// ==================
// class Subscription
// ==================

typedef struct z_bmqt_Subscription z_bmqt_Subscription;

int z_bmqt_Subscription__create(z_bmqt_Subscription** subscription_obj);

int z_bmqt_Subscription__createCopy(z_bmqt_Subscription** subscription_obj,
                                    const z_bmqt_Subscription* other);

int z_bmqt_Subscription__setMaxUnconfirmedMessages(
    z_bmqt_Subscription* subscription_obj,
    int                  value);

int z_bmqt_Subscription__setMaxUnconfirmedBytes(
    z_bmqt_Subscription* subscription_obj,
    int                  value);

int z_bmqt_Subscription__setConsumerPriority(
    z_bmqt_Subscription* subscription_obj,
    int                  value);

int z_bmqt_Subscription__setExpression(
    z_bmqt_Subscription*                 subscription_obj,
    const z_bmqt_SubscriptionExpression* value);

int z_bmqt_Subscription__maxUnconfirmedMessages(
    const z_bmqt_Subscription* subscription_obj);

int z_bmqt_Subscription__maxUnconfirmedBytes(
    const z_bmqt_Subscription* subscription_obj);

int z_bmqt_Subscription__consumerPriority(
    const z_bmqt_Subscription* subscription_obj);

const z_bmqt_SubscriptionExpression*
z_bmqt_Subscription__expression(const z_bmqt_Subscription* subscription_obj);

bool z_bmqt_Subscription__hasMaxUnconfirmedMessages(
    const z_bmqt_Subscription* subscription_obj);

bool z_bmqt_Subscription__hasMaxUnconfirmedBytes(
    const z_bmqt_Subscription* subscription_obj);

bool z_bmqt_Subscription__hasConsumerPriority(
    const z_bmqt_Subscription* subscription_obj);

#if defined(__cplusplus)
}
#endif

#endif