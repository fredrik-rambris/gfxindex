#ifdef __WIN32__
/* Here are some tweaks that has to be made to get it compile under MinGW32
 * basename isn't part of liberty (as I thought it would be) and I got some
 * linkage error with the rest */
#include <popt.h>
#include <string.h>
#include <stdarg.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

char *basename( char *str )
{
	char *ptr=strrchr( str, '\\' );
	if( ptr )
	{
		ptr++;
		return ptr;
	}
	else return str;
}

#ifdef STATIC
/* I don't use this... so why bother... */
char *libintl_dgettext (const char * domainname, const char * msgid)
{
	return (char *)msgid;
}

extern struct poptOption poptHelpOptions[];
struct poptOption *_imp__poptHelpOptions=poptHelpOptions;
#endif

int myfprintf(FILE *stream, const char *format, ...)
{
	int ret=0;
	va_list ap;
	static char buf[1024*16]="";
	int s;
	va_start(ap,format);
	vsprintf( buf, format, ap );
	s=strlen( buf );
	if( s )
	{
		while( --s )
		{
			if( buf[s]=='\\' ) buf[s]='/';
		}
		ret=fprintf( stream, buf );
	}
	va_end(ap);
	return ret;
}


#endif
