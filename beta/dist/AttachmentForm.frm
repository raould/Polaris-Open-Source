VERSION 5.00
Begin {C62A69F0-16DC-11CE-9E98-00AA00574A4F} AttachmentForm 
   Caption         =   "Select Attachment To Launch"
   ClientHeight    =   1584
   ClientLeft      =   36
   ClientTop       =   324
   ClientWidth     =   3696
   OleObjectBlob   =   "AttachmentForm.frx":0000
   StartUpPosition =   1  'CenterOwner
End
Attribute VB_Name = "AttachmentForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Private Sub AttachmentList_Click()

UserForms(0).Hide
Unload UserForms(0)
End Sub
