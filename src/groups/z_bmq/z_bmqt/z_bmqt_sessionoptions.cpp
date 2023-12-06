#include <bmqt_sessionoptions.h>
#include <z_bmqt_sessionoptions.h>


int z_bmqt_SessionOptions__delete(z_bmqt_SessionOptions** options_obj) {
    using namespace BloombergLP;

    bmqt::SessionOptions* options_p = reinterpret_cast<bmqt::SessionOptions*>(*options_obj);
    delete options_p;
    *options_obj = NULL;

    return 0;
}

int z_bmqt_SessionOptions__create(z_bmqt_SessionOptions** options_obj) {
    using namespace BloombergLP;
    bmqt::SessionOptions* options_p = new bmqt::SessionOptions();
    *options_obj = reinterpret_cast<z_bmqt_SessionOptions*>(options_p);
    return 0;
}

const char* z_bmqt_SessionOptions__brokerUri(const z_bmqt_SessionOptions* options_obj) {
    using namespace BloombergLP;

    const bmqt::SessionOptions* options_p = reinterpret_cast<const bmqt::SessionOptions*>(options_obj);

    return options_p->brokerUri().c_str();
}
