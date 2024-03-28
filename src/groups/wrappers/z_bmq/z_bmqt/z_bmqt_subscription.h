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

/**
 * @brief Creates a new subscription handle object.
 *
 * This function creates a new z_bmqt_SubscriptionHandle object and assigns its pointer to the provided pointer.
 * 
 * @param subscriptionHandle_obj A pointer to a pointer to a z_bmqt_SubscriptionHandle object.
 *                               Upon successful completion, this pointer will hold the newly created subscription handle object.
 * @param cid A pointer to a constant z_bmqt_CorrelationId object used to initialize the subscription handle.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_SubscriptionHandle__create(
    z_bmqt_SubscriptionHandle** subscriptionHandle_obj,
    const z_bmqt_CorrelationId* cid);

/**
 * @brief Retrieves the ID of the subscription handle.
 *
 * This function returns the ID of the specified subscription handle.
 * 
 * @param subscriptionHandle_obj A pointer to a constant z_bmqt_SubscriptionHandle object from which to retrieve the ID.
 * @return Returns the ID of the subscription handle.
 */
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


/**
 * @brief Creates a new subscription expression object.
 *
 * This function creates a new z_bmqt_SubscriptionExpression object and assigns its pointer to the provided pointer.
 * 
 * @param subscriptionExpression_obj A pointer to a pointer to a z_bmqt_SubscriptionExpression object.
 *                                    Upon successful completion, this pointer will hold the newly created subscription expression object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_SubscriptionExpression__create(
    z_bmqt_SubscriptionExpression** subscriptionExpression_obj);

/**
 * @brief Creates a new subscription expression object from a string.
 *
 * This function creates a new z_bmqt_SubscriptionExpression object from the provided string expression and version.
 * 
 * @param subscriptionExpression_obj A pointer to a pointer to a z_bmqt_SubscriptionExpression object.
 *                                    Upon successful completion, this pointer will hold the newly created subscription expression object.
 * @param expression A pointer to a constant character string representing the subscription expression.
 * @param version The version of the subscription expression.
 * @return Returns 0 upon successful creation, 1 if an unsupported version is provided.
 */
int z_bmqt_SubscriptionExpression__createFromString(
    z_bmqt_SubscriptionExpression** subscriptionExpression_obj,
    const char*                     expression,
    SubscriptionExpressionEnum      version);

/**
 * @brief Retrieves the text of the subscription expression.
 *
 * This function returns the text of the specified subscription expression.
 * 
 * @param subscriptionExpression_obj A pointer to a constant z_bmqt_SubscriptionExpression object from which to retrieve the text.
 * @return Returns a pointer to a constant character string representing the text of the subscription expression.
 *         The returned pointer may become invalid if the underlying subscription expression object is modified or destroyed.
 */
const char* z_bmqt_SubscriptionExpression__text(
    const z_bmqt_SubscriptionExpression* subscriptionExpression_obj);

/**
 * @brief Retrieves the version of the subscription expression.
 *
 * This function returns the version of the specified subscription expression.
 * 
 * @param subscriptionExpression_obj A pointer to a constant z_bmqt_SubscriptionExpression object from which to retrieve the version.
 * @return Returns the version of the subscription expression.
 */
SubscriptionExpressionEnum z_bmqt_SubscriptionExpression__version(
    const z_bmqt_SubscriptionExpression* subscriptionExpression_obj);

/**
 * @brief Checks whether the subscription expression is valid.
 *
 * This function checks whether the specified subscription expression is valid.
 * 
 * @param subscriptionExpression_obj A pointer to a constant z_bmqt_SubscriptionExpression object to check for validity.
 * @return Returns true if the subscription expression is valid, false otherwise.
 */
bool z_bmqt_SubscriptionExpression__isValid(
    const z_bmqt_SubscriptionExpression* subscriptionExpression_obj);

// ==================
// class Subscription
// ==================

typedef struct z_bmqt_Subscription z_bmqt_Subscription;

/**
 * @brief Creates a new subscription object.
 *
 * This function creates a new z_bmqt_Subscription object and assigns its pointer to the provided pointer.
 * 
 * @param subscription_obj A pointer to a pointer to a z_bmqt_Subscription object.
 *                         Upon successful completion, this pointer will hold the newly created subscription object.
 * @return Returns 0 upon successful creation.
 */

int z_bmqt_Subscription__create(z_bmqt_Subscription** subscription_obj);

