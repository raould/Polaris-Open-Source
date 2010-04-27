// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include ".\getKnownApps.h"
#include "..\polaris\Constants.h"
#include "..\polaris\Logger.h"
#include "PetModel.h"


std::vector<std::wstring> getKeyVal(FILE* in) {
    int const max = 2000;
    wchar_t wlinechars[max] ={};
    char line[max] = {};
    fgets(line, max, in);
    mbstowcs(wlinechars,line, max);
    std::wstring wline = wlinechars;
    std::vector<std::wstring> result;
    int eqsign = wline.find(L"=");
    int semicolon = wline.rfind(L";");
    if (eqsign == std::string::npos) {
        result.push_back(L"");
        result.push_back(L"");
    } else {
        result.push_back(wline.substr(0,eqsign));
        result.push_back(wline.substr(eqsign+1,semicolon-eqsign-1));
    }
    return result;
}

std::vector<PetModel*> getKnownApps() {
    std::vector<PetModel*> apps;
    FILE* appFile = _wfopen((Constants::polarisExecutablesFolderPath() 
        + L"\\knownapps.ini").c_str(), L"r");

    PetModel *nextModel = NULL;
    std::vector<std::wstring> nextPair = getKeyVal(appFile);
    while (nextPair[0].length() > 0) {
        std::wstring key = nextPair[0];
        std::wstring val = nextPair[1];
        if (key == L"nickname") {
            if (nextModel != NULL) {
                apps.push_back(nextModel);
            }
            nextModel = new PetModel();
            nextModel->petname = val;
        } else if (key == L"path") {
            nextModel->executables.push_back(val);
        } else if (key == L"readlinks") {
            nextModel->readOnlyWithLinks = val == L"true";
        } else if (key == L"suffix") {
            nextModel->fileExtensions.push_back(val);
        } else if (key == L"isdefaultbrowserpet") {
            nextModel->isDefaultBrowserPet = val == L"true";
		} else if (key == L"commandargs") {
			nextModel->commandLineArgs = val;
		}
        nextPair = getKeyVal(appFile);
        
    }
    apps.push_back(nextModel);
    return apps;
}