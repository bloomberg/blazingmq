#include <bmqt_sessionoptions.h>
#include <z_bmqt_sessionoptions.h>



namespace BloomberLP {
int z_bmqt_SessionOptions__create(z_bmqt_SessionOptions* options) {
    BloombergLP::bmqt::SessionOptions* options_ptr = new BloombergLP::bmqt::SessionOptions();
    *options = reinterpret_cast<z_bmqt_SessionOptions>(options_ptr);
    return 0;
}

};
