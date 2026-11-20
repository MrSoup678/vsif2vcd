#ifndef CVPKMOUNTPATH_H
#define CVPKMOUNTPATH_H

#include <filesystem/imountpath.h>

#include <vpkpp/vpkpp.h>

class CVPKMountPath : public IMountPath
{
public:
    CVPKMountPath(std::string path);

    std::vector<std::shared_ptr<IFile>> Find(std::string substr) override;



    virtual ~CVPKMountPath() =default;
private:
    std::shared_ptr<vpkpp::PackFile> packageHandler;


    // IMountPath interface
public:
    void ListFiles(std::vector<std::string> &files) override;
};

#endif // CVPKMOUNTPATH_H
