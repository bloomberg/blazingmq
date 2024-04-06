#include <bmqa_openqueuestatus.h>
#include <z_bmqa_openqueuestatus.h>

int z_bmqa_OpenQueueStatus__delete(z_bmqa_OpenQueueStatus** openQueueStatus)
{
    using namespace BloombergLP;

    BSLS_ASSERT(openQueueStatus != NULL);

    bmqa::OpenQueueStatus* status_p = reinterpret_cast<bmqa::OpenQueueStatus*>(
        *openQueueStatus);
    delete status_p;
    *openQueueStatus = NULL;

    return 0;
}

int z_bmqa_OpenQueueStatus__create(z_bmqa_OpenQueueStatus** openQueueStatus)
{
    using namespace BloombergLP;

    bmqa::OpenQueueStatus* status_p = new bmqa::OpenQueueStatus();

    *openQueueStatus = reinterpret_cast<z_bmqa_OpenQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_OpenQueueStatus__createCopy(z_bmqa_OpenQueueStatus** openQueueStatus,
                                       const z_bmqa_OpenQueueStatus* other)
{
    using namespace BloombergLP;

    const bmqa::OpenQueueStatus* other_p =
        reinterpret_cast<const bmqa::OpenQueueStatus*>(other);
    bmqa::OpenQueueStatus* status_p = new bmqa::OpenQueueStatus(*other_p);

    *openQueueStatus = reinterpret_cast<z_bmqa_OpenQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_OpenQueueStatus__createFull(z_bmqa_OpenQueueStatus** openQueueStatus,
                                       const z_bmqa_QueueId*    queueId,
                                       int                      result,
                                       const char* errorDescription)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    const bsl::string           errorDescription_str(errorDescription);
    bmqt::OpenQueueResult::Enum result_enum =
        static_cast<bmqt::OpenQueueResult::Enum>(result);
    bmqa::OpenQueueStatus* status_p = new bmqa::OpenQueueStatus(
        *queueId_p,
        result_enum,
        errorDescription_str);
    *openQueueStatus = reinterpret_cast<z_bmqa_OpenQueueStatus*>(status_p);

    return 0;
}

bool z_bmqa_OpenQueueStatus__toBool(const z_bmqa_OpenQueueStatus* openQueueStatus)
{
    using namespace BloombergLP;
    const bmqa::OpenQueueStatus* status_p =
        reinterpret_cast<const bmqa::OpenQueueStatus*>(openQueueStatus);
    return *status_p;
}

int z_bmqa_OpenQueueStatus__queueId(const z_bmqa_OpenQueueStatus* openQueueStatus,
                                    const z_bmqa_QueueId**        queueId)
{
    using namespace BloombergLP;
    const bmqa::OpenQueueStatus* status_p =
        reinterpret_cast<const bmqa::OpenQueueStatus*>(openQueueStatus);
    const bmqa::QueueId* queueId_p = &(status_p->queueId());

    *queueId = reinterpret_cast<const z_bmqa_QueueId*>(queueId_p);
    return 0;
}

int z_bmqa_OpenQueueStatus__result(const z_bmqa_OpenQueueStatus* openQueueStatus)
{
    using namespace BloombergLP;
    const bmqa::OpenQueueStatus* status_p =
        reinterpret_cast<const bmqa::OpenQueueStatus*>(openQueueStatus);
    return status_p->result();
}

const char* z_bmqa_OpenQueueStatus__errorDescription(
    const z_bmqa_OpenQueueStatus* openQueueStatus)
{
    using namespace BloombergLP;
    const bmqa::OpenQueueStatus* status_p =
        reinterpret_cast<const bmqa::OpenQueueStatus*>(openQueueStatus);
    return status_p->errorDescription().c_str();
}