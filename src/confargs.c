/* confargs.c - Argument and config handling
 * ConfArgs (c) 2000-2003 Fredrik Rambris <fredrik@rambris.com>
 *
 * ConfArgs is a set of functions for minimizing the effort to handle options
 * from command line as well as from config files.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <popt.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "confargs.h"
#include "util.h"

/* Initialize and allocate space for configuration data */
ConfArg *confargs_new( const ConfArgItem *ca_items, void (*sanity_check)( ConfArg *ca ) )
{
	int number;
	ConfArg *ca=NULL;
	
	if( !ca_items ) return( NULL );
	ca=gfx_new0( ConfArg, 1 );
	ca->ca_items=ca_items;
	for( number=0 ; ca_items[number].ci_type ; number++ );
	ca->ca_values=gfx_new0( void *, number+1 );
	for( number-- ; number>-1 ; number -- )
	{
		switch( ca->ca_items[number].ci_type )
		{
			case CT_ARG_BOOL:
			case CT_ARG_INT:
				(int)(ca->ca_values[number])=(int)(ca->ca_items[number].ci_default);
				break;
			case CT_ARG_STR:
				if( ca->ca_items[number].ci_default ) ca->ca_values[number]=strdup( (char *)ca->ca_items[number].ci_default );
				break;
			default:
				break;
		}
	}
	ca->sanity_check=sanity_check;
	return( ca );
}

/* Free values and configuration data */
void confargs_free( ConfArg *ca )
{
	int number;
	if( !ca ) return;
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( (ca->ca_items[number].ci_type==CT_ARG_STR || ca->ca_items[number].ci_type==CT_ARG_INTARRAY) && ca->ca_values[number] ) free( ca->ca_values[number] );
	}
	free( ca->ca_values );
	free( ca );
}

/* Make a copy of a confarg */
ConfArg *confargs_copy( const ConfArg *ca )
{
	int number;
	ConfArg *newca=NULL;
	if( !ca ) return NULL;
	newca=gfx_new0( ConfArg, 1 );
	newca->ca_items=ca->ca_items;
	for( number=0 ; ca->ca_items[number].ci_type ; number++ );
	newca->ca_values=gfx_new0( void *, number+1 );
	for( number-- ; number>-1 ; number -- )
	{
		switch( ca->ca_items[number].ci_type )
		{
			case CT_ARG_BOOL:
			case CT_ARG_INT:
				newca->ca_values[number]=ca->ca_values[number];
				break;
			case CT_ARG_STR:
				newca->ca_values[number]=strdup( (char *)ca->ca_values[number] );
				break;
			case CT_ARG_INTARRAY:
				newca->ca_values[number]=arrdup( (int *)ca->ca_values[number] );
			default:
				break;
		}
	}
	return( newca );
}

/* Get a value by name */
void *confargs_get_value( ConfArg *ca, const char *name )
{
	int number;
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( fastcasecompare( ca->ca_items[number].ci_name, name ) ) return( ca->ca_values[number] );
	}
	fprintf( stderr, "Value '%s' doesn't exist.\n", name );
	return( NULL );
}

/* Set a value by name */
void confargs_set_value( ConfArg *ca, const char *name, void *value )
{
	int number;
	int *numbers;
	int count;
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( fastcasecompare( ca->ca_items[number].ci_name, name ) )
		{
			if( ca->ca_items[number].ci_sanity_check ) ca->ca_items[number].ci_sanity_check( (void *)&ca->ca_items[number], &ca->ca_values[number] );
			switch( ca->ca_items[number].ci_type )
			{
				case CT_ARG_STR:
					ca->ca_values[number]=setstr( ca->ca_values[number], value );
					break;
				case CT_ARG_INTARRAY:
					numbers=(int *)value;
					count=0;
					while( numbers[count++] );
					if( ca->ca_values[number] ) free( ca->ca_values[number] );
					ca->ca_values[number]=gfx_new( int, count );
					memcpy( ca->ca_values[number], value, sizeof( int )*count );
					break;
				default:
					ca->ca_values[number]=value;	
					break;
			}
			if( ca->sanity_check ) ca->sanity_check( ca );
			return;
		}
	}
	fprintf( stderr, "Value '%s' doesn't exist.\n", name );
	return;
}

