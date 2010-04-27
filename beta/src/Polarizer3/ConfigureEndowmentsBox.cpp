// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// ConfigureEndowmentsBox.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "ConfigureEndowmentsBox.h"
#include "TextFunctions.h"
#include ".\configureendowmentsbox.h"
//#include <Afxwin.h>

// ConfigureEndowmentsBox dialog

IMPLEMENT_DYNAMIC(ConfigureEndowmentsBox, CDialog)
ConfigureEndowmentsBox::ConfigureEndowmentsBox(CWnd* pParent /*=NULL*/)
	: CDialog(ConfigureEndowmentsBox::IDD, pParent)
{
}

ConfigureEndowmentsBox::~ConfigureEndowmentsBox()
{
}

void ConfigureEndowmentsBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_AuthenticatingServerSitesField);
	DDX_Control(pDX, IDC_EDIT2, m_additionalApps);
	DDX_Control(pDX, IDC_EDIT3, m_editableFolders);
	DDX_Control(pDX, IDC_CHECK1, m_readOnlyWithLinks);
	DDX_Control(pDX, IDC_CHECK2, m_allowTempEdit);
	DDX_Control(pDX, IDC_CHECK3, m_allowAppFolderEdit);
	DDX_Control(pDX, IDC_CHECK4, m_setAsDefaultBrowserButton);
	DDX_Control(pDX, IDC_EDIT4, m_readonlyFolders);
	DDX_Control(pDX, IDC_CHECK5, m_setAsDefaultMailClientCheck);
	DDX_Control(pDX, IDC_EDIT5, m_additionalCommandLineArgsBox);
	DDX_Control(pDX, IDC_CHECK6, fullNetworkCredentialsButton);
	DDX_Control(pDX, IDC_CHECK7, m_allowOpAfterWindowClosed);
}


BEGIN_MESSAGE_MAP(ConfigureEndowmentsBox, CDialog)
    ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_CHECK4, OnBnClickedCheck4)
    ON_BN_CLICKED(IDC_CHECK5, OnBnClickedCheck5)
	ON_BN_CLICKED(IDC_CHECK7, OnBnClickedCheck7)
	ON_BN_CLICKED(IDC_CHECK1, OnBnClickedCheck1)
END_MESSAGE_MAP()


// ConfigureEndowmentsBox message handlers

BOOL ConfigureEndowmentsBox::OnInitDialog() {
    __super::OnInitDialog();
    // gotta skip the first item in the executables, it goes
    // on the front page, not the endowments page
    std::vector<std::wstring> secondPageExecutables;
    for (size_t i = 1; i < m_model->executables.size(); i++) {
        secondPageExecutables.push_back(m_model->executables[i]);
    }
    linesToWidget(secondPageExecutables, &m_additionalApps);
    linesToWidget(m_model->serversToAuthenticate, &m_AuthenticatingServerSitesField);
    linesToWidget(m_model->editableFolders, &m_editableFolders);
    linesToWidget(m_model->readonlyFolders, &m_readonlyFolders);

    //m_model->readOnlyWithLinks = m_readOnlyWithLinks.GetCheck() == BST_CHECKED;
    m_readOnlyWithLinks.SetCheck(m_model->readOnlyWithLinks);
    m_allowTempEdit.SetCheck(m_model->allowTempEdit);
    m_allowAppFolderEdit.SetCheck(m_model->allowAppFolderEdit);
    m_setAsDefaultBrowserButton.SetCheck(m_model->isDefaultBrowserPet);
    m_setAsDefaultMailClientCheck.SetCheck(m_model->isDefaultMailPet);
    m_additionalCommandLineArgsBox.SetWindowText(m_model->commandLineArgs.c_str());
	m_allowOpAfterWindowClosed.SetCheck(m_model->allowOpAfterWindowClosed);
	fullNetworkCredentialsButton.SetCheck(m_model->grantFullNetworkCredentials);
    return TRUE;
}

void ConfigureEndowmentsBox::setPetModel(PetModel* petModel){
    m_model = petModel;
}

void ConfigureEndowmentsBox::OnEnChangeEdit1()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialog::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}

/**
 * OK/apply button clicked, apply the endowments to the petmodel
 */
void ConfigureEndowmentsBox::OnBnClickedOk()
{
    m_model->clear();
    m_model->serversToAuthenticate = textWidgetToLines(&m_AuthenticatingServerSitesField);
    // there is an additional executable in the executables collection, namely, the 
    // "main" executable for which we set up default-launch file extensions. If 
    // users need it, expand user interface so default extensions can be created for all
    // the executables.
    // meanwhile, need to leave executables[0] blank for the 
    // eventual insertion of the "main" executable
    m_model->executables = textWidgetToLines(&m_additionalApps);
    if (m_model->executables.size() > 0) {
        m_model->executables.push_back(m_model->executables[0]);
        m_model->executables[0] = L"";
    }
    m_model->editableFolders = textWidgetToLines(&m_editableFolders);
    m_model->readonlyFolders = textWidgetToLines(&m_readonlyFolders);
    m_model->readOnlyWithLinks = m_readOnlyWithLinks.GetCheck() == BST_CHECKED;
	m_model->allowOpAfterWindowClosed = m_allowOpAfterWindowClosed.GetCheck() == BST_CHECKED;
	m_model->grantFullNetworkCredentials = fullNetworkCredentialsButton.GetCheck() == BST_CHECKED;
    m_model->allowTempEdit = m_allowTempEdit.GetCheck() == BST_CHECKED;
    m_model->allowAppFolderEdit = m_allowAppFolderEdit.GetCheck() == BST_CHECKED;
    m_model->isDefaultBrowserPet = m_setAsDefaultBrowserButton.GetCheck()== BST_CHECKED;
    m_model->isDefaultMailPet = m_setAsDefaultMailClientCheck.GetCheck()== BST_CHECKED;
    std::vector<std::wstring> commandLine = textWidgetToLines(&m_additionalCommandLineArgsBox);
    m_model->commandLineArgs = L"";
    if (commandLine.size() > 0) {m_model->commandLineArgs = commandLine[0];}
    OnOK();
}

// this is the SetasDefaultBrowser button being clicked
void ConfigureEndowmentsBox::OnBnClickedCheck4()
{
    // TODO: Add your control notification handler code here
}

void ConfigureEndowmentsBox::OnBnClickedCheck5()
{
    // TODO: Add your control notification handler code here
}

void ConfigureEndowmentsBox::OnBnClickedCheck7()
{
	// TODO: Add your control notification handler code here
}

void ConfigureEndowmentsBox::OnBnClickedCheck1()
{
	// TODO: Add your control notification handler code here
}
