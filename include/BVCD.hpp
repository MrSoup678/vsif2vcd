#pragma once

#include "pch.hpp"
#include "helper.hpp"
#include <typeinfo>
#include "enum_bitmask.hpp"

#include "VSIFStringPool.hpp"
#include <LzmaLib.h>


#include <sstream>


namespace BVCD {
    using namespace Helper;
    static  constexpr float One255th = 0.003922f;
    static  constexpr float One4096th = 0.000244f;
    enum Event_Type{
		Event_Unspecified,
		Event_Section, Event_Expression, Event_LookAt, Event_MoveTo,
		Event_Speak, Event_Gesture, Event_Sequence, Event_Face,
		Event_FireTrigger, Event_FlexAnimation, Event_SubScene, Event_Loop,
        Event_Interrupt, Event_StopPoint, Event_PermitResponses, Event_Generic,
        Event_Invalid =-1
    } ;
    static  const char Event_TypeNames[17][16] = {
		"unspecified",
		"section", "expression", "lookat", "moveto",
		"speak", "gesture", "sequence", "face",
		"firetrigger", "flexanimation", "subscene", "loop",
		"interrupt", "stoppoint", "permitresponses", "generic"
	};
    static  char Interpolators[16][23] = {
        "default", "catmullrom_normalize_x", "easein", "easeout",
        "easeinout", "bspline", "linear_interp", "kochanek",
        "kochanek_early", "kochanek_late", "simple_cubic", "catmullrom",
        "catmullrom_normalize", "catmullrom_tangent", "exponential_decay", "hold"
    };
    static inline std::string eventTypeToString(Event_Type event){
        return Event_TypeNames[event];
    };

    static inline Event_Type eventTypeFromString(std::string event) {
        for (int i=0;i<17;i++) {
            if (event == Event_TypeNames[i]){
                return (Event_Type)i;
            }
        }
        return Event_Invalid;
    }

    //

    enum class VCD_Flags : unsigned char
    {
        resumeCondition=1<<0,
        lockBodyFacing=1<<1,
        fixedLength=1<<2,
        isActive=1<<3,
        forceShortMovement=1<<4,
        playOverScript=1<<5,
        syncToFollowingGesture=1<<6,
    };
    enum class TrackFlags : unsigned char
    {
        isEnabled=1<<0,
        isCombo=1<<1,
    };
    enum class VCD_CC_Flags: unsigned char
    {
        usingCombinedFile=1<<0,
        combinedUsingGenderToken=1<<1,
        suppressingCaptionAttenuation=1<<2,
    };
    enum class VCD_AbsTagType : unsigned char
    {
        playback=0,
        shifted=1
    };
    enum class VCD_CC_Type: unsigned char
    {
        master=0,
        slave=1,
        disabled=2,
    };

}
ENABLE_BITMASK_OPERATORS(BVCD::VCD_Flags);
ENABLE_BITMASK_OPERATORS(BVCD::VCD_CC_Flags);
ENABLE_BITMASK_OPERATORS(BVCD::TrackFlags);

namespace BVCD {

struct VCD_Sample {
    float time;
    float value;


    void dumpText(std::string& stream);
};

template <typename S>
void serialize(S& s, VCD_Sample& sample);


struct VCD_Ramp { // CCurveData
    //ramp name is event in VCD_Event and scene in VCD
    std::vector<VCD_Sample> samples;

    void dumpText(std::string& stream,bool inEvent);

};
template <typename S>
void serialize(S& s, VCD_Ramp& ramp);
struct VCD_CC {
    VCD_CC_Type type;
    std::string cc_token;
    VCD_CC_Flags flags;

    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, VCD_CC& subtitles);
struct Flex_Samples {
    float time;
    float value;
    std::string fromCurve; //left
    std::string toCurve; //right


    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, Flex_Samples& sample);
struct Flex_Tracks {
    std::string name;
    TrackFlags flags;
    float minRange=0.0,maxRange=1.0;//(0.0,1.0)
    std::vector<Flex_Samples> samples;
    std::vector<Flex_Samples> comboSamples;

    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, Flex_Tracks& track);

struct VCD_EventFlex {
    std::vector<Flex_Tracks> tracks;


    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, VCD_EventFlex& flex);

struct VCD_RelTags { //tags
    std::string name;
    float duration;

    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, VCD_RelTags& tags);
struct VCD_RelTag { //relativetag
    std::string name;
    std::string wavName;

    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, VCD_RelTag& tag) {
    uint16_t shortTmp;
    s.value2b(shortTmp);
    tag.name= Helper::VsifStringPool->getStringByID(shortTmp);
    s.value2b(shortTmp);
    tag.wavName= Helper::VsifStringPool->getStringByID(shortTmp);

}
struct VCD_FlexTimingTags {

