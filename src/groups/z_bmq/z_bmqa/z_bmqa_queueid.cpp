#include <bmqa_queueid.h>
#include <bmqt_correlationid.h>
#include <z_bmqa_queueid.h>

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj) {
    using namespace BloombergLP;

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId();

    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, const z_bmqt_CorrelationId* correlationId) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId);
    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(*correlationId_ptr);

    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, const z_bmqa_QueueId* other) {
    using namespace BloombergLP;

    const bmqa::QueueId* other_ptr = reinterpret_cast<const bmqa::QueueId*>(other);

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(*other_ptr);
    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, int64_t numeric) {
    using namespace BloombergLP;

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(numeric);
    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj, void* pointer) {
    using namespace BloombergLP;

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(pointer);
    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}