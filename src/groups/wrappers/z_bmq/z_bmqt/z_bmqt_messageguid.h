#ifndef INCLUDED_Z_BMQT_MESSAGEGUID
#define INCLUDED_Z_BMQT_MESSAGEGUID

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct z_bmqt_MessageGUID z_bmqt_MessageGUID;

int z_bmqt_MessageGUID__delete(z_bmqt_MessageGUID** messageGUID_obj);

int z_bmqt_MessageGUID__create(z_bmqt_MessageGUID** messageGUID_obj);

int z_bmqt_MessageGUID__fromBinary(z_bmqt_MessageGUID*  messageGUID_obj,
                                   const unsigned char* buffer);

int z_bmqt_MessageGUID__fromHex(z_bmqt_MessageGUID* messageGUID_obj,
                                const char*         buffer);

bool z_bmqt_MessageGUID__isUnset(const z_bmqt_MessageGUID* messageGUID_obj);

int z_bmqt_MessageGUID__toBinary(const z_bmqt_MessageGUID* messageGUID_obj,
                                 unsigned char*            destination);

int z_bmqt_MessageGUID__toHex(const z_bmqt_MessageGUID* messageGUID_obj,
                              char*                     destination);

int z_bmqt_MessageGUID__toString(const z_bmqt_MessageGUID* messageGUID_obj,
                                 char**                    out);

#if defined(__cplusplus)
}
#endif

#endif