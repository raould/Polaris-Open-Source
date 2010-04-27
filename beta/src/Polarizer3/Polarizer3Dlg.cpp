// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// Polarizer3Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "Polarizer3Dlg.h"
#include "../polaris/RegKey.h"
#include ".\polarizer3dlg.h"
#include "../polaris/RegKey.h"
#include "../polaris/Constants.h"
#include "../polaris/MakeAccount.h"
#include <Lm.h>
#include <windows.h>
#include <userenv.h>
#include "../polaris/RecursiveRemoveDirectoryWithPrejudice.h"
#include "../polaris/Logger.h"
#include <Sddl.h>
#include "CompleteBox.h"
#include "DePolarizationCompleteBox.h"
#include "SetAuthentication.h"
#include "PolarizeExistingPetBox.h"
#include "../polaris/CopyDirectory.h"
#include "CannotPolarizeActivePet.h"
#include "../polaris/RPCClient.h"
#include "../polaris/Request.h"
#include "../polaris/Message.h"
#include "petRegistryTools.h"
#include "ConfigureEndowmentsBox.h"
#include "TextFunctions.h"
#include "getKnownApps.h"
#include "NoExecutable.h"
#include "ImportantError.h"
#include "BrowserSettingVisitor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Logger logger(L"Polarizer3Dlg");




/*
 * copy string values in a key subtree  
 * Skip anything rooted in the polaris subkey, and anything rooted in a key prefixed by "Windows"
 */
void copyKey(RegKey fromKey, RegKey toKey) {
    for (int i = 0 ; true; ++i) {
        std::wstring nextFromSubKeyName = fromKey.getSubKeyName(i);
        // break out when we hit end of subkeys
        if (nextFromSubKeyName == L"") {break;}
        // skip if we hit the root of a non-copy subtree, either the 
        // polaris key or a key with a name prefixed with Windows,
        if (nextFromSubKeyName == Constants::polarisBaseName() || 
            nextFromSubKeyName.find(L"Windows",0) == 0) {
                continue;
        }
        RegKey nextFromSubKey = fromKey.open(nextFromSubKeyName);
        if (nextFromSubKey.isGood()) {
            RegKey nextToSubKey = toKey.create(nextFromSubKeyName);
            copyKey(nextFromSubKey, nextToSubKey);
            /*if (nextToSubKey.isGood()) {
                copyKey(nextFromSubKey, nextToSubKey, replacementPath, replacedPath);
            } else {
                RegKey nextCreatedKey = toKey.create(nextFromSubKeyName);
                copyKey(nextFromSubKey, nextCreatedKey, replacementPath, replacedPath);
            }*/
        }
    }

    const DWORD binarySize = 32000;
    DWORD size = binarySize;
    BYTE bytes[binarySize];
    for (int j = 0; true; ++j) {
        std::wstring nextValueName = fromKey.getValueName(j);
        if (nextValueName == fromKey.NO_VALUE_FOUND()) {break;}
        DWORD type = fromKey.getType(nextValueName);
        size = binarySize;
        fromKey.getBytesValue(nextValueName,bytes,&size);
        toKey.setBytesValue(nextValueName, type, bytes, size);
    }
            
}


/**
 * This is a heuristic algorithm to get the stuff the user is likely to want from
 * the original account registry copied to the new account registry. So it copies from
 * the appropriate subtrees (including printer, control panel, software, etc). 
 *
 * It also skips any parts of subtrees rooted in a key whose name is 
 * prefixed with the word "Windows", this is one of the areas
 * you don't want to make this replacement (this is very very heuristic). It also
 * skips any key named "Polaris", since you don't want to be copying the polaris private
 * data to the pets. Last but not least, it ensures that the file extensions are not
 * hidden in the pet, because if they are hidden, then the extensions do not show up 
 * in the file dialog box generated by the app, and the dialog watcher cannot parse
 * and extract them for the full power dialog. But this is not all. It also sets
 * a registry entry for IE so that, if you wind up running IE here, it will not
 * foolishly launch Word embedded as an ActiveX control inside itself.
 * TODO: there is a more sophisticated and better solution to the problems with
 *    IE launching other apps as activeX controls inside itself, 
 *    implement it (intercepting mime type requests by IE).
 * In a similar vein, it also sets the registry entry for adobe acrobat 7 so that
 * it will not launch pdfs embedded in a browser web page, either.
 * But this is not all the "copy" does. If a pet has been designated as the default browser,
 * it configures the http and https defaults for the new pet so that the default browser
 * pet will be launched from inside this pet 
 */