/* Print out the contents of current config */
void confargs_show( ConfArg *ca )
{
	int number;
	int *num;
	char *types[]=
	{
		NULL,
		"INT",
		"STR",
		"BOOL"
	};
	char *format[]=
	{
		NULL,
		"%s (%s): %d (%d)\n",
		"%s (%s): %s (%s)\n",
		"%s (%s): %s (%s)\n"
	};
	printf( "NAME (TYPE): VALUE (DEFAULT)\n" );
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( ca->ca_items[number].ci_type==CT_ARG_INTARRAY )
		{
			num=(int *)ca->ca_values[number];
			printf( "%s (INTARRAY):", ca->ca_items[number].ci_name );
			if( !num ) printf( " (null)" );
			else
			{
				while( *num )
				{
					printf( " %d", *num );
					num++;
				}
			}
			printf( "\n" );
		}
		else printf( format[ca->ca_items[number].ci_type],
				ca->ca_items[number].ci_name,
				types[ca->ca_items[number].ci_type],
				(ca->ca_items[number].ci_type==CT_ARG_BOOL?((BOOL)ca->ca_values[number]?"True":"False"):ca->ca_values[number] ), (ca->ca_items[number].ci_type==CT_ARG_BOOL?((BOOL)ca->ca_items[number].ci_default?"True":"False"):ca->ca_items[number].ci_default ) );
	}

}

/* Load values from file */
BOOL confargs_load( ConfArg *ca, char *filename )
{
	char buf[1024], *s, *a;
	FILE *fp=NULL;
	int count;
	char *answers="01NYFTnyft";
	BOOL ret=FALSE;
	char path[1024];

	if( !( (ca) && (filename) ) ) return( FALSE );

	if( !strncmp( filename, "~/", 2 ) )
	{
		strcpy( (char *)path, (char *)getenv( "HOME" ) );
		strcat( (char *)path, (char *)(filename+1) );
	}
	else strcpy( (char *)path, (char *)filename );
	
	if( fastcompare( filename, "stdin" ) ) fp=stdin;
	else if( fastcompare( filename, "-" ) ) fp=stdin;
	else if( !( ( fp=fopen( path, "r" ) ) ) ) { fprintf(stderr,"Couldn't open prefsfile" ); return( FALSE ); }

	while( fgets( buf, 1023, fp ) )
	{
		/* First strip off comments */
		if( ( s=strstr( buf, "//" ) ) )
		{
			s[0]='\0';
		}
		/* Strip of whitespaces */
		if( ( s=strchr( buf, '=' ) ) )
		{
			s[0]='\0';
			s++;
			stripws( buf );
			stripws( s );
			strtolower(buf);
			for( count=0 ; ca->ca_items[count].ci_name ; count++ )
			{
				if( (ca->ca_items[count].ci_avail&CIA_CONFIG) && fastcasecompare( ca->ca_items[count].ci_name, buf ) )
				{
					if( ca->ca_items[count].ci_type==CT_ARG_INT )
					{
						ca->ca_values[count]=(void *)( atoi( s ) );
						if( ca->ca_items[count].ci_sanity_check ) ca->ca_items[count].ci_sanity_check( (void *)&ca->ca_items[count], &ca->ca_values[count] );
					}
					else if( ca->ca_items[count].ci_type==CT_ARG_STR )
					{
						if( ca->ca_values[count] ) free( ca->ca_values[count] );
						if( strlen( s ) )
						{
							ca->ca_values[count]=strdup( s );
							if( ca->ca_items[count].ci_sanity_check ) ca->ca_items[count].ci_sanity_check( (void *)&ca->ca_items[count], &ca->ca_values[count] );
						}
					}
					else if( ca->ca_items[count].ci_type==CT_ARG_BOOL )
					{
						if( ( a=strchr( answers, s[0] ) ) )
						{
							ca->ca_values[count]=(void *)( ((((int)(a-answers))%2)==1) );
							if( ca->ca_items[count].ci_sanity_check ) ca->ca_items[count].ci_sanity_check( (void *)&ca->ca_items[count], &ca->ca_values[count] );
						}
					}
					else if( ca->ca_items[count].ci_type==CT_ARG_INTARRAY )
					{
						if( ca->ca_values[count] ) free( ca->ca_values[count] );
						ca->ca_values[count]=(void *)strtoarr( s );
						if( ca->ca_items[count].ci_sanity_check ) ca->ca_items[count].ci_sanity_check( (void *)&ca->ca_items[count], &ca->ca_values[count] );
					}					
				}
			}
			ret=TRUE;
		}
	}
	if( ca->sanity_check ) ca->sanity_check( ca );
	if( fp!=stdin ) fclose( fp );
	return( ret );
}

