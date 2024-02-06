#include <bmqt_correlationid.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <z_bmqt_correlationid.h>

int z_bmqt_CorrelationId__delete(z_bmqt_CorrelationId** correlationId_obj)
{
    using namespace BloombergLP;

    BSLS_ASSERT(correlationId_obj != NULL);

    bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<bmqt::CorrelationId*>(*correlationId_obj);
    delete correlationId_p;
    *correlationId_obj = NULL;

    return 0;
}

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj)
{
    using namespace BloombergLP;

    bmqt::CorrelationId* correlationId_p = new bmqt::CorrelationId();
    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(
        correlationId_p);
    return 0;
}

int z_bmqt_CorrelationId__createCopy(z_bmqt_CorrelationId** correlationId_obj,
                                     const z_bmqt_CorrelationId* other_obj)
{
    using namespace BloombergLP;

    const bmqt::CorrelationId* other_p =
        reinterpret_cast<const bmqt::CorrelationId*>(other_obj);
    bmqt::CorrelationId* correlationId_p = new bmqt::CorrelationId(*other_p);
    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(
        correlationId_p);

    return 0;
}

int z_bmqt_CorrelationId__createFromNumeric(
    z_bmqt_CorrelationId** correlationId_obj,
    int64_t                numeric)
{
    using namespace BloombergLP;

    bmqt::CorrelationId* correlationId_p = new bmqt::CorrelationId(numeric);
    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(
        correlationId_p);
    return 0;
}

int z_bmqt_CorrelationId__createFromPointer(
    z_bmqt_CorrelationId** correlationId_obj,
    void*                  pointer)
{
    using namespace BloombergLP;

    bmqt::CorrelationId* correlationId_p = new bmqt::CorrelationId(pointer);
    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(
        correlationId_p);
    return 0;
}

int z_bmqt_CorrelationId__makeUnset(z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<bmqt::CorrelationId*>(correlationId_obj);

    correlationId_p->makeUnset();
    return 0;
}

int z_bmqt_CorrelationId__setNumeric(z_bmqt_CorrelationId* correlationId_obj,
                                     int64_t               numeric)
{
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<bmqt::CorrelationId*>(correlationId_obj);

    correlationId_p->setNumeric(numeric);
    return 0;
}

int z_bmqt_CorrelationId__setPointer(z_bmqt_CorrelationId* correlationId_obj,
                                     void*                 pointer)
{
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<bmqt::CorrelationId*>(correlationId_obj);

    correlationId_p->setPointer(pointer);
    return 0;
}

bool z_bmqt_CorrelationId__isUnset(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isUnset();
}

bool z_bmqt_CorrelationId__isNumeric(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isNumeric();
}

bool z_bmqt_CorrelationId__isPointer(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isPointer();
}

bool z_bmqt_CorrelationId__isAutoValue(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isAutoValue();
}

int64_t
z_bmqt_CorrelationId__theNumeric(const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->theNumeric();
}

void* z_bmqt_CorrelationId__thePointer(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->thePointer();
}

z_bmqt_CorrelationId::Type
z_bmqt_CorrelationId__type(const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return static_cast<z_bmqt_CorrelationId::Type>(correlationId_p->type());
}

int z_bmqt_CorrelationId__autoValue(z_bmqt_CorrelationId** correlationId_obj)
{
    using namespace BloombergLP;

    bmqt::CorrelationId* correlationId_p = new bmqt::CorrelationId(
        bmqt::CorrelationId::autoValue());
    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(
        correlationId_p);

    return 0;
}

int z_bmqt_CorrelationId__compare(const z_bmqt_CorrelationId* a,
                                  const z_bmqt_CorrelationId* b)
{
    using namespace BloombergLP;

    const bmqt::CorrelationId* a_p =
        reinterpret_cast<const bmqt::CorrelationId*>(a);
    const bmqt::CorrelationId* b_p =
        reinterpret_cast<const bmqt::CorrelationId*>(b);

    if (*a_p == *b_p) {
        return 0;
    }

    return *a_p < *b_p ? -1 : 1;
}

int z_bmqt_CorrelationId__assign(z_bmqt_CorrelationId**      dst,
                                 const z_bmqt_CorrelationId* src)
{
    using namespace BloombergLP;

    bmqt::CorrelationId* dst_p = reinterpret_cast<bmqt::CorrelationId*>(*dst);
    const bmqt::CorrelationId* src_p =
        reinterpret_cast<const bmqt::CorrelationId*>(src);

    *dst_p = *src_p;

    return 0;
}

int z_bmqt_CorrelationId__toString(
    const z_bmqt_CorrelationId* correlationId_obj,
    char**                      out)
{
    using namespace BloombergLP;

    bsl::ostringstream         ss;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);
    ss << *correlationId_p;
    bsl::string out_str = ss.str();
    *out                = new char[out_str.length() + 1];
    strcpy(*out, out_str.c_str());

    return 0;
}