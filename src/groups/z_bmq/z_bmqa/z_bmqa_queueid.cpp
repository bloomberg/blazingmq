#include <bmqa_queueid.h>
#include <bmqt_correlationid.h>
#include <z_bmqa_queueid.h>

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId_obj) {
    using namespace BloombergLP;

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId();

    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__createCopy(z_bmqa_QueueId** queueId_obj, const z_bmqa_QueueId* other) {
    using namespace BloombergLP;

    const bmqa::QueueId* other_ptr = reinterpret_cast<const bmqa::QueueId*>(other);

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(*other_ptr);
    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__createFromCorrelationId(z_bmqa_QueueId** queueId_obj, const z_bmqt_CorrelationId* correlationId) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId);
    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(*correlationId_ptr);

    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__createFromNumeric(z_bmqa_QueueId** queueId_obj, int64_t numeric) {
    using namespace BloombergLP;

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(numeric);
    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

int z_bmqa_QueueId__createFromPointer(z_bmqa_QueueId** queueId_obj, void* pointer) {
    using namespace BloombergLP;

    bmqa::QueueId* queueId_ptr = new bmqa::QueueId(pointer);
    *queueId_obj = reinterpret_cast<z_bmqa_QueueId*>(queueId_ptr);

    return 0;
}

const z_bmqt_CorrelationId* z_bmqa_QueueId__correlationId(const z_bmqa_QueueId* queueId_obj) {
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_ptr = reinterpret_cast<const bmqa::QueueId*>(queueId_obj);
    const bmqt::CorrelationId* correlationId_ptr = &(queueId_ptr->correlationId());

    return reinterpret_cast<const z_bmqt_CorrelationId*>(correlationId_ptr);
}

uint64_t z_bmqa_QueueId__flags(const z_bmqa_QueueId* queueId_obj) {
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_ptr = reinterpret_cast<const bmqa::QueueId*>(queueId_obj);
    return queueId_ptr->flags();
}

const z_bmqt_Uri* z_bmqa_QueueId__uri(const z_bmqa_QueueId* queueId_obj) {
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_ptr = reinterpret_cast<const bmqa::QueueId*>(queueId_obj);
    const bmqt::Uri* uri_ptr = &(queueId_ptr->uri());

    return reinterpret_cast<const z_bmqt_Uri*>(uri_ptr);

}

const z_bmqt_QueueOptions* options(const z_bmqa_QueueId* queueId_obj) {
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_ptr = reinterpret_cast<const bmqa::QueueId*>(queueId_obj);
    const bmqt::QueueOptions* options_ptr = &(queueId_ptr->options());

    return reinterpret_cast<const z_bmqt_QueueOptions*>(options_ptr);
}

int z_bmqa_QueueId__isValid(const z_bmqa_QueueId* queueId_obj) {
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_ptr = reinterpret_cast<const bmqa::QueueId*>(queueId_obj);
    return queueId_ptr->isValid();
}

