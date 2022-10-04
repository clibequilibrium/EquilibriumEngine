#ifndef DEFINES_H
#define DEFINES_H

/* Convenience macro for exporting symbols */
#ifndef sandbox_STATIC
#if sandbox_EXPORTS && (defined(_MSC_VER) || defined(__MINGW32__))
#define SANDBOX_API __declspec(dllexport)
#elif sandbox_EXPORTS
#define SANDBOX_API __attribute__((__visibility__("default")))
#elif defined _MSC_VER
#define SANDBOX_API __declspec(dllimport)
#else
#define SANDBOX_API
#endif
#else
#define SANDBOX_API
#endif

#endif
