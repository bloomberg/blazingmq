#include <bmqt_sessionoptions.h>
#include <z_bmqt_sessionoptions.h>


int z_bmqt_SessionOptions__create(z_bmqt_SessionOptions** options_obj) {
    using namespace BloombergLP;
    bmqt::SessionOptions* options_ptr = new bmqt::SessionOptions();
    *options_obj = reinterpret_cast<z_bmqt_SessionOptions*>(options_ptr);
    return 0;
}

const char* z_bmqt_SessionOptions__brokerUri(const z_bmqt_SessionOptions* options_obj) {
    using namespace BloombergLP;

    const bmqt::SessionOptions* options_ptr = reinterpret_cast<const bmqt::SessionOptions*>(options_obj);

    return options_ptr->brokerUri().c_str();
}