    std::string name;
    float duration;


    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, VCD_FlexTimingTags& tags);
struct VCD_AbsTags {
    VCD_AbsTagType type;
    std::string name;
    float duration;



    void dumpText(std::string& stream);
};
template <typename S>
void serialize(S& s, VCD_AbsTags& tags);

    struct VCD_Event {
        Event_Type eventType;
        std::string name;
        float eventStart,eventEnd;
        std::string param1,param2,param3;
        VCD_Ramp ramp;
        VCD_Flags flags;
        float distanceToTarget;
        std::vector<VCD_RelTags> relativeTags;
        std::vector<VCD_FlexTimingTags> flexTimingTags;
        std::vector<VCD_AbsTags> absoluteTags;
        float sequenceDuration =-1.0f; //not present outside of gesture event
        bool usingRelativetag;
        VCD_RelTag relativeTag;
        VCD_EventFlex flex;
        uint8_t loopCount;
        VCD_CC closeCaptions;



        void dumpText(std::string& stream);

    };

    template <typename S>
    void serialize(S& s, VCD_Event& e);

    struct VCD_Channel {
        std::string name;
        std::vector<VCD_Event> events;
        bool isActive;

        void dumpText(std::string& stream);
    };
    template <typename S>
    void serialize(S& s, VCD_Channel& c);


    struct VCD_Actor {
        std::string name;
        std::vector<VCD_Channel> channels;
        bool isActive;

        void dumpText(std::string& stream);

    };
    template <typename S>
    void serialize(S& s, VCD_Actor& a);


    class VCD {
    public:
        uint32_t magic; // bvcd
        uint8_t version;
        uint32_t CRC;
        std::vector<VCD_Event> events;
        std::vector<VCD_Actor> actors;
        VCD_Ramp ramp;
        bool ignorePhonemes;


        std::string dumpText();

    };

    template <typename S>
    void serialize(S& s, VCD& vcd);






    class CompressedVCD {
    public:
        uint32_t magic; //LZMA
        uint32_t realSize;
        uint32_t compressedSize;
        unsigned char properties[5];
        std::vector<char> compressedBuffer;
    };

    //template <typename S>
    template <typename S>
    void serialize(S& s, CompressedVCD& cVCD);


struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }

    template <typename Iterator=std::vector<char>::iterator>
    membuf(Iterator begin, Iterator end) {
        this->setg(begin.base(),begin.base(),end.base());
    }
};


VCD getSceneFromBuffer(std::vector<char> buffer);









}



template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_Sample& sample) {
    uint8_t byteTmp;
    s.value4b(sample.time);

    s.value1b(byteTmp);
    sample.value= byteTmp *One255th;

}

template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_Ramp& ramp) {
    uint8_t byteTmp;
    s.value1b(byteTmp);
    ramp.samples.resize(byteTmp);
    for (int i=0;i<byteTmp;i++)
        s.object(ramp.samples[i]);

}

template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_CC& subtitles) {
    uint8_t byteTmp;

    s.value1b(byteTmp);
    subtitles.type=(BVCD::VCD_CC_Type)byteTmp;
    uint16_t shortTmp;
    s.value2b(shortTmp);
    subtitles.cc_token= Helper::VsifStringPool->getStringByID(shortTmp);
    s.value1b(byteTmp);
    subtitles.flags=(BVCD::VCD_CC_Flags)byteTmp;
}


template <typename S>
void BVCD::serialize(S& s, BVCD::Flex_Samples& sample) {
    uint8_t byteTmp;
    s.value4b(sample.time);

    s.value1b(byteTmp);
    sample.value = byteTmp * BVCD::One255th;
    uint16_t shortTmp;
    s.value2b(shortTmp);
    //now split to two chars
    uint8_t startCurve = (shortTmp >> 8) & 0xff;
    uint8_t endcurve = (shortTmp) & 0xff;
    sample.fromCurve = BVCD::Interpolators[startCurve];
    sample.toCurve = BVCD::Interpolators[endcurve];
    /*
    s.value1b(charTmp);
    sample.toCurve = BVCD::Interpolators[charTmp];
    s.value1b(charTmp);
    sample.fromCurve = BVCD::Interpolators[charTmp];
    */
}

