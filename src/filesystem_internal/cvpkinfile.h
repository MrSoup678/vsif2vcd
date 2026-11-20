#ifndef CVPKINFILE_H
#define CVPKINFILE_H

#include <filesystem/ifile.h>
#include <vpkpp/vpkpp.h>

class CVPKInFile : public IFile
{
public:
    CVPKInFile(std::string basePath,std::string relPath,std::weak_ptr<vpkpp::PackFile> archiveHandle);
     bool extract(std::string whereTo, std::string &errorStr) override;
private:
     std::shared_ptr<vpkpp::PackFile> archiveHandle;
};

#endif // CVPKINFILE_H
