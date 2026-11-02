#ifdef ENABLE_TESTING

#define BOOST_TEST_MODULE "testBVCD"

#define BOOST_TEST_MAIN
//vcpkg provides only static libraries
#if 0
#if !defined( WIN32 )
#define BOOST_TEST_DYN_LINK
#endif
#endif
#include <boost/test/unit_test.hpp>
#endif


#include "BVCD.hpp"
#include "VSIF.hpp"


#include <iomanip>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <fmt/core.h>
//#include <indent_facet.hpp>







struct indenter {
  int indent;
  std::string tabulation="";
public:

 
  template <typename... Args>
  std::string format(fmt::string_view format_str, const Args&... args) {
      tabulation=indent?std::string(indent,'\t'):"";
      std::string tmp=fmt::vformat(format_str, fmt::make_format_args(args...));
      ReplaceAll(tmp,"\n","\n"+tabulation);
      return tmp;
      //fmt::print(tmp);
  }

  template <typename... Args>
  void print(fmt::string_view format_str, const Args&... args) {
      fmt::print(this->format(format_str,args...));
  }

};


namespace BVCD {
    indenter stringParser;
}


using BVCD::stringParser;

BVCD::VCD BVCD::getSceneFromBuffer(std::vector<char> buffer) {
        char magic[4] = { buffer[0],buffer[1],buffer[2],buffer[3] };
        uint32_t readMagic = Helper::FourCC(magic);

        using Buffer = std::vector<char>;
        //using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
        using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

        memcpy(magic,"LZMA",4* sizeof(char));
        if (readMagic == Helper::FourCC(magic)) {
            BVCD::CompressedVCD vcdToDecompress;
            bitsery::quickDeserialization<InputAdapter, BVCD::CompressedVCD>(InputAdapter{ buffer.begin(),buffer.end() }, vcdToDecompress);
            //since we use streams we need to copy to one
            /*
            boost::iostreams::filtering_istreambuf in;
            in.push(boost::iostreams::lzma_decompressor());
            in.push(boost::make_iterator_range(vcdToDecompress.compressedBuffer.begin(),vcdToDecompress.compressedBuffer.end()));
            */
            std::vector<char> decompressedBuffer;
            unsigned char* decompressed = new unsigned char[vcdToDecompress.realSize+1];
            unsigned char* compressed = new unsigned char[vcdToDecompress.compressedSize+1];
            memset(decompressed,0,vcdToDecompress.realSize+1);
            memset(compressed,0,vcdToDecompress.compressedSize+1);
            std::copy(vcdToDecompress.compressedBuffer.begin(), vcdToDecompress.compressedBuffer.end(), compressed);

            size_t destSize=vcdToDecompress.realSize,srcSize=vcdToDecompress.compressedSize;

            LzmaUncompress(decompressed,
                           &destSize,
                           compressed,
                           &srcSize,
                           vcdToDecompress.properties,5);
            decompressedBuffer.assign(decompressed,decompressed+vcdToDecompress.realSize);



            memset(decompressed,0,vcdToDecompress.realSize);
            memset(compressed,0,vcdToDecompress.compressedSize);
            delete[] compressed;
            delete[] decompressed;


    //        boost::iostreams::stream_buffer<boost::iostreams::back_insert_device<std::vector<uint8_t>>> outStream(decompressedBuffer);

            return getSceneFromBuffer(decompressedBuffer);

            }

        memcpy(magic,"bvcd",sizeof(char)*4);
        assert(BVCD::FourCC(magic) == readMagic);
        BVCD::VCD vcd;
        bitsery::quickDeserialization<InputAdapter, BVCD::VCD>(InputAdapter{ buffer.begin(),buffer.end() }, vcd);


        return vcd;
    };




#ifdef ENABLE_TESTING


