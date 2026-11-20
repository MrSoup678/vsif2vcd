#pragma once
#include "pch.hpp"
#include "BVCD.hpp"
#include "helper.hpp"
#include "VSIFStringPool.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>


namespace BVCD{
 class VCD;
}

//ODR
namespace VSIF {
	typedef struct {
		unsigned int ID;
		unsigned int Version;
		unsigned int ScenesCount;
		unsigned int StringsCount;
		unsigned int EntryOffset;

		
    } VSIF_Header;

//Putting those in c++ nets us
    template <typename S>
    void serialize(S& s, VSIF_Header& vsif) {
        s.value4b(vsif.ID);
        s.value4b(vsif.Version);
        s.value4b(vsif.ScenesCount);
        s.value4b(vsif.StringsCount);
        s.value4b(vsif.EntryOffset);
    };
	
	typedef struct {
		unsigned int CRC;
		unsigned int Offset, Size;
		unsigned int SummaryOffset;


		
	} VSIF_Entry;
	

    template <typename S>
    void serialize(S& s, VSIF_Entry& vsif) {
        s.value4b(vsif.CRC);
        s.value4b(vsif.Offset);
        s.value4b(vsif.Size);
        s.value4b(vsif.SummaryOffset);
    };

	

	typedef struct {
		unsigned int msecs;
		std::vector<int> soundIDs;


		
		

	} VSIF_SceneSummary;


    template <typename S>
    void serialize(S& s, VSIF_SceneSummary& vsif) {
        s.value4b(vsif.msecs);
        uint32_t tmp;
        s.value4b(tmp); //size of the below vector

        vsif.soundIDs.resize(tmp);
        for (int i = 0; i < tmp; i++) {
            s.object(vsif.soundIDs[i]);
        }
    };






	



	//representaion of scenes.image file
	struct ValveScenesImageFile {
		uint32_t size;
		VSIF_Header header;
		CStringPool stringPool;
		std::vector<VSIF_Entry> entries;
		std::vector<VSIF_SceneSummary> summaries;
		uint32_t dataPos;
		//Cannot make a vector of structs since that data might be compressed
                std::vector<std::byte> sceneBuffer;

        std::vector<BVCD::VCD> vcds;
        void fillWithVCDS();
    public:
	
	static bool Open(std::string filePath,ValveScenesImageFile& out,bool& error);


	public:
	ValveScenesImageFile(){

	}



	};


    //template <typename S>
    template <typename S>
    void serialize(S& s, ValveScenesImageFile& vsif) {

        s.object(vsif.header);
        char tmpString[4];
        //tmpString ="VSIF"
        memccpy(tmpString,"VSIF",sizeof(char),4);
        //assert(vsif.header.ID==Helper::FourCC(tmpString) && vsif.header.Version ==2);
        s.ext
        (
            vsif.stringPool,
            bitsery::ext::SaveAndLoad
            (
                [](auto& ser, const CStringPool& obj)
                {
                }, // save function
                [vsif](auto& des, CStringPool& obj)
                { //friend CStringPool;


                    //static_assert(std::is_same<decltype(des), bitsery::InputBufferAdapter<std::vector<char>>,"buffers only.");
                    std::vector<int> stringOffsets;
                    stringOffsets.resize(vsif.header.StringsCount);
                    for (int i = 0; i < vsif.header.StringsCount; i++) {
                        des.value4b(stringOffsets[i]);
                    }
                    for (std::vector<int>::iterator iter = stringOffsets.begin();
                        iter != stringOffsets.end();
                        ++iter)
                    {
                        int stringSize = 0;
                        if (std::next(iter)==stringOffsets.end())
                        { //check if this is the last element

                            stringSize = vsif.header.EntryOffset - *(iter); //do NOT use incrementation here.
                        }
                        else
                        {
                            stringSize = *(iter + 1) - *(iter); //do NOT use incrementation here.
                        };
                        char* stringRaw = new char[stringSize];
                        des.adapter().template readBuffer<1, char>(stringRaw, stringSize);
                        //des.ext(stringRaw, PointerObserver{});
                        obj.findOrAddString(stringRaw);
                        delete[] stringRaw;
                    };

                    //TODO: This might end up misaligned. I cannot use currentReadPos for some reason
                    //Reason: des is Deserializer, and I need access to InputAdapter
                    //This should work. Might be not needed tho.
                    while (des.adapter().currentReadPos() % 4 != 0) {
                        des.adapter().currentReadPos(des.adapter().currentReadPos() + 1);
                    }
                }
            )
        );
        //HACK: anonymous struct access erroneously selects container(type,func) overload
        size_t scenesCount = vsif.header.ScenesCount;
        vsif.entries.resize(scenesCount);
        vsif.summaries.resize(scenesCount);
        for (int i = 0; i < scenesCount; i++) {
            s.object(vsif.entries[i]);
        }

        for (int i = 0; i < scenesCount; i++) {
            s.object(vsif.summaries[i]);
        }
        //s.container(vsif.entries, scenesCount);
        //s.container(vsif.summaries, scenesCount);


        /*
        dataPos = fileStream.tellg();
        //copy raw BVCD buffer to memory
        fileStream.seekg(0, fileStream.end);
        unsigned int fileEnd = fileStream.tellg();
        fileStream.seekg(dataPos, fileStream.beg);
        fileStream.read(sceneBuffer, fileEnd - dataPos);
        */

        vsif.dataPos = s.adapter().currentReadPos();
        uint32_t dataSize = vsif.size - vsif.dataPos;
        //s.adapter().currentReadPos(dataPos);
        vsif.sceneBuffer.resize(dataSize);
        char* tmp = new char[dataSize+1];
        memset(tmp,0,dataSize+1);
        //HACK: There MUST be faster way.
        s.adapter().template readBuffer<1,char>(tmp,(size_t)dataSize);
        //assert();
        vsif.sceneBuffer = std::vector<std::byte>((std::byte*)tmp, (std::byte*)tmp + dataSize+1);
        delete[] tmp;
        /*
        for (int i = 0; i < (vsif.size - vsif.dataPos); i++) {
            s.value1b(vsif.sceneBuffer[i]);
        }
        */
    };

};





