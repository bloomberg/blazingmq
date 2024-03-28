#ifndef INCLUDED_Z_BMQT_QUEUEOPTIONS
#define INCLUDED_Z_BMQT_QUEUEOPTIONS

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <z_bmqt_subscription.h>
#include <z_bmqt_types.h>

typedef struct z_bmqt_QueueOptions z_bmqt_QueueOptions;

typedef struct z_bmqt_HandleAndSubscrption {
    z_bmqt_SubscriptionHandle* handle;
    z_bmqt_Subscription*       subscrption;
} z_bmqt_HandleAndSubscrption;

typedef struct z_bmqt_SubscrptionsSnapshot {
    uint64_t                     size;
    z_bmqt_HandleAndSubscrption* subscriptions;
} z_bmqt_SubscrptionsSnapshot;

/**
 * @brief Deletes the memory allocated for a pointer to a bmqt::QueueOptions object.
 *
 * @param queueOptions_obj A pointer to a pointer to a bmqt::QueueOptions object.
 *                         Upon successful completion, this pointer will be set to NULL.
 * @return Returns 0 upon successful deletion.
 */
int z_bmqt_QueueOptions__delete(z_bmqt_QueueOptions** queueOptions_obj);

/**
 * @brief Creates a new bmqt::QueueOptions object and assigns it to the provided pointer.
 *
 * This function allocates memory for a new bmqt::QueueOptions object and assigns it to the pointer provided as 'queueOptions_obj'.
 * 
 * @param queueOptions_obj A pointer to a pointer to a bmqt::QueueOptions object.
 *                         Upon successful completion, this pointer will point to the newly allocated bmqt::QueueOptions object.
 * @return Returns 0 upon successful creation.
 */
int z_bmqt_QueueOptions__create(z_bmqt_QueueOptions** queueOptions_obj);

/**
 * @brief Creates a copy of the provided z_bmqt::QueueOptions object.
 *
 * This function creates a deep copy of the z_bmqt::QueueOptions object 'other' and assigns it to a new memory location.
 * 
 * @param queueOptions_obj A pointer to a pointer to a z_bmqt::QueueOptions object where the copy will be stored.
 * @param other A const pointer to a z_bmqt::QueueOptions object that will be copied.
 * 
 * @return Returns 0 upon successful creation of the copy.
 */
int z_bmqt_QueueOptions__createCopy(z_bmqt_QueueOptions** queueOptions_obj,
                                    const z_bmqt_QueueOptions* other);

/**
 * @brief Sets the maximum number of unconfirmed messages for a QueueOptions object.
 *
 * This function sets the maximum number of unconfirmed messages allowed for a given QueueOptions object.
 * 
 * @param queueOptions_obj A pointer to a QueueOptions object.
 * @param value The maximum number of unconfirmed messages to set.
 * 
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_QueueOptions__setMaxUnconfirmedMessages(
    z_bmqt_QueueOptions* queueOptions_obj,
    int                  value);

/**
 * @brief Sets the maximum number of unconfirmed bytes for a z_bmqt_QueueOptions object.
 *
 * @param queueOptions_obj A pointer to a z_bmqt_QueueOptions object.
 * @param value The maximum number of unconfirmed bytes to set.
 * 
 * @returns Returns 0 upon successful completion.
 */
int z_bmqt_QueueOptions__setMaxUnconfirmedBytes(
    z_bmqt_QueueOptions* queueOptions_obj,
    int                  value);

/**
 * @brief Sets the consumer priority value in the provided QueueOptions object.
 *
 * @param queueOptions_obj A pointer to a QueueOptions object.
 * @param value The value to set as the consumer priority.
 * 
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_QueueOptions__setConsumerPriority(
    z_bmqt_QueueOptions* queueOptions_obj,
    int                  value);

/**
 * @brief Sets the option to suspend queue operations upon detecting bad host health.
 *
 * This function sets the option to suspend queue operations upon detecting bad host health.
 * 
 * @param queueOptions_obj A pointer to a z_bmqt_QueueOptions object.
 * @param value The value indicating whether to suspend queue operations on bad host health.
 *              A non-zero value indicates suspending, while zero indicates not suspending.
 * 
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_QueueOptions__setSuspendsOnBadHostHealth(
    z_bmqt_QueueOptions* queueOptions_obj,
    int                  value);

/**
 * @brief Merges the properties of another z_bmqt_QueueOptions object into the current z_bmqt_QueueOptions object.
 *
 * This function merges the properties of another z_bmqt_QueueOptions object ('other') into the current z_bmqt_QueueOptions object ('queueOptions_obj').
 * 
 * @param queueOptions_obj A pointer to the z_bmqt_QueueOptions object to merge into. Upon successful completion, this object will contain the merged properties.
 * @param other A pointer to the z_bmqt_QueueOptions object whose properties will be merged into 'queueOptions_obj'.
 * 
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_QueueOptions__merge(z_bmqt_QueueOptions*       queueOptions_obj,
                               const z_bmqt_QueueOptions* other);

// Pass in an an uninitialized char** for errorDescription, the pointer will be
// set to NULL if there is no error, otherwise it will set to an appropriately
// sized string that is allocated on the heap

/**
 * @brief Adds or updates a subscription to the queue options.
 *
 * This function adds or updates a subscription to the provided queue options object.
 * If the operation is successful, the error description pointer will be set to NULL.
 * If an error occurs, the error description pointer will be allocated memory for the error message.
 * 
 * @param queueOptions_obj   A pointer to the queue options object to which the subscription will be added or updated.
 * @param errorDescription   A pointer to a pointer to a character array where the error description will be stored if an error occurs.
 *                            Upon successful completion, this pointer will be set to NULL.
 * @param handle             A pointer to the subscription handle.
 * @param subscription       A pointer to the subscription object.
 * @return Returns 0 upon successful addition or update of the subscription.
 *         Returns 1 if an error occurs, and sets the error description accordingly.
 */
