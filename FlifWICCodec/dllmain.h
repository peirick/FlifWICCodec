#pragma once

#ifdef _DEBUG
#define FLIF_DEBUG_LOGGING
#endif  // DEBUG

#ifdef FLIF_DEBUG_LOGGING
// debug functions
void MAIN_debug_printf(_In_z_ const char* prefix, _In_z_ const char* func, _In_z_ _Printf_format_string_ const char* fmt, ...);
#define TRACE(fmt) MAIN_debug_printf("trace", __FUNCTION__, fmt)
#define TRACE1(fmt, a) MAIN_debug_printf("trace", __FUNCTION__, fmt, a)
#define TRACE2(fmt, a, b) MAIN_debug_printf("trace", __FUNCTION__, fmt, a, b)
#define TRACE3(fmt, a, b, c) MAIN_debug_printf("trace", __FUNCTION__, fmt, a, b, c)
#define TRACE4(fmt, a, b, c, d) MAIN_debug_printf("trace", __FUNCTION__, fmt, a, b, c, d)

char *debugstr_guid(REFGUID guid);
WCHAR *debugstr_var(REFPROPVARIANT var);
#else
#define TRACE(fmt)
#define TRACE1(fmt, a)
#define TRACE2(fmt, a, b)
#define TRACE3(fmt, a, b, c)
#define TRACE4(fmt, a, b, c, d)
#endif

// Number of COM objects created.
extern LONG volatile MAIN_nObjects;