#include <bmqa_messageproperties.h>
#include <bsl_string.h>
#include <z_bmqa_messageproperties.h>

const int z_bmqa_MessageProperties::k_MAX_NUM_PROPERTIES =
    BloombergLP::bmqa::MessageProperties::k_MAX_NUM_PROPERTIES;

const int z_bmqa_MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH =
    BloombergLP::bmqa::MessageProperties::k_MAX_PROPERTIES_AREA_LENGTH;

const int z_bmqa_MessageProperties::k_MAX_PROPERTY_NAME_LENGTH =
    BloombergLP::bmqa::MessageProperties::k_MAX_PROPERTY_NAME_LENGTH;

const int z_bmqa_MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH =
    BloombergLP::bmqa::MessageProperties::k_MAX_PROPERTY_VALUE_LENGTH;

int z_bmqa_MessageProperties__delete(z_bmqa_MessageProperties** properties_obj)
{
    using namespace BloombergLP;

    BSLS_ASSERT(properties_obj != NULL);

    bmqa::MessageProperties* properties_p =
        reinterpret_cast<bmqa::MessageProperties*>(*properties_obj);
    delete properties_p;
    *properties_obj = NULL;

    return 0;
}

int z_bmqa_MessageProperties__create(z_bmqa_MessageProperties** properties_obj)
{
    using namespace BloombergLP;

    bmqa::MessageProperties* properties_p = new bmqa::MessageProperties();
    *properties_obj = reinterpret_cast<z_bmqa_MessageProperties*>(
        properties_p);

    return 0;
}

int z_bmqa_MessageProperties__createCopy(
    z_bmqa_MessageProperties**      properties_obj,
    const z_bmqa_MessageProperties* other)
{
    using namespace BloombergLP;

    const bmqa::MessageProperties* other_p =
        reinterpret_cast<const bmqa::MessageProperties*>(other);
    bmqa::MessageProperties* properties_p = new bmqa::MessageProperties(
        *other_p);
    *properties_obj = reinterpret_cast<z_bmqa_MessageProperties*>(
        properties_p);

    return 0;
}

int z_bmqa_MessageProperties__setPropertyAsBool(
    z_bmqa_MessageProperties* properties_obj,
    const char*               name,
    bool                      value)
{
    using namespace BloombergLP;

    bmqa::MessageProperties* properties_p =
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj);
    bsl::string name_str(name);
    return properties_p->setPropertyAsBool(name_str, value);
}

int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj)
{
    using namespace BloombergLP;

    const bmqa::MessageProperties* properties_p =
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    return properties_p->totalSize();
}