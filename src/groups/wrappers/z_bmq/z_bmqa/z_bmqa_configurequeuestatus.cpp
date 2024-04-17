#include <bmqa_configurequeuestatus.h>
#include <z_bmqa_configurequeuestatus.h>

int z_bmqa_ConfigureQueueStatus__delete(
    z_bmqa_ConfigureQueueStatus** configureQueueStatus)
{
    using namespace BloombergLP;

    BSLS_ASSERT(configureQueueStatus != NULL);

    bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<bmqa::ConfigureQueueStatus*>(*configureQueueStatus);
    delete status_p;
    *configureQueueStatus = NULL;

    return 0;
}

int z_bmqa_ConfigureQueueStatus__create(
    z_bmqa_ConfigureQueueStatus** configureQueueStatus)
{
    using namespace BloombergLP;

    bmqa::ConfigureQueueStatus* status_p = new bmqa::ConfigureQueueStatus();

    *configureQueueStatus = reinterpret_cast<z_bmqa_ConfigureQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_ConfigureQueueStatus__createCopy(
    z_bmqa_ConfigureQueueStatus**      configureQueueStatus,
    const z_bmqa_ConfigureQueueStatus* other)
{
    using namespace BloombergLP;

    const bmqa::ConfigureQueueStatus* other_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(other);
    bmqa::ConfigureQueueStatus* status_p = new bmqa::ConfigureQueueStatus(
        *other_p);

    *configureQueueStatus = reinterpret_cast<z_bmqa_ConfigureQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_ConfigureQueueStatus__createFull(
    z_bmqa_ConfigureQueueStatus** configureQueueStatus,
    const z_bmqa_QueueId*         queueId,
    int                           result,
    const char*                   errorDescription)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    const bsl::string                errorDescription_str(errorDescription);
    bmqt::ConfigureQueueResult::Enum result_enum =
        static_cast<bmqt::ConfigureQueueResult::Enum>(result);
    bmqa::ConfigureQueueStatus* status_p = new bmqa::ConfigureQueueStatus(
        *queueId_p,
        result_enum,
        errorDescription_str);
    *configureQueueStatus = reinterpret_cast<z_bmqa_ConfigureQueueStatus*>(status_p);

    return 0;
}

bool z_bmqa_ConfigureQueueStatus__toBool(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(configureQueueStatus);
    return *status_p;
}

int z_bmqa_ConfigureQueueStatus__queueId(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus,
    z_bmqa_QueueId const**             queueId_obj)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(configureQueueStatus);
    const bmqa::QueueId* queueId_p = &(status_p->queueId());

    *queueId_obj = reinterpret_cast<const z_bmqa_QueueId*>(queueId_p);
    return 0;
}

int z_bmqa_ConfigureQueueStatus__result(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(configureQueueStatus);
    return status_p->result();
}

const char* z_bmqa_ConfigureQueueStatus__errorDescription(
    const z_bmqa_ConfigureQueueStatus* configureQueueStatus)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(configureQueueStatus);
    return status_p->errorDescription().c_str();
}