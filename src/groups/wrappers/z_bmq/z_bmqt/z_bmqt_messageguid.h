#ifndef INCLUDED_Z_BMQT_MESSAGEGUID
#define INCLUDED_Z_BMQT_MESSAGEGUID

typedef struct z_bmqt_MessageGUID z_bmqt_MessageGUID;

int z_bmqt_MessageGUID__delete(z_bmqt_MessageGUID** messageGUID_obj);

int z_bmqt_MessageGUID__create(z_bmqt_MessageGUID** messageGUID_obj);

int z_bmqt_MessageGUID__toString(const z_bmqt_MessageGUID* messageGUID_obj,
                                 char**                    out);

#endif