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

int z_bmqa_MessageProperties__setPropertyAsChar(
    z_bmqa_MessageProperties* properties_obj,
    const char* name,
    char value)
{
    using namespace BloombergLP; 

    bmqa::MessageProperties* properties_p = 
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj); 

    bsl::string name_str(name); 
    return properties_p->setPropertyAsChar(name_str,value);
}

int z_bmqa_MessageProperties__setPropertyAsShort(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    short value)
{
    using namespace BloombergLP; 

    bmqa::MessageProperties* properties_p = 
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj); 
    
    bsl::string name_str(name);
    return properties_p->setPropertyAsShort(name_str,value); 
}

int z_bmqa_MessageProperties__setPropertyAsInt32(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    int32_t value)
{
    using namespace BloombergLP; 

    bmqa::MessageProperties* properties_p = 
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj); 
    
    bsl::string name_str(name);
    return properties_p->setPropertyAsInt32(name_str,value); 
}


int z_bmqa_MessageProperties__setPropertyAsInt64(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    long long value)
{
    using namespace BloombergLP; 

    bmqa::MessageProperties* properties_p = 
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj); 
    
    bsl::string name_str(name);
    return properties_p->setPropertyAsInt64(name_str,value); 
}

//needs rigorous testing to see if value conversion to bsl::string is needed, or we use the char* value 
int z_bmqa_MessageProperties__setPropertyAsString(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value)
{
    using namespace BloombergLP; 

    bmqa::MessageProperties* properties_p = 
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj); 
    
    bsl::string name_str(name);
    bsl::string val_str(value); 
    return properties_p->setPropertyAsString(name_str,val_str); 
}

int z_bmqa_MessageProperties__setPropertyAsBinary(
    z_bmqa_MessageProperties* properties_obj, 
    const char* name, 
    const char* value,
    int size)
{
    using namespace BloombergLP; 

    bmqa::MessageProperties* properties_p = 
        reinterpret_cast<bmqa::MessageProperties*>(properties_obj); 
    
    bsl::string name_str(name);
    bsl::vector<char> val_vec(value, value+size);
    return properties_p->setPropertyAsBinary(name_str,val_vec); 
}


int z_bmqa_MessageProperties__numProperties(
    const z_bmqa_MessageProperties* properties_obj) 
{
    using namespace BloombergLP; 

    const bmqa::MessageProperties* properties_p = 
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj); 
    
    return properties_p->numProperties(); 
}

int z_bmqa_MessageProperties__totalSize(
    const z_bmqa_MessageProperties* properties_obj)
{
    using namespace BloombergLP;

    const bmqa::MessageProperties* properties_p =
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    return properties_p->totalSize();
}

bool z_bmqa_MessageProperties__getPropertyAsBool(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name)
{
    using namespace BloombergLP; 

    const bmqa::MessageProperties* properties_p = 
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    
    bsl::string name_str(name);
    return properties_p->getPropertyAsBool(name_str);
}

char z_bmqa_MessageProperties__getPropertyAsChar(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name)
{
    using namespace BloombergLP; 

    const bmqa::MessageProperties* properties_p = 
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);

    bsl::string name_str(name);
    return properties_p->getPropertyAsChar(name_str);
}

short z_bmqa_MessageProperties__getPropertyAsShort(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name) 
{
    using namespace BloombergLP;

    const bmqa::MessageProperties* properties_p = 
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    
    bsl::string name_str(name);
    return properties_p->getPropertyAsShort(name_str);
}

int32_t z_bmqa_MessageProperties__getPropertyAsInt32(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name)
{
    using namespace BloombergLP; 

    const bmqa::MessageProperties* properties_p = 
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    
    bsl::string name_str(name);
    return properties_p->getPropertyAsInt32(name_str);
}

long long z_bmqa_MessageProperties__getPropertyAsInt64(
    const z_bmqa_MessageProperties* properties_obj, 
    const char* name)
{
    using namespace BloombergLP; 

    const bmqa::MessageProperties* properties_p = 
        reinterpret_cast<const bmqa::MessageProperties*>(properties_obj);
    
    bsl::string name_str(name);
    return properties_p->getPropertyAsInt64(name_str);
}