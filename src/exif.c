#if HAVE_CONFIG_H
#include <config.h>
#endif


#include "exif.h"
#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-tag.h>

ExifEntry *get_exif_entry( ExifData *ed, ExifTag t )
{
    int i;
	ExifEntry *e;
    for( i=0; i<EXIF_IFD_COUNT; i++ ) 
    {
        if( ed->ifd[i] && ed->ifd[i]->count )
		{
			if( ( e=exif_content_get_entry( ed->ifd[i], t ) ) )
			{
				return e;
			}
		}
    }

    return 0;

}

const char *get_exif_value(ExifData *ed, ExifTag t)
{
#if HAVE_LIBEXIF == 10
	static char buf[1024*16];
#endif
	ExifEntry *e;
	if( ( e=get_exif_entry( ed, t ) ) )
	{
#if HAVE_LIBEXIF == 10
		return exif_entry_get_value( e, (char *)buf, (unsigned int)(1024*16)-1 );
#else
		return exif_entry_get_value( e );
#endif
	}
	return NULL;
}

int get_exif_rotate( ExifData *ed )
{
	ExifShort orientation;
	ExifEntry *e;
	if( ( e=get_exif_entry( ed, EXIF_TAG_ORIENTATION ) ) )
	{
		orientation=exif_get_short( e->data, exif_data_get_byte_order(ed) );
		if( orientation==6 ) return 90;
		else if( orientation==8 ) return 270;
	}
    return 0;
}

int get_exif_flash( ExifData *ed )
{
	ExifShort flash;
	ExifEntry *e;
	if( ( e=get_exif_entry( ed, EXIF_TAG_FLASH ) ) )
	{
		flash=exif_get_short( e->data, exif_data_get_byte_order(ed) );
		return flash & 0x0001;
	}
    return 0;
}

#endif
ExifInfo *gfx_exif_file( char *file )
{
	ExifInfo *ei=NULL;
#ifdef HAVE_LIBEXIF
	ExifData *ed;
	if( !STR_ISSET(file) ) return ei;
	if( ( ed=exif_data_new_from_file( file ) ) )
	{
		if( ( ei=gfx_new0( ExifInfo, 1 ) ) )
		{
			ei->ei_make=setstr( ei->ei_make, get_exif_value( ed, EXIF_TAG_MAKE ) );
			ei->ei_model=setstr( ei->ei_model, get_exif_value( ed, EXIF_TAG_MODEL ) );
			ei->ei_flash=get_exif_flash( ed );
			ei->ei_exposure=setstr( ei->ei_exposure, get_exif_value( ed, EXIF_TAG_EXPOSURE_TIME ) );
			ei->ei_aperture=setstr( ei->ei_aperture, get_exif_value( ed, EXIF_TAG_FNUMBER ) );
			ei->ei_rotate=get_exif_rotate( ed );
			ei->ei_date=setstr( ei->ei_date, get_exif_value( ed, EXIF_TAG_DATE_TIME ) );
			ei->ei_focal=setstr( ei->ei_focal, get_exif_value( ed, EXIF_TAG_FOCAL_LENGTH ) );
			/* If we have no info then no deal */
			if( !ei->ei_make && !ei->ei_model && !ei->ei_exposure && !ei->ei_aperture && !ei->ei_date && !ei->ei_focal )
			{
				gfx_exif_free( ei, TRUE );
				ei=NULL;
			}
		}
		exif_data_unref (ed);
	}
#endif
	return ei;
}

void gfx_exif_free( ExifInfo *ei, BOOL free_ei )
{
	if( ei )
	{
		if( ei->ei_make ) free( ei->ei_make );
		if( ei->ei_model ) free( ei->ei_model );
		if( ei->ei_exposure ) free( ei->ei_exposure );
		if( ei->ei_aperture ) free( ei->ei_aperture );
		if( ei->ei_date ) free( ei->ei_date );
		if( ei->ei_focal ) free( ei->ei_focal );
		if( free_ei ) free( ei );
		else memset( ei, '\0', sizeof( ExifInfo ) );
	}
}