VSIF::ValveScenesImageFile* Helper::vsif;
BOOST_AUTO_TEST_CASE(testBVCD) {
    VSIF::ValveScenesImageFile vsif;
    bool err = false;
    vsif.Open("D:/SteamLibrary/steamapps/common/Team Fortress 2/tmp/scenes/scenes.image",vsif,err);
    Helper::vsif =&vsif;

    for (unsigned int i=0;i<Helper::vsif->header.ScenesCount;i++){
    VSIF::VSIF_Entry entry = Helper::vsif->entries[i];
    //[first,last) - we need [first,last] so effectively [first,last+sizeof(char))
    //TODO - can std::next return end iterator?
    uint32_t startPos = entry.Offset-Helper::vsif->dataPos;

    std::vector<char> bvcd(std::next(Helper::vsif->sceneBuffer.begin(),startPos),
                           std::next(Helper::vsif->sceneBuffer.begin(),startPos+entry.Size));


    BVCD::VCD vcdMem = BVCD::getSceneFromBuffer(bvcd);

    std::string vcdText = vcdMem.dumpText();
    std::ofstream vcdFileOut;
    std::filesystem::create_directories("temp/");
    std::string fileToOpen = "temp/"+std::to_string(vcdMem.CRC)+".vcd";
    vcdFileOut.open(fileToOpen);
vcdFileOut << vcdText;
vcdFileOut.flush();
vcdFileOut.close();


    std::cout << "Got an object from "<<entry.Size<<" B buffer."<<std::endl; //no reliable way mo measure size of object
    }
};
#endif


//#define ADD_LINE_WITH_INDENTS(x) \
//    stream+=std::string(indentation,'\t')+x;

void BVCD::VCD_Sample::dumpText(std::string& stream)
{
    stream += stringParser.format("{0} {1}\n",this->time,this->value);
       // stream << this->time << " " << this->value << "\n";
}

void BVCD::VCD_Ramp::dumpText(std::string& stream,bool inEvent)
{
    if(samples.size()==0) return;
     if (inEvent)
         stream += stringParser.format("event_ramp\n");
     else
         stream += stringParser.format("scene_ramp\n");

    stringParser.indent++;
   stream += stringParser.format("{{\n");

    for (auto sample=samples.begin();sample!=samples.end();sample++)
        sample->dumpText(stream);


    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream += stringParser.format("}}\n");
}

void BVCD::VCD_AbsTags::dumpText(std::string& stream)
{
    //focus just on dumping; We already prepared the {}

    stream += stringParser.format("\"{0}\" {1}\n",this->name,this->duration);

    return;
}

void BVCD::VCD_CC::dumpText(std::string& stream)
{
    stream+= stringParser.format("cctype ");

    switch (this->type){
    case VCD_CC_Type::master:
        stream+=stringParser.format("\"cc_master\" ");
        break;
    case VCD_CC_Type::slave:

        stream+=stringParser.format("\"cc_slave\" ");
        break;
    case VCD_CC_Type::disabled:

        stream+=stringParser.format("\"cc_disabled\" ");
        break;
    }
    stream += stringParser.format("\ncctoken \"{0}\"\n",this->cc_token);
    if ((bool)(flags & VCD_CC_Flags::usingCombinedFile))
        stream+=stringParser.format("cc_usingcombinedfile\n");
    if ((bool)(flags & VCD_CC_Flags::combinedUsingGenderToken))
        stream+=stringParser.format("cc_combinedusesgender\n");
    if ((bool)(flags & VCD_CC_Flags::suppressingCaptionAttenuation))
        stream+=stringParser.format("cc_noattentuate\n");
    return;
}

void BVCD::Flex_Samples::dumpText(std::string& stream)
{
    stream += stringParser.format("{0} {1} curve_{2}_to_curve_{3}\n", time,value,fromCurve,toCurve);
    return;
}

void BVCD::Flex_Tracks::dumpText(std::string& stream)
{
    bool bIsCombo = (bool)(this->flags & TrackFlags::isCombo);
    stream += stringParser.format("\"{0}\"",this->name);
    if (!(bool)(this->flags & TrackFlags::isEnabled))
        stream += " disabled";
    if (bIsCombo)
        stream += " combo";
    if (minRange!=0.0 && maxRange!=1.0)
        stream += stringParser.format(" range {0} {1}\n",minRange,maxRange);
    stringParser.indent++;
    stringParser.format("{{\n");

    //assert(this->samples.size()!=0);

       for (auto sample=samples.begin();sample!=samples.end();sample++)
           sample->dumpText(stream);



    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream += stringParser.format("}}\n");


    if (bIsCombo){

    stringParser.indent++;
    stringParser.format("{{\n");

    //assert(this->comboSamples.size()!=0);

       for (auto sample=comboSamples.begin();sample!=comboSamples.end();sample++)
           sample->dumpText(stream);



    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream += stringParser.format("}}\n");
    }

    return;
}

void BVCD::VCD_EventFlex::dumpText(std::string& stream)
{

    if(tracks.size()==0) return;
    stream += stringParser.format("flexanimations samples_use_time\n");
    stringParser.indent++;
     stream += stringParser.format( "{{\n");
    for (auto track=tracks.begin();track!=tracks.end();track++)
        track->dumpText(stream);

    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream += stringParser.format("}}\n");
    return;
}

