#include <bmqa_closequeuestatus.h>
#include <z_bmqa_closequeuestatus.h>

int z_bmqa_CloseQueueStatus__delete(z_bmqa_CloseQueueStatus** status_obj) {
    using namespace BloombergLP;

    bmqa::CloseQueueStatus* status_p = reinterpret_cast<bmqa::CloseQueueStatus*>(*status_obj);
    delete status_p;
    *status_obj = NULL;

    return 0;
}

int z_bmqa_CloseQueueStatus__create(z_bmqa_CloseQueueStatus** status_obj)
{
    using namespace BloombergLP;

    bmqa::CloseQueueStatus* status_p = new bmqa::CloseQueueStatus();

    *status_obj = reinterpret_cast<z_bmqa_CloseQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_CloseQueueStatus__createCopy(z_bmqa_CloseQueueStatus** status_obj,
                                        const z_bmqa_CloseQueueStatus* other)
{
    using namespace BloombergLP;

    const bmqa::CloseQueueStatus* other_p =
        reinterpret_cast<const bmqa::CloseQueueStatus*>(other);
    bmqa::CloseQueueStatus* status_p = new bmqa::CloseQueueStatus(*other_p);

    *status_obj = reinterpret_cast<z_bmqa_CloseQueueStatus*>(status_p);

    return 0;
}

int z_bmqa_CloseQueueStatus__createFull(z_bmqa_CloseQueueStatus** status_obj,
                                        const z_bmqa_QueueId*     queueId,
                                        int                       result,
                                        const char* errorDescription)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    const bsl::string            errorDescription_str(errorDescription);
    bmqt::CloseQueueResult::Enum result_enum =
        static_cast<bmqt::CloseQueueResult::Enum>(result);
    bmqa::CloseQueueStatus* status_p = new bmqa::CloseQueueStatus(
        *queueId_p,
        result_enum,
        errorDescription_str);
    *status_obj = reinterpret_cast<z_bmqa_CloseQueueStatus*>(status_p);

    return 0;
}

bool z_bmqa_CloseQueueStatus__toBool(const z_bmqa_CloseQueueStatus* status_obj)
{
    using namespace BloombergLP;
    const bmqa::CloseQueueStatus* status_p =
        reinterpret_cast<const bmqa::CloseQueueStatus*>(status_obj);
    return *status_p;
}

int z_bmqa_CloseQueueStatus__queueId(const z_bmqa_CloseQueueStatus* status_obj,
                                     z_bmqa_QueueId const** queueId_obj)
{
    using namespace BloombergLP;
    const bmqa::CloseQueueStatus* status_p =
        reinterpret_cast<const bmqa::CloseQueueStatus*>(status_obj);
    const bmqa::QueueId* queueId_p = &(status_p->queueId());

    *queueId_obj = reinterpret_cast<const z_bmqa_QueueId*>(queueId_p);
    return 0;
}

int z_bmqa_CloseQueueStatus__result(const z_bmqa_CloseQueueStatus* status_obj)
{
    using namespace BloombergLP;
    const bmqa::CloseQueueStatus* status_p =
        reinterpret_cast<const bmqa::CloseQueueStatus*>(status_obj);
    return status_p->result();
}

const char* z_bmqa_CloseQueueStatus__errorDescription(
    const z_bmqa_CloseQueueStatus* status_obj)
{
    using namespace BloombergLP;
    const bmqa::CloseQueueStatus* status_p =
        reinterpret_cast<const bmqa::CloseQueueStatus*>(status_obj);
    return status_p->errorDescription().c_str();
}