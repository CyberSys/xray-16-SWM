#include "stdafx.h"

void VS_Output(const char* szFormat, ...);

void VS_Output(const char* szFormat, ...)
{
    char    szBuff[1024];
    va_list arg;
    va_start(arg, szFormat);
    _vsnprintf(szBuff, sizeof(szBuff), szFormat, arg);
    va_end(arg);

    OutputDebugString(szBuff);
}

//--#SM_TODO+#--