template <typename S>
void BVCD::serialize(S& s, BVCD::Flex_Tracks& track) {
    uint16_t shortTmp;
    s.value2b(shortTmp);
    track.name= Helper::VsifStringPool->getStringByID(shortTmp);
    uint8_t byteTmp;
    s.value1b(byteTmp);
    track.flags=(BVCD::TrackFlags)byteTmp;
    s.value4b(track.minRange);
    s.value4b(track.maxRange);
    //assert(track.minRange>=0&&track.maxRange<=1); //this assertion is ill-formed
    s.value2b(shortTmp);
    track.samples.resize(shortTmp);
    for (int i=0;i<shortTmp;i++)
        s.object(track.samples[i]);
    if ((int)(track.flags & TrackFlags::isCombo)){

        s.value2b(shortTmp);
        track.comboSamples.resize(shortTmp);
        for (int i=0;i<shortTmp;i++)
            s.object(track.comboSamples[i]);
    }


}

template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_EventFlex& flex) {
    uint8_t byteTmp;
    s.value1b(byteTmp);
    flex.tracks.resize(byteTmp);
    for (int i=0;i<byteTmp;i++)
        s.object(flex.tracks[i]);
}



template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_RelTags& tags) {

    uint16_t shortTmp;
    s.value2b(shortTmp);
    tags.name= Helper::VsifStringPool->getStringByID(shortTmp);
    uint8_t byteTmp;
    s.value1b(byteTmp);
    tags.duration= byteTmp *BVCD::One255th;
}


template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_FlexTimingTags& tags) {
    uint16_t shortTmp;
    s.value2b(shortTmp);
    tags.name= Helper::VsifStringPool->getStringByID(shortTmp);
    uint8_t byteTmp;
    s.value1b(byteTmp);
    tags.duration= byteTmp *BVCD::One255th;
}


template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_AbsTags& tags) {

    /*
    uint8_t byteTmp;
    s.value1b(byteTmp);
    tags.type=(VCD_AbsTagType)byteTmp; //This is NOT serialized
    */
    uint16_t shortTmp;
    s.value2b(shortTmp);
    tags.name= Helper::VsifStringPool->getStringByID(shortTmp);
    s.value2b(shortTmp); //here's the source of the confusion
    tags.duration=shortTmp*BVCD::One4096th;
}


template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_Event& e) {
    /*Header*/
uint8_t flagTmp;

uint16_t stringIndexTmp;
s.value1b(flagTmp);
e.eventType=(BVCD::Event_Type)flagTmp;
s.value2b(stringIndexTmp);
e.name= Helper::VsifStringPool->getStringByID(stringIndexTmp);
//Time
s.value4b(e.eventStart);
s.value4b(e.eventEnd);
//Params
s.value2b(stringIndexTmp);
e.param1= Helper::VsifStringPool->getStringByID(stringIndexTmp);
s.value2b(stringIndexTmp);
e.param2= Helper::VsifStringPool->getStringByID(stringIndexTmp);
s.value2b(stringIndexTmp);
e.param3= Helper::VsifStringPool->getStringByID(stringIndexTmp);
//ramp
s.object(e.ramp);
//flags
s.value1b(flagTmp);
e.flags = (BVCD::VCD_Flags)flagTmp;
//distance to target
float tmpFloat;
s.value4b(tmpFloat);
if(tmpFloat>0){
    e.distanceToTarget = tmpFloat;
}
//s.value4b(e.distanceToTarget);
//relative tags
uint8_t tagCount;
s.value1b(tagCount);
e.relativeTags.resize(tagCount);
for (int i=0;i<tagCount;i++)
    s.object(e.relativeTags[i]);
//flex timing tags
s.value1b(tagCount);
e.flexTimingTags.resize(tagCount);
for (int i=0;i<tagCount;i++)
    s.object(e.flexTimingTags[i]);
//absolute tags
{
    std::vector<BVCD::VCD_AbsTags> absTagsPlay;
    std::vector<BVCD::VCD_AbsTags> absTagsShift;
    s.value1b(tagCount);
    absTagsPlay.resize(tagCount);
    for (int i = 0; i < tagCount; i++)
        s.object(absTagsPlay[i]);
    //uint8_t tag = tagCount;
    s.value1b(tagCount);
    //std::vector<VCD_AbsTags> absTagsTmp(e.absoluteTags);
    //e.absoluteTags.resize(tagCount + tag);
    //e.absoluteTags.assign(absTagsTmp.begin(), absTagsTmp.end());

    absTagsShift.resize(tagCount);
    for (int i = 0; i < tagCount; i++)
        s.object(absTagsShift[i]);

    std::for_each(absTagsPlay.begin(), absTagsPlay.end(), [](BVCD::VCD_AbsTags & absTag) {
        absTag.type = BVCD::VCD_AbsTagType::playback;
    });
    std::for_each(absTagsShift.begin(), absTagsShift.end(), [](BVCD::VCD_AbsTags & absTag) {
            absTag.type = BVCD::VCD_AbsTagType::shifted;
    });

    e.absoluteTags.reserve(absTagsPlay.size() + absTagsShift.size()); //we already constructed objects
    e.absoluteTags.insert(e.absoluteTags.end(), absTagsPlay.begin(), absTagsPlay.end());
    e.absoluteTags.insert(e.absoluteTags.end(), absTagsShift.begin(), absTagsShift.end());
}

//sequence duration
if(BVCD::Event_Type::Event_Gesture==e.eventType) {
    s.value4b(e.sequenceDuration);
}
//relative tag
s.value1b(flagTmp);
e.usingRelativetag = flagTmp?true:false;

if (e.usingRelativetag)
    s.object(e.relativeTag);
//flex
s.object(e.flex);
//loop
if (BVCD::Event_Type::Event_Loop==e.eventType)
    s.value1b(e.loopCount);
//closed captions
if(BVCD::Event_Type::Event_Speak==e.eventType)
    s.object(e.closeCaptions);


}


