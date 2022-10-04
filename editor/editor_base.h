#ifndef EDITOR_BASE_H
#define EDITOR_BASE_H

/* Convenience macro for exporting symbols */
#ifndef editor_STATIC
#if editor_EXPORTS && (defined(_MSC_VER) || defined(__MINGW32__))
#define EDITOR_API __declspec(dllexport)
#elif editor_EXPORTS
#define EDITOR_API __attribute__((__visibility__("default")))
#elif defined _MSC_VER
#define EDITOR_API __declspec(dllimport)
#else
#define EDITOR_API
#endif
#else
#define EDITOR_API
#endif

#include <entity.h>
#include <world.h>

#endif