/* Save values to file */
BOOL confargs_save( ConfArg *ca, char *filename )
{
	FILE *fp;
	BOOL ret=FALSE;
	int number;
	char *types[]=
	{
		NULL,
		"INT",
		"STR",
		"BOOL"
	};
	char path[1024];
	if( !( (ca) && (filename) ) ) return( ret );

	if( !strncmp( filename, "~/", 2 ) )
	{
		strcpy( (char *)path, (char *)getenv( "HOME" ) );
		strcat( (char *)path, (char *)(filename+1) );
	}
	else strcpy( (char *)path, (char *)filename );

	if( fastcompare( filename, "stdout" ) ) fp=stdout;
	else if( fastcompare( filename, "-" ) ) fp=stdout;
	else if( !( ( fp=fopen( path, "w" ) ) ) ) return( FALSE );
	fprintf( fp, "// GFXIndex configuration file.\n\n" );
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( ca->ca_items[number].ci_avail & CIA_CONFIG )
		{
			switch( ca->ca_items[number].ci_type )
			{
				case CT_ARG_BOOL:
					/* If the value is the same as default then comment it out */
					if( ca->ca_items[number].ci_default==ca->ca_values[number] ) fprintf( fp, "// " );
					fprintf( fp, "%s = %s // %s: %s. Default: %s\n", ca->ca_items[number].ci_name, (ca->ca_values[number]?"Yes":"No"), types[ca->ca_items[number].ci_type], ca->ca_items[number].ci_description, (ca->ca_items[number].ci_default?"Yes":"No") );
					break;

				case CT_ARG_INT:
					if( ca->ca_items[number].ci_default==ca->ca_values[number] ) fprintf( fp, "// " );
					fprintf( fp, "%s = %d // %s: %s. Default: %d\n", ca->ca_items[number].ci_name, (int)( ca->ca_values[number] ), types[ca->ca_items[number].ci_type], ca->ca_items[number].ci_description, (int)( ca->ca_items[number].ci_default ) );
					break;

				case CT_ARG_STR:
					if( fastcompare( ca->ca_items[number].ci_default, ca->ca_values[number] ) ) fprintf( fp, "// " );
					fprintf( fp, "%s = %s // %s: %s. Default: %s\n", ca->ca_items[number].ci_name, (ca->ca_values[number]?(char *)ca->ca_values[number]:""), types[ca->ca_items[number].ci_type], ca->ca_items[number].ci_description, (ca->ca_items[number].ci_default?(char *)ca->ca_items[number].ci_default:"") );
					break;
				default:
					break;
			}
		}
	}
	fprintf( fp, "\n// End of file\n" );
	ret=TRUE;
	if( fp!=stdout ) fclose( fp );
	return( ret );
}

