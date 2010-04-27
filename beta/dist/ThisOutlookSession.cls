VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "ThisOutlookSession"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = True

Public Sub OpenStrict()
Dim myOutlook As Object, myNameSpace As Object
Dim nowItem As Object, attachmentsCount As Integer
Dim selectionsCount, attachmentSelectedIndex As Integer
Dim launchable As Attachment
Set myOutlook = CreateObject("Outlook.Application")
selectionsCount = myOutlook.ActiveExplorer.Selection.Count
If (selectionsCount = 0) Then
    MsgBox ("No Message Selected")
    Return
End If

Set nowItem = myOutlook.ActiveExplorer.Selection.Item(1)
attachmentsCount = nowItem.Attachments.Count
If (attachmentsCount = 0) Then
    MsgBox ("No Attachments On Selected Message")
    Return
End If
    
If (attachmentsCount = 1) Then
    attachmentSelectedIndex = 1
Else
    Dim attachBox As New AttachmentForm
    Dim i
    For i = 1 To attachmentsCount
        attachBox.AttachmentList.AddItem (nowItem.Attachments.Item(i))
    Next
    attachBox.Show
    attachmentSelectedIndex = attachBox.AttachmentList.ListIndex + 1
    
End If
        
Set launchable = nowItem.Attachments.Item(attachmentSelectedIndex)
attachedFilePath = Environ("USERPROFILE") & "\Local Settings\Temp\" & launchable.FileName
launchable.SaveAsFile (attachedFilePath)
writeRequest (attachedFilePath)

End Sub


'******* open from message window ****

'99% of code for these 2 opens is duplicated. Someday, condense
Public Sub OpenFromMessageWindow()
Dim myOutlook As Object, myNameSpace As Object
Dim nowItem As Object, attachmentsCount As Integer
Dim selectionsCount, attachmentSelectedIndex As Integer
Dim launchable As Attachment
Set myOutlook = CreateObject("Outlook.Application")

Set nowItem = myOutlook.ActiveInspector.CurrentItem
attachmentsCount = nowItem.Attachments.Count
If (attachmentsCount = 0) Then
    MsgBox ("No Attachments On Selected Message")
    Return
End If
    
If (attachmentsCount = 1) Then
    attachmentSelectedIndex = 1
Else
    Dim attachBox As New AttachmentForm
    Dim i
    For i = 1 To attachmentsCount
        attachBox.AttachmentList.AddItem (nowItem.Attachments.Item(i))
    Next
    attachBox.Show
    attachmentSelectedIndex = attachBox.AttachmentList.ListIndex + 1
    
End If
        
Set launchable = nowItem.Attachments.Item(attachmentSelectedIndex)
Dim fs, requestFile, stream, attachedFilePath, requestPath, requestName, tempRequestPath
'Set fs = CreateObject("Scripting.FileSystemObject")
attachedFilePath = Environ("USERPROFILE") & "\Local Settings\Temp\" & launchable.FileName
launchable.SaveAsFile (attachedFilePath)
writeRequest (attachedFilePath)
'requestName = randomName & ".request"
'requestPath = Environ("USERPROFILE") & "\Application Data\Hewlett-Packard\Polaris\requests\" & requestName
'tempRequestPath = requestPath & ".temp"
'fs.CreateTextFile (tempRequestPath)
'Set requestFile = fs.GetFile(tempRequestPath)
'Set stream = requestFile.OpenAsTextStream(2, -2)
'stream.Write ("openstrict" & Chr(0) & Chr(34) & attachedFilePath & Chr(34))
    'stream.writeline (Chr(34) & computeAcct() & Chr(34))
    'stream.writeline ("0")
    'stream.writeline (Chr(34) & Replace(attachedFilePath, "\", "\x5c") & Chr(34))
'stream.Close
'FileCopy tempRequestPath, "c:\test.request"
'Name tempRequestPath As requestPath

End Sub


Function randomName()
Randomize
Dim name, digit, number
name = ""
While (Len(name) < 20)
    name = name & Trim(Str(Int(Rnd() * 10000)))
Wend
randomName = name
End Function

Function computeAcct()
Dim acct
acct = Environ("USERNAME")
computeAcct = acct
End Function

Public Sub writeRequest(attachedFilePath)
Dim fs, requestFile, stream, requestPath, requestName, tempRequestPath
Set fs = CreateObject("Scripting.FileSystemObject")
requestName = randomName & ".request"
requestPath = Environ("USERPROFILE") & "\Application Data\Hewlett-Packard\Polaris\requests\" & requestName
tempRequestPath = requestPath & ".temp"
fs.CreateTextFile (tempRequestPath)
Set requestFile = fs.GetFile(tempRequestPath)
Set stream = requestFile.OpenAsTextStream(2, -2)
stream.Write ("openstrict" & Chr(0) & Chr(34) & Chr(0))
For i = 1 To Len(attachedFilePath)
    stream.Write (Mid(attachedFilePath, i, 1))
    stream.Write (Chr(0))
Next
stream.Write (Chr(34) & Chr(0))
stream.Write (Chr(0) & Chr(0) & Chr(0))
stream.Close
Name tempRequestPath As requestPath
End Sub


