#if defined(ENABLE_TESTING) && defined(TESTING_VSIF)

#define BOOST_TEST_MODULE "testVSIF"

#define BOOST_TEST_MAIN
//vcpkg provides only static libraries
#if 0
#if !defined( WIN32 )
#define BOOST_TEST_DYN_LINK
#endif
#endif
#include <boost/test/unit_test.hpp>
#endif


#include "VSIF.hpp"
#include "BVCD.hpp"




#if defined(ENABLE_TESTING) && defined(TESTING_VSIF)
BOOST_AUTO_TEST_CASE(testVSIF) {


	VSIF::ValveScenesImageFile vsif("D:/SteamLibrary/steamapps/common/Team Fortress 2/tmp/scenes/scenes.image");
	
}
#endif


#if !defined(ENABLE_TESTING)
void VSIF::ValveScenesImageFile::fillWithVCDS()
{

    vcds.reserve(header.ScenesCount);
    for (unsigned int i=0;i<header.ScenesCount;i++){
        VSIF::VSIF_Entry entry = entries[i];
        //[first,last) - we need [first,last] so effectively [first,last+sizeof(char))
        //TODO - can std::next return end iterator?
        uint32_t startPos = entry.Offset-dataPos;

        std::vector<std::byte> bvcd(std::next(sceneBuffer.begin(),startPos),
                           std::next(sceneBuffer.begin(),startPos+entry.Size));


        BVCD::VCD vcdMem = BVCD::getSceneFromBuffer(bvcd);
        SPDLOG_INFO("Got VCD with CRC {0:#010X} from filebuffer size {1} B.", entry.CRC,entry.Size);
        //BVCD::VCD* vcdMemPtr = &vcdMem;
        vcds.push_back(vcdMem);

    }
}
#endif


static bool is_empty(std::ifstream& pFile)
{
    return pFile.peek() == std::ifstream::traits_type::eof();
}


bool VSIF::ValveScenesImageFile::Open(std::string filePath,ValveScenesImageFile& out,bool& error)
{
    error = false;
    using Buffer = std::vector<char>;
    using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
    using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
    std::ifstream fileStream = std::ifstream(filePath, std::ios::in | std::ios::binary);

    if (!fileStream || is_empty(fileStream))
    {
        return false;
    }
fileStream.seekg(0, std::ios_base::end);
    out.size = fileStream.tellg();

    fileStream.seekg(0, std::ios_base::beg);

    Buffer fileBuf(std::istreambuf_iterator<char>(fileStream), {});
    /*
    fileBuf.reserve(size);
    fileStream.read(fileBuf.data(), size);
    assert(fileBuf.size() != 0);
    */
    SPDLOG_INFO("Loading file header of {0}", filePath);

    bitsery::quickDeserialization<InputAdapter, VSIF_Header>(InputAdapter{ fileBuf.begin(),fileBuf.end() }, out.header);

    if(out.header.Version!=2){
        error=true;
        SPDLOG_ERROR("This utility supports version 2 of VSIF file, got version {0}",out.header.Version);
        return false;
      }


    fileStream.seekg(0, std::ios_base::beg);
    bitsery::quickDeserialization<InputAdapter, ValveScenesImageFile>(InputAdapter{ fileBuf.begin(),fileBuf.end() }, out);
    SPDLOG_INFO("File header loaded. Found {0} scenes.", out.entries.size());
    return true;
}