void deriveRegistry(RegKey fromKey, RegKey toKey) {
        std::wstring copyRoots[] = {L"Printers", L"Control Panel", L"Software", 
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices",
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts",
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"};
    int rootCount = 6;
    for (int i = 0; i < rootCount; ++i) {
        RegKey fromSub = fromKey.open(copyRoots[i]);
        if (fromSub.isGood()) {
            RegKey toSub = toKey.create(copyRoots[i]);
            copyKey(fromSub, toSub); 
        }
    }
    ensureFileExtensionsShown(toKey);
	makeWordNotActiveXinBrowser(toKey);
	makeAcrobatNotEmbedInBrowser(toKey);
	makeEachAcrobatLaunchUseNewWindow(toKey);
	BrowserSettingVisitor visitor;
	visitor.visit(toKey);
}

// here is an abortive attempt to configure firefox in the pet so that downloads are not dropped on the 
// desktop by default, but rather, the user is always asked where to put downloads.
// Aborted because the folder/file where this config setting is stored, in prefs.js where you
// would put the line 
//    user_pref("browser.download.useDownloadDir", false);
// is not created until firefox launches, at which point it sets its own stupid default. This
// can't be fixed at polarize time because firefox hasn't launched and configured itself yet.
/*void configureFireFox(std::wstring petAccount) {
	std::wstring firefoxConfigPath = Constants::userAppDataPath(Constants::userProfilePath(petAccount)) + L"\\Mozilla\\Firefox";
	std::wstring profileIniPath = firefoxConfigPath + L"\\profile.ini";

}
*/



void deletePetsList (CListBox* petsList) {
	int numToKill = petsList->GetCount();
	for (int i = 0; i < numToKill; ++i) {
		petsList->DeleteString(0);
	}
}

void loadExistingPets(CListBox* petsList) {
	deletePetsList(petsList);
    RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());
    int i = 0;
    while (true) {
        std::wstring petname = petsKey.getSubKeyName(i);
        if (petname.length() == 0) {break;}
        if (petname != Constants::icebox()) {
            petsList->AddString(petname.c_str());
        }
        i++;
    }
}

void loadKnownApps(CListBox* knownAppsBox, std::vector<PetModel*> knownAppModels) {
    for (int i = 0; i < knownAppModels.size(); ++i) {
        knownAppsBox->AddString(knownAppModels[i]->petname.c_str());
    }
}

void deleteAccount(std::wstring accountName) {
    NetUserDel(NULL,accountName.c_str());
	if (Constants::horridDeleteError() == 
		RecursiveRemoveDirectoryWithPrejudice(Constants::userProfilePath(accountName))) {
		logger.log(L"horrid delete error deleting: " + accountName);
		ImportantError dlg;
		dlg.DoModal();
	}
    RegKey accountKey = RegKey::HKCU.open(Constants::registryAccounts());
    accountKey.deleteSubKey(accountName);
}

std::wstring getTextFromDialog(CEdit* nameField) {    
    CString pathAnswer;
    nameField->GetWindowText(pathAnswer);
    std::wstring path = (const wchar_t*) pathAnswer ;
    return path;
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CPolarizer3Dlg dialog



CPolarizer3Dlg::CPolarizer3Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPolarizer3Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


CPolarizer3Dlg::~CPolarizer3Dlg() {
    while (knownAppModels.size()>0) {
        PetModel* next = knownAppModels.back();
        knownAppModels.pop_back();
        next->clear();
        delete next;
    }
    knownAppModels.clear();
}

void CPolarizer3Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT2, m_path);
    DDX_Control(pDX, IDC_EDIT1, m_petname);
    DDX_Control(pDX, IDC_EDIT3, m_fileExtensionsText);
    DDX_Control(pDX, IDC_LIST1, ExistingPetsList);
    DDX_Control(pDX, IDC_LIST2, KnownAppsBox);
}

