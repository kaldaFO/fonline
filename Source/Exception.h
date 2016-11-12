#ifndef ___EXCEPTION___
#define ___EXCEPTION___

// Uncomment for use minidumps (dmp) instead readable dumps (txt)
// #define EXCEPTION_MINIDUMP

extern void CatchExceptions( const char* app_name, int app_ver );
extern void CreateDump( const char* appendix, const char* message );
extern bool RaiseAssert( const char* message, const char* file, int line );

#endif // ___EXCEPTION___
