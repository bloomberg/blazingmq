#include <bmqt_uri.h>
#include <z_bmqt_uri.h>

int z_bmqt_Uri_create(z_bmqt_Uri** uri_obj, z_bmqt_Uri* to_copy){
    using namespace BloombergLP;
    bmqt::Uri* original_uri_ptr = reinterpret_cast<bmqt::Uri*>(to_copy);
    bmqt::Uri* copy  = new bmqt::Uri(*original_uri_ptr);
    *uri_obj = reinterpret_cast<z_bmqt_Uri*>(copy);
    return 0;
}


//URIBUILDER


int z_bmqt_UriBuilder__create(z_bmqt_UriBuilder** uribuilder_obj) {
    using namespace BloombergLP;
    bmqt::UriBuilder* uriBuilder_ptr  = new bmqt::UriBuilder();
    *uribuilder_obj = reinterpret_cast<z_bmqt_UriBuilder*>(uriBuilder_ptr);
    return 0;
}

int z_bmqt_UriBuilder__createFromUri(z_bmqt_UriBuilder** uribuilder_obj, const z_bmqt_Uri* uri_obj) {
    using namespace BloombergLP;
    const bmqt::Uri* uri_ptr = reinterpret_cast<const bmqt::Uri*>(uri_obj);
    bmqt::UriBuilder* uriBuilder_ptr  = new bmqt::UriBuilder(*uri_ptr);
    *uribuilder_obj = reinterpret_cast<z_bmqt_UriBuilder*>(uriBuilder_ptr);
    return 0;
}

int z_bmqt_UriBuilder__setDomain(z_bmqt_UriBuilder* uribuilder_obj, const char* value){
    using namespace BloombergLP;
    bmqt::UriBuilder* uriBuilder_ptr = reinterpret_cast<bmqt::UriBuilder*>(uribuilder_obj);
    uriBuilder_ptr->setDomain(value);
    return 0;
}

int z_bmqt_UriBuilder__setTier(z_bmqt_UriBuilder* uribuilder_obj, const char* value){
    using namespace BloombergLP;
    bmqt::UriBuilder* uriBuilder_ptr = reinterpret_cast<bmqt::UriBuilder*>(uribuilder_obj);
    uriBuilder_ptr->setTier(value);
    return 0;
}

int z_bmqt_UriBuilder__setQualifiedDomain(z_bmqt_UriBuilder* uribuilder_obj, const char* value){
    using namespace BloombergLP;
    bmqt::UriBuilder* uriBuilder_ptr = reinterpret_cast<bmqt::UriBuilder*>(uribuilder_obj);
    uriBuilder_ptr->setQualifiedDomain(value);
    return 0;
}

int z_bmqt_UriBuilder__setQueue(z_bmqt_UriBuilder* uribuilder_obj, const char* value){
    using namespace BloombergLP;
    bmqt::UriBuilder* uriBuilder_ptr = reinterpret_cast<bmqt::UriBuilder*>(uribuilder_obj);
    uriBuilder_ptr->setQueue(value);
    return 0;
}

int z_bmqt_UriBuilder_uri(z_bmqt_UriBuilder* uribuilder_obj, z_bmqt_Uri** uri_obj){
    using namespace BloombergLP;
    const bmqt::UriBuilder* uriBuilder_ptr = reinterpret_cast<bmqt::UriBuilder*>(uri_obj);
    bmqt::Uri* uri_ptr = new bmqt::Uri();

   int  res =  uriBuilder_ptr->uri(uri_ptr); // support error description ?
    if(res != 0){
        delete uri_ptr;
        return res;
    }

    *uri_obj = reinterpret_cast<z_bmqt_Uri*>(uri_ptr);
    return res;
}