BEGIN_MESSAGE_MAP(CPolarizer3Dlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
    ON_EN_CHANGE(IDC_EDIT3, OnEnChangeEdit3)
    ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
    ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
    ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
    ON_BN_CLICKED(IDC_BUTTON5, OnBnClickedButton5)
    ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
    ON_LBN_SELCHANGE(IDC_LIST2, OnLbnSelchangeList2)
	ON_BN_CLICKED(IDC_BUTTON6, OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, OnBnClickedButton7)
END_MESSAGE_MAP()


// CPolarizer3Dlg message handlers

BOOL CPolarizer3Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Polarizer-specific initialization
    loadExistingPets(&ExistingPetsList);
    knownAppModels = getKnownApps();
    loadKnownApps(&KnownAppsBox, knownAppModels);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPolarizer3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPolarizer3Dlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPolarizer3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// This is the Browse for Executable button being clicked
// open a file dialog, get path name
void CPolarizer3Dlg::OnBnClickedButton2()
{
CFileDialog executableDialog(TRUE,NULL,NULL);
    //executableDialog.SetWindowText("Select Executable To Polarize");
    executableDialog.DoModal();
    CString filePath = executableDialog.GetPathName();
    m_path.SetWindowText(filePath);
}

bool petIsRunning(std::wstring petname) {
	RPCClient* client = makeClient();
    Request* request = client->initiate("ping", (petname.length()+1) * sizeof(wchar_t));
    request->send(petname.c_str());
    Message answer = request->finish(false, 5);
    size_t messageSize = answer.length();
    wchar_t* result;
    answer.cast(&result);
    // if the petwindow is not running, we presume that no pets are running
    // (or if they are, it is a test configuration), return false
    if (result == NULL) {return false;}
    return result[0] == L't';
}

// This is the Polarize button being clicked
// Configure the pet
void CPolarizer3Dlg::OnBnClickedButton1()
{
	std::wstring petname = getTextFromDialog(&m_petname);
	std::wstring exepath = getTextFromDialog(&m_path);

    // first verify this is a reasonable thing to try to polarize
	if (petname.find(L" ") != std::string::npos) {
		MessageBox(L"Spaces Disallowed in Petnames", L"Please remove spaces from petname", MB_OK);
		return;
	}
	if (petname.length() == 0) {
		MessageBox(L"No Petname Selected", L"Please Specify a Petname", MB_OK);
		return;
	}
    HANDLE hfile = CreateFile(exepath.c_str(), FILE_EXECUTE | FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (hfile == INVALID_HANDLE_VALUE) {
        int err = GetLastError();
        NoExecutable dlg;
        INT_PTR nResponse = dlg.DoModal();
		return;
    }
    CloseHandle(hfile);

	// petname and path are reasonable
    RegKey parentKey = RegKey::HKCU.open(Constants::registryPets());
    RegKey existingPetKey = parentKey.open(petname);
    if (!existingPetKey.isGood()) {
        polarizeFromScratch();
    } else {
        if (petIsRunning(petname)) {
            // tell user to shut down pet
            CannotPolarizeActivePet dlg;
	        INT_PTR nResponse = dlg.DoModal();
        } else {
            PolarizeExistingPetBox dlg;
	        INT_PTR nResponse = dlg.DoModal();
            if (nResponse == Constants::polarizeFromScratchRequested()) {
                polarizeFromScratch();
            } else if (nResponse== Constants::repolarizeRequested()) {
                repolarize();
            }
        }
    }
}

int CPolarizer3Dlg::loadModelFromDialog() {
    //first see if the specified executable path file exists, if not, abort
    std::wstring path = getTextFromDialog(&m_path);
    DWORD doesFileExist = GetFileAttributes(path.c_str());
    // TODO recover from no-file more sensibly, send back nonzero status after
    // popping dialog warning
    ASSERT (doesFileExist != INVALID_FILE_ATTRIBUTES);

    // the first slot in the list of executables is reserved for the front-page executable
    // (which has 2 special properties: the name of the shortcut
    // to it is just the petname, and it is the executable to be placed in the basic
    // window executable field in the future)
    if (model.executables.size() == 0) {
        model.executables.push_back(path);
    } else {
        model.executables[0] = path;
    } 
    model.fileExtensions = getFileExtensions();
    model.petname = getTextFromDialog(&m_petname);
    return 0;
}

void CPolarizer3Dlg::clearWindow() {
    m_path.SetWindowText(L"");
    m_fileExtensionsText.SetWindowText(L"");
    m_petname.SetWindowText(L"");
}

/**
 * TODO: The repolarize operation currently copies the Favorites folder, the Application Data folder, and all the registry keys,
 *     from the old pet account to the new pet account before destroying the old pet.
 *     In other words, it copies just about everything that a successful virus could have stored itself in. So this version of 
 *     of repolarize can't help you save your setting after a virus has corrupted the pet, because the corruption will be copied
 *     over (though this repolarize is still useful if the OS has, because of OS bugs, started doing funny things with the pet
 *     account). A future version must give the user choices about what to save, i.e., keys and/or favorites and/or specific chunks of 
 *     application data. Copying the firefox bookmarks (stored in application data) is probably safe, for example, since a virus
 *     is more likely to have installed itself as an extension than as a bookmark.
 */
void CPolarizer3Dlg::repolarize() {
    // Create a new scope so that the RegKey's handle to the petkey will be closed, and the
    // key will be truly deleted, before the model commits (creating a new key of the same name)
    {        
        RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());

        // get old account from which copying will be performed
        std::wstring petname = getTextFromDialog(&m_petname);
        std::wstring oldAccountName = petsKey.open(petname).getValue(Constants::accountName());

        std::wstring oldAccountPassword = RegKey::HKCU.open(Constants::registryAccounts() + L"\\" + oldAccountName).getValue(Constants::password());
        RegKey oldAccountRegRoot = getUserRegistryRoot(oldAccountName, oldAccountPassword);

        std::wstring newAccountName = makeAccount();
		if (newAccountName.length() == 0) {
			MessageBox(L"Cannot Make New Account", L"Error", MB_OK);
			return;
		}

        //now copy registry and app data from old account to new account
        std::wstring newAccountPassword = RegKey::HKCU.open(Constants::registryAccounts() + L"\\" + newAccountName).getValue(Constants::password());
        RegKey accountRegRoot = getUserRegistryRoot(newAccountName, newAccountPassword);
        deriveRegistry(oldAccountRegRoot, accountRegRoot);
        copyDirectory(Constants::userAppDataPath(Constants::userProfilePath(oldAccountName)),
            Constants::userAppDataPath(Constants::userProfilePath(newAccountName)));
        copyDirectory(Constants::userProfilePath(oldAccountName) + L"\\Favorites",
            Constants::userProfilePath(newAccountName) + L"\\Favorites");
        loadModelFromDialog();
        model.accountName = newAccountName;

        // kill the existing account, then kill the pet registry key,
        // leaving it clean for the new commit
        // TODO: if a pet is running an app when the pet is polarized a second time, what are the consequences?
        //     I think it actually works ok, but it sure is funky
        // TODO: this is badly organized, leading to inconsistencies. Repolarize should use
        //     a well-crafted depolarize as one of its methods. At the moment one of the 
        //     consistencies is, for example, deplorize deletes the shortcuts from the desktop,
        //     repolarize does not. The model.commit, and the polarize from scratch and 
        //     repolarize and depolarize scream for reorg.
		if (oldAccountName.length() == 0) {
			Logger::basicLog(L"!!!old account name zero length " + petname);
			ImportantError dlg;
			dlg.DoModal();
		} else {
			deleteAccount(oldAccountName);
		}
        model.undoRegisteredConfiguration();
        petsKey.deleteSubKey(model.petname);
    }
    model.commit();
    model.clear();
    CompleteBox dlg;
	//m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal(); 
    clearWindow();
}

void CPolarizer3Dlg::polarizeFromScratch() {
    loadModelFromDialog();
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
    std::wstring accountName = makeAccount();
	if (accountName.length() == 0) {
		MessageBox(L"Cannot Make Account", L"Error Polarizing", MB_OK);
		return;
	}
    std::wstring newAccountPassword = RegKey::HKCU.open(Constants::registryAccounts() + L"\\" + accountName).getValue(Constants::password());
    RegKey accountRegRoot = getUserRegistryRoot(accountName, newAccountPassword);
    deriveRegistry(RegKey::HKCU, accountRegRoot);
    // Setup the registry keys so that petmapi works, i.e., you can send mail from the apps in the pet
    // Important safety tip, you cannot do this until after the registry copying is all done
    RegKey softKey = accountRegRoot.create(L"Software");
    RegKey clientKey = softKey.create(L"Clients");
    RegKey mailKey = clientKey.create(L"Mail");
    mailKey.setValue(L"", Constants::polarisBaseName());

	//copy the favorites folder from the real user account. This is not a dangerous 
	// operation, if the real user account favorites is compromised, you have already lost
	std::wstring userPath = _wgetenv(L"USERPROFILE");
    copyDirectory(userPath + L"\\Favorites",
        Constants::userProfilePath(accountName) + L"\\Favorites");
    

    // if this pet already exists, kill the existing account, then kill the pet registry key,
    // leaving it clean for the new commit
    // Create a new scope so that the handle to the petkey will be closed, and the
    // key will be truly deleted, before the model commits (creating a new key of the same name)
    // TODO: if a pet is running an app when the pet is polarized a second time, what are the consequences?
    //     I think it actually works ok, but it sure is funky
    // TODO: This does not properly cleanup the pet's account when a pet is recreated. Need a fuller depolarize
    //     to do this properly
    {
        RegKey parentKey = RegKey::HKCU.open(Constants::registryPets());
        RegKey existingPetKey = parentKey.open(model.petname);
        if (existingPetKey.isGood()) {
            std::wstring oldAccountName = existingPetKey.getValue(L"accountName");
            deleteAccount(oldAccountName);
            parentKey.deleteSubKey(model.petname);
        }
    }
    model.accountName = accountName;
    model.commit();	
    model.clear();
    CompleteBox dlg;
	//m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal(); 
    clearWindow();
	loadExistingPets(&ExistingPetsList);	
}



/**
 * TODO: someday the file extensions should be in a list not a string with embedded crlf
 */
std::vector<std::wstring> CPolarizer3Dlg::getFileExtensions() {
    std::vector<std::wstring> lines = textWidgetToLines(&m_fileExtensionsText);
    std::vector<std::wstring> extensions;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::wstring nextExtension = lines[i];
        if (nextExtension.length() > 0) {
            extensions.push_back(nextExtension);
        }
    }
    return extensions;
}

