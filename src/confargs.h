#ifndef CONFARGS_H
#define CONFARGS_H

#include "gfx_types.h"

typedef enum
{
	CT_ARG_END=0,
	CT_ARG_INT,
	CT_ARG_STR,
	CT_ARG_BOOL,
	CT_ARG_INTARRAY,
	CT_ARG_ACTION
} ConfArgItemType;

/* Defines for ci_avail */
#define CIA_NONE (0L)
#define CIA_CONFIG (1L)
#define CIA_CMDLINE (2L)
#define CIA_BOTH (3L)

typedef struct _ConfArgItem
{
	/* Name of item. item = value where item is ci_name */
	char *ci_name;
	/* Type of value, integer, string or bool */
	ConfArgItemType ci_type;
	/* Accelerator. Used for shortoptions in popt. */
	char ci_accel;
	/* Where this option is available. Bit0=Configfile,Bit1=Commandline */
	unsigned int ci_avail;
	/* Default value */
	const void *ci_default;
	/* A description of the item */
	const char *ci_description;
	/* An optional pointer to a function used to check sanity of value */
	void (*ci_sanity_check) ( void *ci, void *value );
} ConfArgItem;

typedef struct _ConfArg
{
	const ConfArgItem *ca_items;
	void **ca_values;
	void (*sanity_check)();
} ConfArg;

/* Prototypes */
ConfArg *confargs_new( const ConfArgItem *ca_items, void (*sanity_check)( ConfArg *ca ) );
ConfArg *confargs_copy( const ConfArg *ca );
void confargs_free( ConfArg *ca );
void *confargs_get_value( ConfArg *ca, const char *name );
void confargs_show( ConfArg *ca );
BOOL confargs_load( ConfArg *ca, char *filename );
BOOL confargs_save( ConfArg *ca, char *filename );
BOOL confargs_commandline( ConfArg *ca, int argc, char **argv );

#endif
