#include "BenLib.h"

#ifdef __DEBUG__
void BenLog(char *format, ...)
{
	va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
#else
void BenLog(char *format, ...) {}
#endif //__DEBUG__