void CPolarizer3Dlg::loadDialogFromModel() { 
    if (model.executables.size() == 0) {
        m_path.SetWindowText(L"");
    } else {
        m_path.SetWindowText(model.executables[0].c_str());
    }
    m_petname.SetWindowText(model.petname.c_str());
    std::wstring suffixes = L"";
    for (size_t i = 0; i < model.fileExtensions.size(); ++i) {
        suffixes = suffixes + model.fileExtensions[i] + L"\r\n";
    }
    m_fileExtensionsText.SetWindowText(suffixes.c_str());
}


void CPolarizer3Dlg::OnEnChangeEdit3()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

/**
 * This is the SetAuthentication button being clicked
 */
void CPolarizer3Dlg::OnBnClickedButton3()
{
    SetAuthentication auth;
    INT_PTR response = auth.DoModal();

}

void CPolarizer3Dlg::OnEnChangeEdit1()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

/**
 * This is the ExistingPets list having an item selected,
 * load the pet area and the pet model with the selected pet's data
 */
void CPolarizer3Dlg::OnLbnSelchangeList1()
{
    int sel = ExistingPetsList.GetCurSel();
    wchar_t petnameChars[1000];
    ExistingPetsList.GetText(sel,petnameChars);
    model.load(petnameChars);
    loadDialogFromModel();
}