void BVCD::VCD_RelTags::dumpText(std::string& stream)
{
    stream += stringParser.format("\"{0}\" {1}\n",this->name,this->duration);
    return;
}

void BVCD::VCD_RelTag::dumpText(std::string& stream)
{
    stream += stringParser.format("relativetag \"{0}\" \"{1}\"",this->name,this->wavName);
    return;
}

void BVCD::VCD_FlexTimingTags::dumpText(std::string& stream)
{
    stream += stringParser.format("\"{0}\" {1}\n",this->name,this->duration);
    return;
}

void BVCD::VCD_Event::dumpText(std::string& stream)
{

    stream += stringParser.format("event {0} \"{1}\"\n",eventTypeToString(eventType),name);
   // stream <<"event " << eventTypeToString(eventType) << " \"" <<name<<"\"\n";

    stringParser.indent++;
    stream += stringParser.format("{{\n");
    stream += stringParser.format("time {0} {1}\n",this->eventStart,this->eventEnd);
    //stream << "time "<<this->eventStart<< " "<<this->eventEnd<<"\n";
    stream += stringParser.format("param \"{0}\"\n",this->param1);
    //stream << "param \"" << this->param1 << "\"\n";
    if (param2.size()>0)

        stream += stringParser.format("param2 \"{0}\"\n",this->param2);
        //stream << "param2 \"" << this->param2 << "\"\n";
    if (param3.size()>0)

        stream += stringParser.format("param3 \"{0}\"\n",this->param3);
        //stream << "param3 \"" << this->param3 << "\"\n";

    ramp.dumpText(stream,true);
    if ((bool)(this->flags&VCD_Flags::resumeCondition))
        stream += stringParser.format("resumecondition\n");
    if ((bool)(this->flags&VCD_Flags::lockBodyFacing))
        stream += stringParser.format("lockbodyfacing\n");
    if ((bool)(this->flags&VCD_Flags::fixedLength))
       stream += stringParser.format("fixedlength\n");
    if (!(bool)(this->flags&VCD_Flags::isActive))
        stream += stringParser.format("active \"0\"\n");
    if ((bool)(this->flags&VCD_Flags::forceShortMovement))
        stream += stringParser.format("forceshortmovement\n");
    if ((bool)(this->flags&VCD_Flags::playOverScript))
        stream += stringParser.format("playoverscript\n");
    if ((bool)(this->flags&VCD_Flags::syncToFollowingGesture))
        stream += stringParser.format("synctofollowinggesture\n");
    //int precision = stream.precision();
    if (distanceToTarget>0)
        stream += stringParser.format("distancetotarget {0:-.2f}",distanceToTarget);
        //stream <<"distancetotarget "<< std::setprecision (2) << std::fixed << distanceToTarget<<"\n";
    //stream.unsetf(std::ios_base::floatfield);
    //stream<< std::setprecision(precision);

    //Relative Tags
    if (relativeTags.size()!=0)
    {
        stream += stringParser.format("tags\n");

        stringParser.indent++;

         stream +=stringParser.format("{{\n");
        //stream << "tags\n"<<"{{\n";
        for (auto tag=relativeTags.begin();tag!=relativeTags.end();tag++)
            tag->dumpText(stream);
        //stream << "}}\n";
        stringParser.indent--;
         stream = stream.substr(0, stream.size()-1);
        stream +=stringParser.format("}}\n");
    }

    //Flex Timing Tags
    if (flexTimingTags.size()!=0)
    {
        stream += stringParser.format("flextimingtags\n");

        stringParser.indent++;

         stream +=stringParser.format("{{\n");

       // stream << "flextimingtags\n"<<"{{\n";
        for (auto tag=flexTimingTags.begin();tag!=flexTimingTags.end();tag++)
            tag->dumpText(stream);


        stringParser.indent--;
         stream = stream.substr(0, stream.size()-1);
        stream +=stringParser.format("}}\n");
    }

    //Absolute Tags
    std::vector<VCD_AbsTags> shiftedTime;
    std::vector<VCD_AbsTags> playbackTime;
    std::copy_if(absoluteTags.begin(),absoluteTags.end(),
                 std::back_inserter(shiftedTime),[](VCD_AbsTags absTag) {
                    return absTag.type ==VCD_AbsTagType::shifted;
    });
    std::copy_if(absoluteTags.begin(),absoluteTags.end(),
                 std::back_inserter(playbackTime),[](VCD_AbsTags absTag) {
                    return absTag.type ==VCD_AbsTagType::playback;
    });
    if (shiftedTime.size()!=0)
    {
        stream += stringParser.format("absolutetags shifted_time\n");

        stringParser.indent++;

         stream +=stringParser.format("{{\n");
        //stream << "absolutetags\n"<<"{{\n";
        for (auto tag=shiftedTime.begin();tag!=shiftedTime.end();tag++)
            tag->dumpText(stream);
       // stream << "}}\n";
        stringParser.indent--;
         stream = stream.substr(0, stream.size()-1);
        stream +=stringParser.format("}}\n");
    }
    if (playbackTime.size()!=0)
    {stream += stringParser.format("absolutetags playback_time\n");

        stringParser.indent++;

        stream +=stringParser.format("{{\n");
        //stream << "flextimingtags\n"<<"{{\n";
        for (auto tag=playbackTime.begin();tag!=playbackTime.end();tag++)
            tag->dumpText(stream);
        //stream << "}}\n";
        stringParser.indent--;
         stream = stream.substr(0, stream.size()-1);

        stream +=stringParser.format("}}\n");
    }


    //Sequence duration
    if (eventType==Event_Type::Event_Gesture)
        stream += stringParser.format("sequenceduration {0}\n",sequenceDuration);
       // stream <<"sequenceduration "<< sequenceDuration <<"\n";

    //relative tag
    if (usingRelativetag)
        relativeTag.dumpText(stream);
    //flexAnimation
    flex.dumpText(stream);
    if (eventType==Event_Type::Event_Loop)

        stream += stringParser.format("loopcount {0}\n",loopCount);
       // stream << "loopcount \""<<loopCount<<"\"\n";
    //CCs
    stream +=stringParser.format("\n");
    if (eventType==Event_Type::Event_Speak)
    {
        closeCaptions.dumpText(stream);
    }

    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream +=stringParser.format("}}\n");
    //stream << "}}\n";

}

