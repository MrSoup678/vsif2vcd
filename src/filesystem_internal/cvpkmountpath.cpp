#include "cvpkmountpath.h"
#include <cstring>

#include "cvpkinfile.h"
#include <wildcard.h>

CVPKMountPath::CVPKMountPath(std::string path)
{
        //hlInitialize();
        //packageHandler = HLLib::CPackage::AutoOpen(path.c_str(), HLFileMode::HL_MODE_READ);
    packageHandler = std::move(vpkpp::VPK::open(path));
	filePath = path;

}


std::vector<std::shared_ptr<IFile>> CVPKMountPath::Find(std::string substr)
{

        //HLLib::CDirectoryFolder* pRoot = packageHandler->GetRoot();

        //pRoot->Sort(); //by default this searches alphabetically and recursively
    //lazy init
    if(this->fileList.empty())
    {
        this->ListFiles(fileList);
        std::for_each(fileList.begin(),fileList.end(),[](std::string& element){
            Helper::ReplaceAll(element,"\\","/");
        });
    }
	std::vector<std::string> matchingPaths;
	matchingPaths = filter(this->fileList, [substr](std::string path) {
		return wildcard(substr.c_str(), path.c_str());
		});
	std::vector<std::shared_ptr<IFile>> results;


        for (const auto& path : matchingPaths) {
                std::shared_ptr<IFile> fileHandle = std::make_shared<CVPKInFile>(CVPKInFile(this->filePath, path,this->packageHandler));
		results.push_back(std::move(fileHandle));
	}

	

	return results;
}
/*
static void DoList(std::vector<std::string>& files, std::weak_ptr<vpkpp::Entry> pDir)
{
	pDir->Sort();
	auto itemCount = pDir->GetCount();
	for (int i = 0; i < itemCount; i++)
	{
		auto item = pDir->GetItem(i);
		auto type = item->GetType();
		switch (type) {
			case HL_ITEM_FOLDER:
			{
				auto castedItem = dynamic_cast<HLLib::CDirectoryFolder*>(item);
				DoList(files, castedItem);
				break;
			}
			case HL_ITEM_FILE:
			{
				auto castedItem = dynamic_cast<HLLib::CDirectoryFile*>(item);
				char path[256];
				castedItem->GetPath(path, 256); //Does this include "root/"?
				std::string strPath{ path };
				strPath = strPath.substr(5);
				files.emplace_back(strPath);
				break;
			}
		}
	}


}
*/
void CVPKMountPath::ListFiles(std::vector<std::string>& files)
{
    packageHandler->runForAllEntries([files](const std::string& path, const vpkpp::Entry& entry) -> void
                                     {
                                        const_cast<std::vector<std::string>&>(files).emplace_back(path);
                                     },false);
}