void dePolarize(PetModel* model) {
    std::wstring petname = model->petname;
    model->undoRegisteredConfiguration();
    RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());
    RegKey petKey = petsKey.open(petname);
    std::wstring account = petKey.getValue(Constants::accountName());
    deleteAccount(account);
    petsKey.deleteSubKey(petname);
    std::wstring fromHomeToFile = L"\\Desktop\\";
    fromHomeToFile = fromHomeToFile + petname + L"Safe.lnk";
    std::wstring shortcutPath = _wgetenv(L"USERPROFILE") + fromHomeToFile;
    DeleteFile(shortcutPath.c_str());
}

/**
 * DePolarize button clicked
 */
void CPolarizer3Dlg::OnBnClickedButton5()
{
    wchar_t petnameChars[1000];
    m_petname.GetWindowText(petnameChars, 999);
    std::wstring petname = petnameChars;
    RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());
    RegKey petKey =  petsKey.open(petname);
    if (!petKey.isGood()) {
		MessageBox(L"The Pet specified for depolarization does not exist",
			L"No Such Pet", MB_OK);
    } else {
        model.petname = petname;
        dePolarize(&model); 
        DePolarizationCompleteBox dlg;
	    INT_PTR nResponse = dlg.DoModal();
		loadExistingPets(&ExistingPetsList);
		clearWindow();
    }
}

