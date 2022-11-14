#include "stringconsts.h"
#include <rpm/rpmtag.h>

struct tagtype_t
{
    int Id;
    const char* const Name;
};

//==========================================================

static tagtype_t TagTypes[] = {
#include "tagtypes.gen.cpp"
};

static size_t TagTypeCount = sizeof(TagTypes)/sizeof(TagTypes[0]);

std::string TagTypeName(int Id)
{
    for (size_t i=0; i<TagTypeCount; i++)
        if (TagTypes[i].Id == Id)
            return TagTypes[i].Name;
    return "UNKNOWN";
}

//==========================================================

static tagtype_t Tags[] = {
#include "tags.gen.cpp"
};

static size_t TagCount = sizeof(Tags)/sizeof(Tags[0]);

std::string TagName(int Id)
{
    for (size_t i=0; i<TagCount; i++)
        if (Tags[i].Id == Id)
            return Tags[i].Name;
    return "UNKNOWN";
}

//==========================================================

static tagtype_t SigTags[] = {
    { HEADER_SIGNATURES, "SIGNATURES" },
#include "sigtags.gen.cpp"
};

static size_t SigTagCount = sizeof(SigTags)/sizeof(SigTags[0]);

std::string SigTagName(int Id)
{
    for (size_t i=0; i<SigTagCount; i++)
        if (SigTags[i].Id == Id)
            return SigTags[i].Name;
    return "UNKNOWN";
}
