<?php

$album_file="thumbnails/gfxindex.xml";
if( !file_exists( $album_file ) ): ?>
<HTML>
 <HEAD>
  <TITLE>GFXindex - error</TITLE>
 </HEAD>
 <BODY>
  <H1>Error</H1>
  No album information in this directory.
 </BODY>
</HTML>
<?php 
exit;
endif;

$stack=Array();
$sp=0;
$buf="";
$thumblist=Array();
$done=0;
$collectall=0;
$pn=NULL;
global $stack, $sp, $buf, $thumblist, $done, $collectall, $pn;

define( E_NONE, 0 );
define( E_THUMBDATA, 1 );
define( E_T_TITLE, 2 );
define( E_T_CAPTION, 3 );
define( E_PICTURENODE, 4 );
define( E_P_TITLE, 5 );
define( E_P_CAPTION, 6 );
define( E_ORIGINAL, 7 );
define( E_O_PICTURE, 8 );
define( E_THUMBNAIL, 9 );
define( E_T_PICTURE, 10 );
define( E_BIGS, 11 );
define( E_B_PICTURE, 12 );
define( E_UNKNOWN, 13 );

define(	XML_THUMBDATA, "thumbdata" );
define(	XML_PICTURE, "picture" );
define(	XML_PICTURENODE, "picture" );
define(	XML_THUMBNAIL, "thumbnail" );
define(	XML_ORIGINAL, "original" );
define(	XML_BIGS, "bigs" );
define(	XML_CAPTION, "caption" );
define(	XML_TITLE, "title" );
define(	XML_PATH, "path" );
define(	XML_WIDTH, "width" );
define(	XML_HEIGHT, "height" );

class Picture
{
	var $p_path;
	var $p_width;
	var $p_height;
}

class PictureNode
{
	var $pn_skip;
	var $pn_rotate;
	var $pn_title;
	var $pn_caption;
	var $pn_original;
	var $pn_pictures;
	var $pn_thumbnail;
	var $pn_dir;
}



function data( $parser, $s )
{
	global $stack, $sp, $buf, $thumblist, $done, $collectall, $pn;
	switch( $stack[$sp] )
	{
		case E_T_TITLE:
		case E_T_CAPTION:
		case E_P_TITLE:
		case E_P_CAPTION:
			if( strlen( $buf ) && substr( $buf, 0, 1 )!=' ' ) $buf.="\n";
			$buf.=$s;
			break;
		default:
			if( $collectall )
			{
				$buf.=$s;
			}
			break;
	}
}

function start_element( $parser, $name, $atts )
{
	global $stack, $sp, $buf, $thumblist, $done, $collectall, $pn;
	if( $sp==40 )
	{
		echo "*** Stack overflow (nothing I can't handle)\n";
		return;
	}

	if( $collectall )
	{
		$buf.="<$name";
		foreach( $atts as $key=>$val )
		{
			$buf.=" $key=\"$val\"";
		}
		$buf.=">";
		$stack[++$sp]=E_UNKNOWN; // PUSH
	}
	else
	{
		switch( $stack[$sp] )
		{
			case E_THUMBDATA:
				if( $name==XML_TITLE )
				{
					$stack[++$sp]=E_T_TITLE;
					$buf="";
					$collectall=TRUE;
				}
				else if( $name==XML_CAPTION )
				{
					$stack[++$sp]=E_T_CAPTION;
					$buf="";
					$collectall=TRUE;
				}
				else if( $name==XML_PICTURENODE )
				{
					$stack[++$sp]=E_PICTURENODE;
					$pn=new PictureNode();
				}
				else $stack[++$sp]=E_UNKNOWN;
				break;
			case E_PICTURENODE:
				if( $name==XML_TITLE )
				{
					$stack[++$sp]=E_P_TITLE;
					$buf="";
					$collectall=TRUE;
				}
				else if( $name==XML_CAPTION )
				{
					$stack[++$sp]=E_P_CAPTION;
					$buf="";
					$collectall=TRUE;
				}
				else if( $name==XML_ORIGINAL ) $stack[++$sp]=E_ORIGINAL;
				else if( $name==XML_THUMBNAIL ) $stack[++$sp]=E_THUMBNAIL;
				else if( $name==XML_BIGS ) $stack[++$sp]=E_BIGS;
				else $stack[++$sp]=E_UNKNOWN;
				break;
			case E_ORIGINAL:
				if( $name==XML_PICTURE )
				{
					$pn->original=new Picture();
					while( $atts as $key=>$val )
					{
						if( $key==XML_PATH ) 
					}
				}
				else $stack[++$sp]=E_UNKNOWN;
		}
	}
}

function end_element( $parser, $name )
{
	global $stack, $sp, $buf, $thumblist, $done, $collectall, $pn;
}

$sp=0;
if( ( $fp=fopen( $album_file, "r" ) ) )
{
	if( ( $parser=xml_parser_create() ) )
	{
		xml_set_element_handler( $parser, "start_element", "end_element" );
		xml_set_character_data_handler( $parser, "data" );
		$done=0;
		while( !$done )
		{
			if( !( $rbuf=fgets( $fp ) ) ) $done=1;
			if( !$done )
			{
				if( feof( $fp ) ) $done=1;
			}
			else break;
			$rbuf=trim( $rbuf );
			$len=strlen( $buf );
			if( !$done && !$len ) continue;
			if( !xml_parse( $parser, $rbuf, $done ) )
			{
				    die(sprintf("XML error: %s at line %d",
                    xml_error_string(xml_get_error_code($parser)),
                    xml_get_current_line_number($parser)));
			}
		}
		xml_parser_free( $parser );
	}
	fclose( $fp );
}

?>
