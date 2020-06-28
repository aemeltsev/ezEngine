#pragma once

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RMLUIPLUGIN_LIB
#    define EZ_RMLUIPLUGIN_DLL __declspec(dllexport)
#  else
#    define EZ_RMLUIPLUGIN_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_RMLUIPLUGIN_DLL
#endif

#define RMLUI_USE_CUSTOM_RTTI
