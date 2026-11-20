

#pragma once
//GameInfo parsing.
//TODO Get files to copy to tmpdir for cleaning


//What do we actually need?

//There are two cases sdk-2013 base or L4D base.

//Do we even implement older code? (sdk-2007)
//Determine which is which
#include <vdf_parser.hpp>
#include "enum_bitmask.hpp"
#include "gameinfoKV.hpp"
#include "pch.hpp"
#include <unordered_map>


#include <filesystem/imountpath.h>

//this is first-in set for file seek
//TODO: |gameinfo_path| and |all_source_engine_paths|

/*
	What we need:
	- maps/(x).bsp
	- scenes/scenes.image
	- scripts/talker/(all)
*/

#define BASEGAME_DIR_TMPL "|all_source_engine_paths|"
#define MODDIR_TMPL "|gameinfo_path|"
#define EXTERNAL_APP_MOUNT_TMPL "|app_" // then appid then '|'
namespace FileSystem {
	enum class PathID : unsigned short
	{
		GAME = 1 << 0,
		GAME_WRITE = 1 << 1,
		GAMEBIN = 1 << 2,
		MOD = 1 << 3,
		MOD_WRITE = 1 << 4,
		DEFAULT_WRITE_PATH = 1 << 5,
		PLATFORM = 1 << 6, //Skip this
		DOWNLOAD = 1 << 7, //download dir for multiplayer
		CUSTOM = 1 << 8,	// custom dir for client mods

	};

	class CGameInfo {
		//this class represents gameinfo.txt keyvalues file.
	private:
		std::vector<std::pair<PathID, std::string>> searchPaths;
		std::string baseDir;

		std::string modDir;
		gameInfoKV memGI; //this var holds memory representation of gameinfo.txt
		bool isSDK2013Game; //This controls the path loading of gameinfo. If true this behaves explicitly. Otherwise implicit behavior is assumed.
		bool hasSingleplayer; // game is either singleplayer_only or has no type (implying SP+MP). games that do not have singleplayer will not have scenes inside map files.
		//FIXME: Handle older SDKs if anyone wants that.
        std::vector<std::pair<std::string,std::shared_ptr<IMountPath>>> filesAndTargets;
		


		
		void initGamepaths();
		void resolveLoadDirs();
		void resolveLoadDir(std::pair<FileSystem::PathID,std::string>& entry);

		void resolveBaseDir();
		bool replace(std::string& str, const std::string& from, const std::string& to);

	public:
		int appID=0;
		CGameInfo(): isSDK2013Game(false),hasSingleplayer(false)
		{};
        CGameInfo(std::string modDir); /* */

        bool prepareTmpDirectory(std::string&);
        void initializeFileSystem();



	private:
		void loadPAKs(PathID pathId, std::string atPath);
	void getSteamAppID();
		PathID resolvePathIDs(std::string input);
		std::string getPathFromAppID(int appID, std::vector<std::string> steamLibDirs);
	};
}

ENABLE_BITMASK_OPERATORS(FileSystem::PathID);
