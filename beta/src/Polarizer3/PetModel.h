// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include <vector>
#include <string>

class PetModel
{
    bool accountCanLaunchFile(std::wstring password, std::wstring filePath);
public:
    PetModel(void);
    virtual ~PetModel(void);
    std::wstring petname;
    std::vector<std::wstring> executables;
    std::vector<std::wstring> fileExtensions;
    std::wstring accountName;
    std::vector<std::wstring> serversToAuthenticate;
    std::vector<std::wstring> editableFolders;
    std::vector<std::wstring> readonlyFolders;
    std::wstring commandLineArgs;
    bool allowTempEdit;
    bool allowAppFolderEdit;
    bool readOnlyWithLinks;
    bool isDefaultBrowserPet;
    bool isDefaultMailPet;
	bool grantFullNetworkCredentials;
	bool allowOpAfterWindowClosed;
    bool commit();
    void copyFrom(PetModel* other);
    /**
     * loads the model from the registry
     */
    void load(std::wstring newPetname);
    void unsetFileAssociations();
    void undoRegisteredConfiguration();
    /**
     * clears the lists in the model so that
     * old elements from a loaded copy of the list
     * do not survive into a new list
     * when the new elements are just being pushed onto
     * the list; crucial to use this after a load of the 
     * model has been used to fill the polarizer dialog,
     * before adding new things in the polarize step
     * after the user has modified the dialog.
     */
    void clear();
};