template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_Channel& c){

    uint16_t stringIndex;
    s.value2b(stringIndex);
    c.name = Helper::VsifStringPool->getStringByID(stringIndex);
    uint8_t eventCount;
    s.value1b(eventCount);
    c.events.resize(eventCount);
    for (int i=0;i<eventCount;i++)
        s.object(c.events[i]);
    uint8_t isActive;

    s.value1b(isActive);
    c.isActive = (isActive)?true:false;
};


template <typename S>
void BVCD::serialize(S& s, BVCD::VCD_Actor& a) {
    uint16_t stringIndex;
    s.value2b(stringIndex);
    a.name = Helper::VsifStringPool->getStringByID(stringIndex);
    uint8_t channelCount;
    s.value1b(channelCount);
    a.channels.resize(channelCount);
    for (int i=0;i<channelCount;i++)
        s.object(a.channels[i]);
    uint8_t isActive;

    s.value1b(isActive);
    a.isActive = (isActive)?true:false;

}



template <typename S>
void BVCD::serialize(S& s, BVCD::VCD& vcd) {
    //std::string test = Helper::VsifStringPool->getStringByID(5);
    s.value4b(vcd.magic);
    s.value1b(vcd.version);
    s.value4b(vcd.CRC);
    uint8_t eventsCount;
    s.value1b(eventsCount);
    vcd.events.resize(eventsCount);
    for (int i=0;i<eventsCount;i++)
        s.object(vcd.events[i]);
    uint8_t actorsCount;
    s.value1b(actorsCount);

    vcd.actors.resize(actorsCount);
    for (int i=0;i<actorsCount;i++)
        s.object(vcd.actors[i]);
    s.object(vcd.ramp);


    
    uint8_t tmpIgnorePhonemes;
    s.value1b(tmpIgnorePhonemes);
    vcd.ignorePhonemes = (tmpIgnorePhonemes)?true:false;
}


template <typename S>
void BVCD::serialize(S& s, BVCD::CompressedVCD& cVCD) {
    s.value4b(cVCD.magic);
    s.value4b(cVCD.realSize);
    s.value4b(cVCD.compressedSize);
    s.container(cVCD.properties);

    cVCD.compressedBuffer.resize(cVCD.compressedSize);
    char* tmp = new char[cVCD.compressedSize+1];
    memset(tmp,0,cVCD.compressedSize+1);
    //HACK: There MUST be faster way.
    s.adapter().template readBuffer<1,char>(tmp,(size_t)cVCD.compressedSize);
    //assert();
    cVCD.compressedBuffer = std::vector<char>(tmp, tmp + cVCD.compressedSize + 1);
    delete[] tmp;

};









