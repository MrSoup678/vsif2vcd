#ifdef ENABLE_TESTING
#define BOOST_TEST_MODULE "testMap"

#define BOOST_TEST_MAIN
#if 0
#if !defined( WIN32 )
#define BOOST_TEST_DYN_LINK
#endif
#endif
#include <boost/test/unit_test.hpp>
#endif




#include "program.h"
#include "response_system.h"
//#include <valve-bsp-parser/bsp_parser.hpp>
#include <bsppp/bsppp.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <set>
#include <vector>
#include <iterator>


int BSPParser::ExtractNames(std::string GameDirectory)
{
    //std::ifstream maplistTxt(GameDirectory+"/maplist.txt");
    //Iterating for every file in the map dir. This is deliberate, as there may be maps not listed in maplist.txt
    std::vector<std::string> maplist;
    std::filesystem::path mapPath(GameDirectory+"/maps/");

    for (auto path:std::filesystem::directory_iterator{mapPath}) {
        std::string mapPath = path.path().filename().generic_string();
        if (mapPath.substr(mapPath.size()-4)==".bsp") {
            maplist.push_back(mapPath);
        }
    }
    /*
    std::string line;
    while (safeGetline(maplistTxt,line)) {
        maplist.emplace_back(line);
    }
    */

    

    //using namespace rn;
    //bsp_parser map;
    //TODO: check if we dont miss anything...

    for (auto iter = maplist.begin();iter!=maplist.end();++iter) {
        //load map.
        bsppp::BSP bspIter(GameDirectory+"/maps/"+*iter);
        if(!bspIter)
            continue; //no map
        //if (map.load_map(GameDirectory+"/maps/",*iter)) {
            //Map is fully loaded. Check for logic_choreographed_scene

            std::vector<Map_Scene> scenes;

            auto entitiesKV = bspIter.getLumpData<bsppp::BSPLump::ENTITIES>();

            for (auto entityIter = entitiesKV.begin();entityIter!=entitiesKV.end();++entityIter) {
                auto entity = *entityIter;
                if (entity["classname"].getValue<std::string>() =="logic_choreographed_scene"){
                    const std::string sceneName = entity["SceneFile"].getValue<std::string>();
                    Map_Scene scene = Map_Scene(sceneName.c_str());
                    scenes.emplace_back(scene);
                }
                if (entity["classname"].getValue<std::string>() == "env_speaker") {//HL2: env_speaker uses its own script. Parse them separately if possible.
                    //TODO: append value of rulescript to scripts to parse.
                        RRParser::entryPointsToParse.push_back(entity["rulescript"].getValue<std::string>());


                }

            }
            //Scenes.insert(Scenes.end(),scenes.begin(),scenes.end());
            Scenes.emplace(*iter,scenes);
            SPDLOG_INFO("Found {0} scene filenames in {1}!", scenes.size(), *iter);
            //map.unload_map();
            
    //};



    }
    

    std::sort(RRParser::entryPointsToParse.begin(),RRParser::entryPointsToParse.end());
    RRParser::entryPointsToParse.erase(std::unique(RRParser::entryPointsToParse.begin(),RRParser::entryPointsToParse.end()),RRParser::entryPointsToParse.end());
//move globalscript to front
    auto globalScript = std::find_if(RRParser::entryPointsToParse.begin(),RRParser::entryPointsToParse.end(),
                                     [](std::string& script) -> bool {
                        return script == "scripts/talker/response_rules.txt";
    });
    std::rotate(RRParser::entryPointsToParse.begin(),globalScript,globalScript+1);
    SPDLOG_INFO("Added additional {0} entrypoints for Response Rules.", RRParser::entryPointsToParse.size() - 1);
    RRParser::entryPointsToParse.shrink_to_fit();
	return 0;
}










#ifdef ENABLE_TESTING

std::map<std::string,std::vector<BSPParser::Map_Scene>> BSPParser::Scenes;
BOOST_AUTO_TEST_CASE(testVSIF) {

	//VSIF::ValveScenesImageFile vsif("E:/hl2_tmp/scenes/scenes.image");
    BSPParser::ExtractNames("/home/slawomir/Dane/hl2_tmp");
    RRParser::initRules("/home/slawomir/Dane/hl2_tmp");

}
#endif
