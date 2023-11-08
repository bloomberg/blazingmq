#ifndef INCLUDED_Z_BMQT_URI
#define INCLUDED_Z_BMQT_URI

#if defined(__cplusplus)
extern "C" {
#endif

// =========
// class Uri
// =========

typedef struct z_bmqt_Uri z_bmqt_Uri;


int z_bmqt_Uri_create(z_bmqt_Uri** uri_obj, z_bmqt_Uri* to_copy);


// ================
// class UriBuilder
// ================

typedef struct z_bmqt_UriBuilder z_bmqt_UriBuilder;

int z_bmqt_UriBuilder__create(z_bmqt_UriBuilder** uribuilder_obj);

int z_bmqt_UriBuilder__createFromUri(z_bmqt_UriBuilder** uribuilder_obj, const z_bmqt_Uri* uri_obj);

int z_bmqt_UriBuilder__setDomain(z_bmqt_UriBuilder* uribuilder_obj, const char* value);

int z_bmqt_UriBuilder__setTier(z_bmqt_UriBuilder* uribuilder_obj, const char* value);

int z_bmqt_UriBuilder__setQualifiedDomain(z_bmqt_UriBuilder* uribuilder_obj, const char* value);

int z_bmqt_UriBuilder__setQueue(z_bmqt_UriBuilder* uribuilder_obj, const char* value);

int z_bmqt_UriBuilder_uri(z_bmqt_UriBuilder* uribuilder_obj, z_bmqt_Uri** uri_obj);


#if defined(__cplusplus)
}
#endif

#endif