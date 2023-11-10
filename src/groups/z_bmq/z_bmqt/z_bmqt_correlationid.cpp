#include <bmqt_correlationid.h>
#include <z_bmqt_correlationid.h>

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj) {
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_ptr = new bmqt::CorrelationId();

    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(correlationId_ptr);
    return 0;
}

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj, int64_t numeric) {
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_ptr = new bmqt::CorrelationId(numeric);

    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(correlationId_ptr);
    return 0;
}

int z_bmqt_CorrelationId__create(z_bmqt_CorrelationId** correlationId_obj, void* pointer) {
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_ptr = new bmqt::CorrelationId(pointer);

    *correlationId_obj = reinterpret_cast<z_bmqt_CorrelationId*>(correlationId_ptr);
    return 0;
}

int z_bmqt_CorrelationId__makeUnset(z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<bmqt::CorrelationId*>(correlationId_obj);

    correlationId_ptr->makeUnset();
    return 0;
}

int z_bmqt_CorrelationId__setNumeric(z_bmqt_CorrelationId* correlationId_obj, int64_t numeric) {
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<bmqt::CorrelationId*>(correlationId_obj);

    correlationId_ptr->setNumeric(numeric);
    return 0;
}

int z_bmqt_CorrelationId__setPointer(z_bmqt_CorrelationId* correlationId_obj, void* pointer) {
    using namespace BloombergLP;
    bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<bmqt::CorrelationId*>(correlationId_obj);


    correlationId_ptr->setPointer(pointer);
    return 0;
}

int z_bmqt_CorrelationId__isUnset(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->isUnset();
}

int z_bmqt_CorrelationId__isNumeric(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->isNumeric();
}

int z_bmqt_CorrelationId__isPointer(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->isPointer();
}

int z_bmqt_CorrelationId__isSharedPtr(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->isSharedPtr();
}

int z_bmqt_CorrelationId__isAutoValue(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->isAutoValue();
}

int64_t z_bmqt_CorrelationId__theNumeric(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->theNumeric();
}

void* z_bmqt_CorrelationId__thePointer(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    return correlationId_ptr->thePointer();
}

CorrelationId_Type z_bmqt_CorrelationId__type(const z_bmqt_CorrelationId* correlationId_obj) {
    using namespace BloombergLP;
    const bmqt::CorrelationId* correlationId_ptr = reinterpret_cast<const bmqt::CorrelationId*>(correlationId_obj);

    switch(correlationId_ptr->type()) {
        case bmqt::CorrelationId::Type::e_NUMERIC: return CorrelationId_Type::e_NUMERIC;
        case bmqt::CorrelationId::Type::e_POINTER: return CorrelationId_Type::e_POINTER;
        case bmqt::CorrelationId::Type::e_SHARED_PTR: return CorrelationId_Type::e_SHARED_PTR;
        case bmqt::CorrelationId::Type::e_AUTO_VALUE: return CorrelationId_Type::e_AUTO_VALUE;
        case bmqt::CorrelationId::Type::e_UNSET: return CorrelationId_Type::e_UNSET;
        default: break;
    }

    return CorrelationId_Type::e_CORRELATIONID_ERROR;
}