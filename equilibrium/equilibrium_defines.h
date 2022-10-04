#ifndef EQUILIBRIUM_DEFINES_H
#define EQUILIBRIUM_DEFINES_H

/* Convenience macro for exporting symbols */
#ifndef equilibrium_STATIC
#if equilibrium_EXPORTS && (defined(_MSC_VER) || defined(__MINGW32__))
#define EQUILIBRIUM_API __declspec(dllexport)
#elif equilibrium_EXPORTS
#define EQUILIBRIUM_API __attribute__((__visibility__("default")))
#elif defined _MSC_VER
#define EQUILIBRIUM_API __declspec(dllimport)
#else
#define EQUILIBRIUM_API
#endif
#else
#define EQUILIBRIUM_API
#endif

#endif