int z_bmqt_QueueOptions__addOrUpdateSubscription(
    z_bmqt_QueueOptions*             queueOptions_obj,
    char**                           errorDescription,
    const z_bmqt_SubscriptionHandle* handle,
    const z_bmqt_Subscription*       subscription);

/**
 * @brief Removes a subscription from the queue options.
 *
 * This function removes a subscription identified by the provided handle from the given queue options object.
 * 
 * @param queueOptions_obj A pointer to a z_bmqt_QueueOptions object from which the subscription will be removed.
 * @param handle A pointer to a z_bmqt_SubscriptionHandle object representing the handle of the subscription to be removed.
 * 
 * @return Returns 0 upon successful removal of the subscription.
 */
int z_bmqt_QueueOptions__removeSubscription(
    z_bmqt_QueueOptions*             queueOptions_obj,
    const z_bmqt_SubscriptionHandle* handle);

/**
 * @brief Removes all subscriptions from a QueueOptions object.
 *
 * This function removes all subscriptions associated with the provided QueueOptions object.
 * 
 * @param queueOptions_obj A pointer to a QueueOptions object from which all subscriptions will be removed.
 * 
 * @return Returns 0 upon successful removal of all subscriptions.
 */
int z_bmqt_QueueOptions__removeAllSubscriptions(
    z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Retrieves the maximum number of unconfirmed messages allowed by the specified QueueOptions.
 *
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the maximum number of unconfirmed messages.
 * @return Returns the maximum number of unconfirmed messages allowed by the specified QueueOptions.
 */
int z_bmqt_QueueOptions__maxUnconfirmedMessages(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Retrieves the maximum number of unconfirmed bytes allowed in the queue options.
 *
 * This function returns the maximum number of unconfirmed bytes allowed in the queue options.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the maximum unconfirmed bytes.
 * @return Returns the maximum number of unconfirmed bytes allowed in the queue options.
 */
int z_bmqt_QueueOptions__maxUnconfirmedBytes(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Retrieves the consumer priority from the queue options.
 *
 * This function returns the consumer priority from the specified queue options.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the consumer priority.
 * @return Returns the consumer priority specified in the queue options.
 */
int z_bmqt_QueueOptions__consumerPriority(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Checks whether the queue suspends on bad host health.
 *
 * This function checks whether the queue suspends on bad host health as specified in the queue options.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the suspension status.
 * @return Returns true if the queue suspends on bad host health, false otherwise.
 */
bool z_bmqt_QueueOptions__suspendsOnBadHostHealth(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Checks whether the queue options specify a maximum number of unconfirmed messages.
 *
 * This function checks whether the queue options specify a maximum number of unconfirmed messages.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the information.
 * @return Returns true if the queue options specify a maximum number of unconfirmed messages, false otherwise.
 */
bool z_bmqt_QueueOptions__hasMaxUnconfirmedMessages(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Checks whether the queue options specify a maximum number of unconfirmed bytes.
 *
 * This function checks whether the queue options specify a maximum number of unconfirmed bytes.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the information.
 * @return Returns true if the queue options specify a maximum number of unconfirmed bytes, false otherwise.
 */
bool z_bmqt_QueueOptions__hasMaxUnconfirmedBytes(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Checks whether the queue options specify a consumer priority.
 *
 * This function checks whether the queue options specify a consumer priority.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the information.
 * @return Returns true if the queue options specify a consumer priority, false otherwise.
 */
bool z_bmqt_QueueOptions__hasConsumerPriority(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Checks whether the queue options specify suspension on bad host health.
 *
 * This function checks whether the queue options specify suspension on bad host health.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object from which to retrieve the information.
 * @return Returns true if the queue options specify suspension on bad host health, false otherwise.
 */
bool z_bmqt_QueueOptions__hasSuspendsOnBadHostHealth(
    const z_bmqt_QueueOptions* queueOptions_obj);

// Experimental (Modifiers)
/**
 * @brief Loads a subscription into the queue options.
 *
 * This function attempts to load a subscription into the specified queue options using the provided subscription handle.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object where the subscription will be loaded.
 * @param subscription A pointer to a pointer to a z_bmqt_Subscription object. Upon successful completion, this pointer will be set to the loaded subscription.
 * @param handle A pointer to a constant z_bmqt_SubscriptionHandle object specifying the handle of the subscription to load.
 * @return Returns true if the subscription was successfully loaded into the queue options, false otherwise.
 */
bool z_bmqt_QueueOptions__loadSubscription(
    const z_bmqt_QueueOptions* queueOptions_obj);

/**
 * @brief Loads subscriptions snapshot into the queue options.
 *
 * This function loads a snapshot of subscriptions into the specified queue options.
 * 
 * @param queueOptions_obj A pointer to a constant z_bmqt_QueueOptions object where the subscriptions will be loaded.
 * @param snapshot A pointer to a z_bmqt_SubscrptionsSnapshot object where the subscriptions snapshot will be stored.
 * @return Returns 0 upon successful completion.
 */
int z_bmqt_QueueOptions__loadSubscriptions(
    const z_bmqt_QueueOptions*   queueOptions_obj,
    z_bmqt_SubscrptionsSnapshot* snapshot);

#if defined(__cplusplus)
}
#endif

#endif