/**
 * The Configure Endowments button clicked
 */
void CPolarizer3Dlg::OnBnClickedButton4()
{    
        ConfigureEndowmentsBox dlg;
        dlg.setPetModel(&model);
	    INT_PTR nResponse = dlg.DoModal();
}

/**
 * A Known App has been selected from the Known Apps ListBox
 */
void CPolarizer3Dlg::OnLbnSelchangeList2()
{
    int sel = KnownAppsBox.GetCurSel();
    wchar_t petnameChars[1000];
    KnownAppsBox.GetText(sel,petnameChars);
    for (int i = 0; i< knownAppModels.size(); ++i) {
        if (knownAppModels[i]->petname == petnameChars) {
            model.copyFrom(knownAppModels[i]);
        }
    }
    loadDialogFromModel();
}

/**
 * Update Endowments button clicked
 **/
void CPolarizer3Dlg::OnBnClickedButton6() {
	updateEndowments();
}

void CPolarizer3Dlg::updateEndowments() { 
    RegKey petsKey = RegKey::HKCU.open(Constants::registryPets());

    std::wstring petname = getTextFromDialog(&m_petname);
	RegKey petkey = petsKey.open(petname);
	if (!petkey.isGood()) {
		MessageBox(L"The Pet specified for update does not exist",
			L"No Such Pet", MB_OK);
	} else {
		loadModelFromDialog();
		model.accountName = petkey.getValue(Constants::accountName());
		model.undoRegisteredConfiguration();
		petsKey.deleteSubKey(model.petname);
		model.commit();
		model.clear();
		MessageBox(L"Update Complete", L" Update Completed", MB_OK);
		clearWindow();
	}
}

/**
 * This is the Open Pet Desktop button being clicked
 **/
void CPolarizer3Dlg::OnBnClickedButton7()
{
	wchar_t nameBuf[1000] = {};
	m_petname.GetWindowText(nameBuf, 900);
	std::wstring petname = nameBuf;
	RegKey petKey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + petname);
	if (petname.length() == 0 || !petKey.isGood()) {
		MessageBox(L"Current Pet name is not yet associated with a pet", L"No Such Pet", MB_OK);
	} else {
		std::wstring account = petKey.getValue(Constants::accountName());
		std::wstring deskPath = Constants::userProfilePath(account) + L"\\Desktop";
		HINSTANCE result = ShellExecute(      
			NULL,
			L"open",
			deskPath.c_str(),
			NULL,
			deskPath.c_str(),
			SW_SHOW);
		if ((int)result < 32) {
			MessageBox(L"Mysterious Opening Failure", L"Open Failed", MB_OK);
		}
	}
}