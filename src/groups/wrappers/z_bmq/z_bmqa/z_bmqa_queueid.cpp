#include <bmqa_queueid.h>
#include <bmqt_correlationid.h>
#include <z_bmqa_queueid.h>

int z_bmqa_QueueId__delete(z_bmqa_QueueId** queueId)
{
    using namespace BloombergLP;

    BSLS_ASSERT(queueId != NULL);

    bmqa::QueueId* queueId_p = reinterpret_cast<bmqa::QueueId*>(*queueId);
    delete queueId_p;
    *queueId = NULL;

    return 0;
}

int z_bmqa_QueueId__create(z_bmqa_QueueId** queueId)
{
    using namespace BloombergLP;

    bmqa::QueueId* queueId_p = new bmqa::QueueId();

    *queueId = reinterpret_cast<z_bmqa_QueueId*>(queueId_p);

    return 0;
}

int z_bmqa_QueueId__createCopy(z_bmqa_QueueId**      queueId,
                               const z_bmqa_QueueId* other)
{
    using namespace BloombergLP;

    const bmqa::QueueId* other_p = reinterpret_cast<const bmqa::QueueId*>(
        other);

    bmqa::QueueId* queueId_p = new bmqa::QueueId(*other_p);
    *queueId             = reinterpret_cast<z_bmqa_QueueId*>(queueId_p);

    return 0;
}

int z_bmqa_QueueId__createFromCorrelationId(
    z_bmqa_QueueId**            queueId,
    const z_bmqt_CorrelationId* correlationId)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId);
    bmqa::QueueId* queueId_p = new bmqa::QueueId(*correlationId_p);

    *queueId = reinterpret_cast<z_bmqa_QueueId*>(queueId_p);

    return 0;
}

int z_bmqa_QueueId__createFromNumeric(z_bmqa_QueueId** queueId,
                                      int64_t          numeric)
{
    using namespace BloombergLP;

    bmqa::QueueId* queueId_p = new bmqa::QueueId(numeric);
    *queueId             = reinterpret_cast<z_bmqa_QueueId*>(queueId_p);

    return 0;
}

int z_bmqa_QueueId__createFromPointer(z_bmqa_QueueId** queueId,
                                      void*            pointer)
{
    using namespace BloombergLP;

    bmqa::QueueId* queueId_p = new bmqa::QueueId(pointer);
    *queueId             = reinterpret_cast<z_bmqa_QueueId*>(queueId_p);

    return 0;
}

const z_bmqt_CorrelationId*
z_bmqa_QueueId__correlationId(const z_bmqa_QueueId* queueId)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    const bmqt::CorrelationId* correlationId_p = &(queueId_p->correlationId());

    return reinterpret_cast<const z_bmqt_CorrelationId*>(correlationId_p);
}

uint64_t z_bmqa_QueueId__flags(const z_bmqa_QueueId* queueId)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    return queueId_p->flags();
}

const z_bmqt_Uri* z_bmqa_QueueId__uri(const z_bmqa_QueueId* queueId)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    const bmqt::Uri* uri_p = &(queueId_p->uri());

    return reinterpret_cast<const z_bmqt_Uri*>(uri_p);
}

const z_bmqt_QueueOptions* options(const z_bmqa_QueueId* queueId)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    const bmqt::QueueOptions* options_p = &(queueId_p->options());

    return reinterpret_cast<const z_bmqt_QueueOptions*>(options_p);
}

int z_bmqa_QueueId__isValid(const z_bmqa_QueueId* queueId)
{
    using namespace BloombergLP;

    const bmqa::QueueId* queueId_p = reinterpret_cast<const bmqa::QueueId*>(
        queueId);
    return queueId_p->isValid();
}
