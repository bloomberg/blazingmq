#include <bmqt_correlationid.h>
#include <z_bmqt_correlationid.h>

int z_bmqt_CorrelationId__delete(z_bmqt_CorrelationId** correlationId_obj)
{
    using namespace BloombergLP;

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

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj,
                                 int64_t                numeric)
{
    using namespace BloombergLP;

    bmqt::CorrelationId* correlationId_p = new bmqt::CorrelationId(numeric);
    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(
        correlationId_p);
    return 0;
}

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj,
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

int z_bmqt_CorrelationId__isUnset(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isUnset();
}

int z_bmqt_CorrelationId__isNumeric(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isNumeric();
}

int z_bmqt_CorrelationId__isPointer(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isPointer();
}

int z_bmqt_CorrelationId__isSharedPtr(
    const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_p->isSharedPtr();
}

int z_bmqt_CorrelationId__isAutoValue(
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

CorrelationId_Type
z_bmqt_CorrelationId__type(const z_bmqt_CorrelationId* correlationId_obj)
{
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_p =
        reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    switch (correlationId_p->type()) {
    case bmqt::CorrelationId::Type::e_NUMERIC:
        return CorrelationId_Type::ec_NUMERIC;
    case bmqt::CorrelationId::Type::e_POINTER:
        return CorrelationId_Type::ec_POINTER;
    case bmqt::CorrelationId::Type::e_SHARED_PTR:
        return CorrelationId_Type::ec_SHARED_PTR;
    case bmqt::CorrelationId::Type::e_AUTO_VALUE:
        return CorrelationId_Type::ec_AUTO_VALUE;
    case bmqt::CorrelationId::Type::e_UNSET:
        return CorrelationId_Type::ec_UNSET;
    default: break;
    }

    return CorrelationId_Type::ec_CORRELATIONID_ERROR;
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