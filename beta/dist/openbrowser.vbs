Function randomName()
	Randomize
	Dim name, digit, number
	name = ""
	While (Len(name) < 20)
	    name = name & Trim(CStr(Int(Rnd() * 10000)))
	Wend
	randomName = name
End Function

Sub writeOpenBrowserRequest()
Dim fs, requestFile, stream, requestPath, requestName 
Dim tempRequestPath, url, blahShell, testPath

if (WScript.Arguments.Count) >= 1 then
	url = WScript.Arguments(0)
else
	url = ""
End If
	set blahShell = WScript.CreateObject("WScript.Shell")
	Set fs = CreateObject("Scripting.FileSystemObject")
	
	requestName = randomName & ".request"
	requestPath = blahShell.ExpandEnvironmentStrings("%USERPROFILE%") & "\Application Data\Hewlett-Packard\Polaris\requests\" & requestName
	'requestPath = "c:\documents and settings\marcstgr" & "\Application Data\Hewlett-Packard\Polaris\requests\" & requestName
	tempRequestPath = requestPath & ".temp"
	fs.CreateTextFile (tempRequestPath)
	Set requestFile = fs.GetFile(tempRequestPath)
	Set stream = requestFile.OpenAsTextStream(2, -2)
	stream.Write ("openbrowser" & Chr(0) & Chr(34) & url & Chr(34))
	stream.Close
	requestFile.Name = requestName
End Sub

Call writeOpenBrowserRequest()