/* Use popt to take options from command line */
BOOL confargs_commandline( ConfArg *ca, int argc, char **argv )
{
	BOOL ret=FALSE;
	poptContext optCon;
	int number, add, sub;
	struct poptOption *optionsTable;
	char **boolnames=NULL;
	if( !ca ) return( ret );

	sub=add=0;
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( !(ca->ca_items[number].ci_avail&CIA_CMDLINE) ) sub++;
		else if( ca->ca_items[number].ci_type==CT_ARG_BOOL ) add++;
	}
	if( !(optionsTable=gfx_new0( struct poptOption, number+add-sub+2 ) ) ) goto error;
	if( !(boolnames=gfx_new0( char *, add+1 ) ) ) goto error;
	sub=add=0;
	for( number=0 ; ca->ca_items[number].ci_type ; number++ )
	{
		if( ca->ca_items[number].ci_avail & CIA_CMDLINE )
		{
			optionsTable[number+add-sub].descrip=ca->ca_items[number].ci_description;
			switch( ca->ca_items[number].ci_type )
			{
				case CT_ARG_STR:
				case CT_ARG_INT:
				case CT_ARG_INTARRAY:
					optionsTable[number+add-sub].longName=ca->ca_items[number].ci_name;
					optionsTable[number+add-sub].shortName=ca->ca_items[number].ci_accel;
					optionsTable[number+add-sub].val=number+1;
					if( ca->ca_items[number].ci_type==CT_ARG_INT ) optionsTable[number+add].argInfo=POPT_ARG_INT;
					else optionsTable[number+add-sub].argInfo=POPT_ARG_STRING;
					break;
				case CT_ARG_BOOL:
					if( !( boolnames[add]=gfx_new( char, strlen( ca->ca_items[number].ci_name )+3 ) ) ) goto error;
					strcpy( boolnames[add], "no" );
					strcat( boolnames[add], ca->ca_items[number].ci_name );
					if( (int)( ca->ca_items[number].ci_default ) )
					{
						optionsTable[number+add-sub].longName=boolnames[add];
						optionsTable[number+add-sub].shortName=ca->ca_items[number].ci_accel;
						optionsTable[number+add-sub].argInfo=POPT_ARG_VAL;
						optionsTable[number+add-sub].arg=(int *)&ca->ca_values[number];
						optionsTable[number+add-sub].val=0;
						add++;
						optionsTable[number+add-sub].longName=ca->ca_items[number].ci_name;
						optionsTable[number+add-sub].argInfo=POPT_ARG_VAL;
						optionsTable[number+add-sub].arg=(int *)&ca->ca_values[number];
						optionsTable[number+add-sub].val=1;
					}
					else
					{
						optionsTable[number+add-sub].longName=ca->ca_items[number].ci_name;;
						optionsTable[number+add-sub].shortName=ca->ca_items[number].ci_accel;
						optionsTable[number+add-sub].argInfo=POPT_ARG_VAL;
						optionsTable[number+add-sub].arg=(int *)&ca->ca_values[number];
						optionsTable[number+add-sub].val=1;
						add++;
						optionsTable[number+add-sub].longName=boolnames[add-1];
						optionsTable[number+add-sub].argInfo=POPT_ARG_VAL;
						optionsTable[number+add-sub].arg=(int *)&ca->ca_values[number];
						optionsTable[number+add-sub].val=0;
					}
					break;
				default:
					break;
			}
		}
		else sub++;
	}

	optionsTable[number+add-sub].argInfo=POPT_ARG_INCLUDE_TABLE;
	optionsTable[number+add-sub].arg=poptHelpOptions;
	optionsTable[number+add-sub].descrip=(char *)"Help options:";

	optCon = poptGetContext( NULL, argc, (const char **)argv, optionsTable, 0 );
	while( ( number=poptGetNextOpt( optCon ) )>=0 )
	{
		number--;
		switch( ca->ca_items[number].ci_type )
		{
			case CT_ARG_INT:
				ca->ca_values[number]=(void *)( atoi( poptGetOptArg( optCon ) ) );
				if( ca->ca_items[number].ci_sanity_check ) ca->ca_items[number].ci_sanity_check( (void *)&ca->ca_items[number], &ca->ca_values[number] );
				break;
			case CT_ARG_STR:
				if( ca->ca_values[number] ) free( ca->ca_values[number] );
				ca->ca_values[number]=strdup( poptGetOptArg( optCon ) );
				if( ca->ca_items[number].ci_sanity_check ) ca->ca_items[number].ci_sanity_check( (void *)&ca->ca_items[number], &ca->ca_values[number] );
				break;
			case CT_ARG_INTARRAY:
				if( ca->ca_values[number] ) free( ca->ca_values[number] );
				ca->ca_values[number]=(void *)strtoarr( poptGetOptArg( optCon ) );
				if( ca->ca_items[number].ci_sanity_check ) ca->ca_items[number].ci_sanity_check( (void *)&ca->ca_items[number], &ca->ca_values[number] );
				break;
			default:
				break;
		}

	}
	poptFreeContext(optCon);
	ret=TRUE;
	if( ca->sanity_check ) ca->sanity_check( ca );
error:
	if( boolnames )
	{
		for( number=0; boolnames[number] ; number++ ) free( boolnames[number] );
		free( boolnames );
	}
	if( optionsTable ) free( optionsTable );
	return( ret );
}
