#include <bmqt_messageguid.h>
#include <z_bmqt_messageguid.h>

int z_bmqt_MessageGUID__delete(z_bmqt_MessageGUID** messageGUID_obj)
{
    using namespace BloombergLP;

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