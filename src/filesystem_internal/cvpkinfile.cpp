#include "cvpkinfile.h"
//#include <HLTypes.h>
//#include <HLLib.h>

#include <filesystem>
CVPKInFile::CVPKInFile(std::string basePath,std::string relPath,std::weak_ptr<vpkpp::PackFile> archiveHandle)
{
    this->basePath = basePath;
    this->relPath = relPath;
    this->archiveHandle = archiveHandle.lock();


}

bool  CVPKInFile::extract(std::string whereTo, std::string &errorStr)
{
    std::string extractPath = whereTo;
    bool success = false;
/*
    if (this->fileHandle->GetType()==HLDirectoryItemType::HL_ITEM_FILE) {
        std::string truePath = this->relPath;
        std::filesystem::path path(truePath);
        path = path.remove_filename();

        truePath = path.generic_string();
        extractPath= whereTo+truePath;
        std::filesystem::create_directories(extractPath);
    }
    bool success = this->fileHandle->Extract(extractPath.c_str());
    if (!success) {
        errorStr= hlGetString(HLOption::HL_ERROR_LONG_FORMATED);
    }
    */
    this->archiveHandle->extractEntry(relPath,std::filesystem::path(whereTo).append(relPath));
    return success;
}

