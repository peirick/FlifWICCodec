[![Build status](https://ci.appveyor.com/api/projects/status/1nr6guwkvyxui3rt?svg=true)](https://ci.appveyor.com/project/peirick/flifwiccodec)

# FLIF Windows Codec
This plugin allows to **decode** and **encode** [FLIF](http://flif.com) files in Windows aplications using the Windows Imaging Component (WIC) API. That allows e.g., to see the files
in Windows PhotoViewer and Windows Explorer.

## Build Instructions
1. Open Visual Studio 2015 and open FlifWICCodec.sln
2. Compile

## Installation
1. open an administrative command prompt
2. navigate to folder with the FlifWICCodec.dll
3. execute:
```
regsvr32 FlifWICCodec.dll
```
4. on a 64bit system, execute:
```
%SystemRoot%\Syswow64\regsvr32 FlifWICCodec.dll
```

## Uninstall
1. open an administrative command prompt
2. navigate to folder with the FlifWICCodec.dll
3. execute:
```
regsvr32 -u FlifWICCodec.dll
```
4. on a 64bit system, execute:
```
%SystemRoot%\Syswow64\regsvr32 -u FlifWICCodec.dll
```

## Used Repositories
* [https://github.com/FLIF-hub/FLIF](https://github.com/FLIF-hub/FLIF)
