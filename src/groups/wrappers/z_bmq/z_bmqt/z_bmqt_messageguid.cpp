#include <bmqt_messageguid.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <z_bmqt_messageguid.h>

int z_bmqt_MessageGUID__delete(z_bmqt_MessageGUID** messageGUID_obj)
{
    using namespace BloombergLP;

    BSLS_ASSERT(messageGUID_obj != NULL);

    bmqt::MessageGUID* messageGUID_p = reinterpret_cast<bmqt::MessageGUID*>(
        *messageGUID_obj);
    delete messageGUID_p;
    *messageGUID_obj = NULL;

    return 0;
}

int z_bmqt_MessageGUID__create(z_bmqt_MessageGUID** messageGUID_obj)
{
    using namespace BloombergLP;

    bmqt::MessageGUID* messageGUID_p = new bmqt::MessageGUID();
    *messageGUID_obj = reinterpret_cast<z_bmqt_MessageGUID*>(messageGUID_p);

    return 0;
}

int z_bmqt_MessageGUID__fromBinary(z_bmqt_MessageGUID*  messageGUID_obj,
                                   unsigned const char* buffer)
{
    using namespace BloombergLP;

    bmqt::MessageGUID* messageGUID_p = reinterpret_cast<bmqt::MessageGUID*>(
        messageGUID_obj);

    return 0;
}

int z_bmqt_MessageGUID__fromHex(z_bmqt_MessageGUID* messageGUID_obj,
                                const char*         buffer)
{
    using namespace BloombergLP;

    bmqt::MessageGUID* messageGUID_p = reinterpret_cast<bmqt::MessageGUID*>(
        messageGUID_obj);

    return 0;
}

bool z_bmqt_MessageGUID__isUnset(const z_bmqt_MessageGUID* messageGUID_obj)
{
    using namespace BloombergLP;

    const bmqt::MessageGUID* messageGUID_p =
        reinterpret_cast<const bmqt::MessageGUID*>(messageGUID_obj);

    return messageGUID_p->isUnset();
}

int z_bmqt_MessageGUID__toBinary(const z_bmqt_MessageGUID* messageGUID_obj,
                                 unsigned char*            destination)
{
    using namespace BloombergLP;

    const bmqt::MessageGUID* messageGUID_p =
        reinterpret_cast<const bmqt::MessageGUID*>(messageGUID_obj);

    return 0;
}

int z_bmqt_MessageGUID__toHex(const z_bmqt_MessageGUID* messageGUID_obj,
                              char*                     destination)
{
    using namespace BloombergLP;

    const bmqt::MessageGUID* messageGUID_p =
        reinterpret_cast<const bmqt::MessageGUID*>(messageGUID_obj);

    return 0;
}

int z_bmqt_MessageGUID__toString(const z_bmqt_MessageGUID* messageGUID_obj,
                                 char**                    out)
{
    using namespace BloombergLP;

    bsl::ostringstream       ss;
    const bmqt::MessageGUID* messageGUID_p =
        reinterpret_cast<const bmqt::MessageGUID*>(messageGUID_obj);
    ss << *messageGUID_p;
    bsl::string out_str = ss.str();
    *out                = new char[out_str.length() + 1];
    strcpy(*out, out_str.c_str());

    return 0;
}