void BVCD::VCD_Channel::dumpText(std::string& stream)
{
    stream+=stringParser.format("channel \"{0}\"\n",this->name);
    //stream << "channel \""<<this->name<<"\"\n";
    stringParser.indent++;
    stream += stringParser.format("{{\n");
    for (auto event = events.begin();event!=events.end();event++)
        event->dumpText(stream);
    if (!isActive)
        stream += stringParser.format("active \"0\"\n");

    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream += stringParser.format("}}\n");
}

void BVCD::VCD_Actor::dumpText(std::string& stream)
{
    stream+=stringParser.format("actor \"{0}\"\n",this->name);
    //stream << "channel \""<<this->name<<"\"\n";
    stringParser.indent++;
    stream += stringParser.format("{{\n");
    for (auto channel = channels.begin();channel !=channels.end();channel++) {
        channel->dumpText(stream);
    }
    if (!isActive)
        stream += stringParser.format("active \"0\"\n");

    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
    stream += stringParser.format("}}\n");

}

std::string BVCD::VCD::dumpText()
{
    //indentStringStream streamOut;

    std::string stream="";
    stringParser.indent =0;
    //int indentation=0;

    //streamOut << incrementIndent;
    std::ios_base::sync_with_stdio(false);
    stream +=  stringParser.format("//Choreo version 1\n");

    for(auto element=events.begin();element!=events.end();element++) {
        element->dumpText(stream);
    }
    for(auto element=actors.begin();element!=actors.end();element++) {
        element->dumpText(stream);
    }
    ramp.dumpText(stream,false);

    assert(stringParser.indent==0);
    stream +=  stringParser.format( "\"scalesettings\"\n");

    stringParser.indent++;
    //indentation++;
    stream +=  stringParser.format( "{{\n") ;//<<indent_manip::push;

    stream +=  stringParser.format( "\"CChoreoView\" \"100\"\n" );
    stream +=  stringParser.format( "\"SceneRampTool\" \"100\"\n" );
    stream +=  stringParser.format( "\"ExpressionTool\" \"100\"\n") ;
    stream +=  stringParser.format( "\"GestureTool\" \"100\"\n" );
    stream +=  stringParser.format( "\"RampTool\" \"100\"\n") ;//<<indent_manip::pop;

    stringParser.indent--;
     stream = stream.substr(0, stream.size()-1);
//indentation--;
   stream +=  stringParser.format("}}\n" );
    stream +=  stringParser.format("fps 60\n");
    stream +=  stringParser.format("snap off\n");




    return stream;
}

//#undef ADD_LINE_WITH_INDENTS


