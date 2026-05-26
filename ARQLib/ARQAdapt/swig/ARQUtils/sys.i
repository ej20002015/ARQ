#pragma once

%{
#include <ARQUtils/sys.h>
%}

%nspacemove(ARQ) SysWrapper;
%rename(Sys) SysWrapper;

%inline %{
class SysWrapper {
public:
    static const char* tempDir()    { static const std::string path = ARQ::Sys::tempDir().string();    return path.c_str(); }
    static const char* logDir()     { static const std::string path = ARQ::Sys::logDir().string();     return path.c_str(); }
    static const char* rootCfgDir() { static const std::string path = ARQ::Sys::rootCfgDir().string(); return path.c_str(); }
    static const char* libCfgDir()  { static const std::string path = ARQ::Sys::libCfgDir().string();  return path.c_str(); }
    static const char* svcCfgDir()  { static const std::string path = ARQ::Sys::svcCfgDir().string();  return path.c_str(); }
    static const char* userCfgDir() { static const std::string path = ARQ::Sys::userCfgDir().string(); return path.c_str(); }

private:
    SysWrapper() {} 
};
%}