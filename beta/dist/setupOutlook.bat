
rem  Copyright 2010 Hewlett-Packard under the terms of the MIT X license
rem  found at http://www.opensource.org/licenses/mit-license.html

rename "%userprofile%\application data\microsoft\outlook\vbaproject.otm" "%userprofile%\application data\microsoft\outlook\vbaproject.otm.original"

copy vbaproject.otm "%userprofile%\application data\microsoft\outlook\vbaproject.otm"

rename "%userprofile%\application data\microsoft\outlook\outcmd.dat" "%userprofile%\application data\microsoft\outlook\outcmd.dat.original"

copy outcmd.dat "%userprofile%\application data\microsoft\outlook\outcmd.dat"

reg import outlook10security.reg

reg import outlook11security.reg


