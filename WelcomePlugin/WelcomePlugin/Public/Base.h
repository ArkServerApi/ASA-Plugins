#pragma once

#ifdef WELCOMEPLUGIN_EXPORTS
#define WELCOMEPLUGIN_API __declspec(dllexport)
#else
#define WELCOMEPLUGIN_API __declspec(dllimport)
#endif
