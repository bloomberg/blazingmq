#include <bmqa_configurequeuestatus.h>
#include <z_bmqa_configurequeuestatus.h>

int z_bmqa_ConfigureQueueStatus__delete(
    z_bmqa_ConfigureQueueStatus** status_obj)
{
    using namespace BloombergLP;

    BSLS_ASSERT(status_obj != NULL);

    bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<bmqa::ConfigureQueueStatus*>(*status_obj);
    delete status_p;
    *status_obj = NULL;

    return 0;
}

int z_bmqa_ConfigureQueueStatus__create(
    z_bmqa_ConfigureQueueStatus** status_obj)
{
    using namespace BloombergLP;

    bmqa::ConfigureQueueStatus* status_p = new bmqa::ConfigureQueueStatus();

    *status_obj = reinterpret_cast<z_bmqa_ConfigureQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_ConfigureQueueStatus__createCopy(
    z_bmqa_ConfigureQueueStatus**      status_obj,
    const z_bmqa_ConfigureQueueStatus* other)
{
    using namespace BloombergLP;

    const bmqa::ConfigureQueueStatus* other_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(other);
    bmqa::ConfigureQueueStatus* status_p = new bmqa::ConfigureQueueStatus(
        *other_p);

    *status_obj = reinterpret_cast<z_bmqa_ConfigureQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_ConfigureQueueStatus__createFull(
    z_bmqa_ConfigureQueueStatus** status_obj,
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
    *status_obj = reinterpret_cast<z_bmqa_ConfigureQueueStatus*>(status_p);

    return 0;
}

bool z_bmqa_ConfigureQueueStatus__toBool(
    const z_bmqa_ConfigureQueueStatus* status_obj)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(status_obj);
    return *status_p;
}

int z_bmqa_ConfigureQueueStatus__queueId(
    const z_bmqa_ConfigureQueueStatus* status_obj,
    z_bmqa_QueueId const**             queueId_obj)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(status_obj);
    const bmqa::QueueId* queueId_p = &(status_p->queueId());

    *queueId_obj = reinterpret_cast<const z_bmqa_QueueId*>(queueId_p);
    return 0;
}

int z_bmqa_ConfigureQueueStatus__result(
    const z_bmqa_ConfigureQueueStatus* status_obj)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(status_obj);
    return status_p->result();
}

const char* z_bmqa_ConfigureQueueStatus__errorDescription(
    const z_bmqa_ConfigureQueueStatus* status_obj)
{
    using namespace BloombergLP;
    const bmqa::ConfigureQueueStatus* status_p =
        reinterpret_cast<const bmqa::ConfigureQueueStatus*>(status_obj);
    return status_p->errorDescription().c_str();
}