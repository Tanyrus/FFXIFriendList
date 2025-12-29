#include "XIFriendList.h"
#include "Debug/Perf.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <windows.h>

#include <Ashita.h>


#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define STDCALL __stdcall
#else
#define EXPORT
#define STDCALL
#endif

static ::XIFriendList::Platform::Ashita::XIFriendList* g_Plugin = nullptr;

extern "C" EXPORT ::IPlugin* STDCALL expCreatePlugin(const char* args)
{
    (void)args;
    ::XIFriendList::Debug::Reset();
    
    if (g_Plugin == nullptr) {
        PERF_SCOPE("PluginEntryPoint::expCreatePlugin new XIFriendList");
        g_Plugin = new ::XIFriendList::Platform::Ashita::XIFriendList();
    }
    
    return g_Plugin;
}

extern "C" EXPORT double STDCALL expGetInterfaceVersion(void)
{
    return ASHITA_INTERFACE_VERSION;
}