/**
 * @brief Creates a copy of a subscription object.
 *
 * This function creates a copy of the specified z_bmqt_Subscription object and assigns its pointer to the provided pointer.
 * 
 * @param subscription_obj A pointer to a pointer to a z_bmqt_Subscription object.
 *                         Upon successful completion, this pointer will hold the newly created copy of the subscription object.
 * @param other A pointer to a constant z_bmqt_Subscription object to copy.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_Subscription__createCopy(z_bmqt_Subscription** subscription_obj,
                                    const z_bmqt_Subscription* other);

/**
 * @brief Sets the maximum number of unconfirmed messages for a subscription.
 *
 * This function sets the maximum number of unconfirmed messages for the specified subscription.
 * 
 * @param subscription_obj A pointer to a z_bmqt_Subscription object for which to set the maximum number of unconfirmed messages.
 * @param value The value representing the maximum number of unconfirmed messages to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_Subscription__setMaxUnconfirmedMessages(
    z_bmqt_Subscription* subscription_obj,
    int                  value);

/**
 * @brief Sets the maximum number of unconfirmed bytes for a subscription.
 *
 * This function sets the maximum number of unconfirmed bytes for the specified subscription.
 * 
 * @param subscription_obj A pointer to a z_bmqt_Subscription object for which to set the maximum number of unconfirmed bytes.
 * @param value The value representing the maximum number of unconfirmed bytes to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_Subscription__setMaxUnconfirmedBytes(
    z_bmqt_Subscription* subscription_obj,
    int                  value);

/**
 * @brief Sets the consumer priority for a subscription.
 *
 * This function sets the consumer priority for the specified subscription.
 * 
 * @param subscription_obj A pointer to a z_bmqt_Subscription object for which to set the consumer priority.
 * @param value The value representing the consumer priority to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_Subscription__setConsumerPriority(
    z_bmqt_Subscription* subscription_obj,
    int                  value);

/**
 * @brief Sets the subscription expression for a subscription.
 *
 * This function sets the subscription expression for the specified subscription.
 * 
 * @param subscription_obj A pointer to a z_bmqt_Subscription object for which to set the subscription expression.
 * @param value A pointer to a constant z_bmqt_SubscriptionExpression object representing the subscription expression to set.
 * @return Returns 0 upon successful setting.
 */
int z_bmqt_Subscription__setExpression(
    z_bmqt_Subscription*                 subscription_obj,
    const z_bmqt_SubscriptionExpression* value);

/**
 * @brief Retrieves the maximum number of unconfirmed messages for a subscription.
 *
 * This function returns the maximum number of unconfirmed messages for the specified subscription.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object from which to retrieve the maximum number of unconfirmed messages.
 * @return Returns the maximum number of unconfirmed messages for the subscription.
 */
int z_bmqt_Subscription__maxUnconfirmedMessages(
    const z_bmqt_Subscription* subscription_obj);

/**
 * @brief Retrieves the maximum number of unconfirmed bytes for a subscription.
 *
 * This function returns the maximum number of unconfirmed bytes for the specified subscription.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object from which to retrieve the maximum number of unconfirmed bytes.
 * @return Returns the maximum number of unconfirmed bytes for the subscription.
 */
int z_bmqt_Subscription__maxUnconfirmedBytes(
    const z_bmqt_Subscription* subscription_obj);

/**
 * @brief Retrieves the consumer priority for a subscription.
 *
 * This function returns the consumer priority for the specified subscription.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object from which to retrieve the consumer priority.
 * @return Returns the consumer priority for the subscription.
 */
int z_bmqt_Subscription__consumerPriority(
    const z_bmqt_Subscription* subscription_obj);

/**
 * @brief Retrieves the subscription expression for a subscription.
 *
 * This function returns a pointer to the subscription expression for the specified subscription.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object from which to retrieve the subscription expression.
 * @return Returns a pointer to a constant z_bmqt_SubscriptionExpression object representing the subscription expression.
 *         The returned pointer may become invalid if the underlying subscription object is modified or destroyed.
 */
const z_bmqt_SubscriptionExpression*
z_bmqt_Subscription__expression(const z_bmqt_Subscription* subscription_obj);

/**
 * @brief Checks whether the subscription specifies a maximum number of unconfirmed messages.
 *
 * This function checks whether the specified subscription specifies a maximum number of unconfirmed messages.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object to check.
 * @return Returns true if the subscription specifies a maximum number of unconfirmed messages, false otherwise.
 */
bool z_bmqt_Subscription__hasMaxUnconfirmedMessages(
    const z_bmqt_Subscription* subscription_obj);

/**
 * @brief Checks whether the subscription specifies a maximum number of unconfirmed bytes.
 *
 * This function checks whether the specified subscription specifies a maximum number of unconfirmed bytes.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object to check.
 * @return Returns true if the subscription specifies a maximum number of unconfirmed bytes, false otherwise.
 */
bool z_bmqt_Subscription__hasMaxUnconfirmedBytes(
    const z_bmqt_Subscription* subscription_obj);

/**
 * @brief Checks whether the subscription specifies a consumer priority.
 *
 * This function checks whether the specified subscription specifies a consumer priority.
 * 
 * @param subscription_obj A pointer to a constant z_bmqt_Subscription object to check.
 * @return Returns true if the subscription specifies a consumer priority, false otherwise.
 */
bool z_bmqt_Subscription__hasConsumerPriority(
    const z_bmqt_Subscription* subscription_obj);

#if defined(__cplusplus)
}
#endif

#endif