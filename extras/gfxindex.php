<?php
/*
 * GFXIndex (c) 1999-2002 Fredrik Rambris <fredrik@rambris.com>. All rights reserved.
 *
 * GFXIndex is a script that let you browse your images.
 *
 * This is licensed under GNU GPL.
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
 *
 */
	/***[ Configuration ]*************************************/
	if( !isset( $xstop ) ) $xstop=5; // Number of columns per page
	if( !isset( $ystop ) ) $ystop=3; // Number of rows per page
	if( !isset( $bodyargs ) ) $bodyargs='"';
	if( !isset( $thumbdir ) ) $thumbdir=".thumbnails";
	if( !isset( $style ) ) $style="$thumbdir/style.css";
	$TITLE="MyPictures";
	/*********************************************************/

	$VERSION="1.2";


if( !$frame )
{ ?>
<HTML>
 <HEAD>
  <TITLE>GFXIndex v<?php echo $VERSION; ?> by Fredrik Rambris</TITLE>
 </HEAD>
 <FRAMESET COLS="200,*">
  <FRAMESET ROWS="20%,*">
   <FRAME SRC="<?php echo $PHP_SELF; ?>?frame=dir" NAME="dir">
   <FRAME SRC="<?php echo $PHP_SELF; ?>?frame=thumbs" NAME="thumbs">
  </FRAMESET>
  <FRAME SRC="<?php echo $PHP_SELF; ?>?frame=image" NAME="image">
 </FRAMESET>
</HTML>
<?php }
else if( $frame=="dir" )
{ ?>
<HTML>
 <HEAD>
  <LINK REL="stylesheet" TYPE="text/css" HREF="<?php echo $style; ?>">
 </HEAD>
 <BODY BGCOLOR="#000000" TEXT="#ffffff" LINK="#ffffff" ALINK="#ff0000" VLINK="#ffffff">
  dir
 </BODY>
</HTML>
<?php }
else if( $frame=="thumbs" )
{ ?>
<HTML>
 <HEAD>
  <LINK REL="stylesheet" TYPE="text/css" HREF="<?php echo $style; ?>">
 </HEAD>
 <BODY BGCOLOR="#000000" TEXT="#ffffff" LINK="#ffffff" ALINK="#ff0000" VLINK="#ffffff">
  thumbs
 </BODY>
</HTML>
<?php }
else if( $frame=="image" )
{ ?>
<HTML>
 <HEAD>
  <LINK REL="stylesheet" TYPE="text/css" HREF="<?php echo $style; ?>">
 </HEAD>
 <BODY BGCOLOR="#000000" TEXT="#ffffff" LINK="#ffffff" ALINK="#ff0000" VLINK="#ffffff">
  image
 </BODY>
</HTML>
<?php }
exit;

	if( !isset( $album ) ) $album="";
	else
	{
		$lalbum=$album;
		$album="$album/";
		
	}
	$thumbdir=$album . $thumbdir;

	/* Get document part of URI */
	unset( $temp );
	$temp=explode( "?", $REQUEST_URI );
	$REQUEST_DOCUMENT=$temp[0];
	unset( $temp );

	if( !isset( $page ) ) $page=0;

	if( !file_exists( "$thumbdir/files.db" ) )
	{
		echo "<HTML><BODY $bodyargs>There exist no images this directory</BODY></HTML>";
		exit;
	}

	$arr=file( "$thumbdir/files.db" );
	for( $count=0 ; $count<count($arr) ; $count++ )
	{
		$Thumbs[]=explode(";", trim( $arr[$count] ) );
	}
	unset( $arr );

	$numpics=count( $Thumbs );
	$ppp=$xstop*$ystop; // Pictures per page;
	$PicStart=$ppp*$page;
	$PicStop=MIN( $PicStart+$ppp, $numpics );
	$numpages=intval($numpics/$ppp+0.99999);

	if( isset($show) && $show>=0 )
	{
		if( $show<$numpics )
		{
			$td=$Thumbs[$show];
			echo "<HTML>\n";
			echo " <HEAD>\n";
			echo "  <TITLE>" . ( $TITLE?"$TITLE - ":"" ) . "$td[0] - " . ( $show+1 ) . " / " . ( $numpics ) . "</TITLE>\n";
			echo "  <META NAME=\"generator\" CONTENT=\"GFXIndex v$VERSION (PHP) by Fredrik Rambris (fredrik@rambris.com)\">\n";
			if( $style ) echo "  <LINK REL=\"stylesheet\" HREF=\"$style\" TYPE=\"text/css\">\n";
			echo " </HEAD>\n";
			echo " <BODY $bodyargs>\n";
			echo "  <DIV ALIGN=\"center\">\n";

			if( $numpics>1 ) echo "   <SPAN ID=\"navbar\">[ ";
			if( $show>0 ) echo "<A HREF=\"$REQUEST_DOCUMENT?show=" . ( $show-1 ) . ($album?"&album=$lalbum":"") . "\">Previous</A> | ";
			$page=intval( $show/$ppp );
			if( $page==0 ) $index="" . ($album?"?album=$lalbum":"");
			else $index="?page=$page" . ($album?"&album=$lalbum":"");
			echo "<A HREF=\"$REQUEST_DOCUMENT$index\">Index</A> ";
			if( $show<($numpics-1) ) echo "| <A HREF=\"$REQUEST_DOCUMENT?show=" . ( $show+1 ) . ($album?"&album=$lalbum":"") . "\">Next</A> ";
			if( $numpics>1 ) echo "]</SPAN><BR>\n";

			echo "   <A HREF=\"$REQUEST_DOCUMENT$index\"><IMG SRC=\"$album$td[0]\" WIDTH=\"$td[1]\" HEIGHT=\"$td[2]\" BORDER=\"0\" ALT=\"Click to get back to index\"></A>\n";
			echo "   <HR>\n";
			echo "   <SPAN ID=\"credits\">This page was created using GFXIndex v$VERSION (PHP) by <A HREF=\"mailto:fredrik_at_rambris_dot_com\">Fredrik Rambris</A></SPAN>\n";
			echo "  </DIV>\n";
			echo " </BODY>\n";
			echo "</HTML>\n";
			exit;
		}
	}


	$xcount=0;
	$ycount=0;
	echo "<HTML>\n";
	echo " <HEAD>\n";
	echo "  <TITLE>" . ( $TITLE?"$TITLE - ":"" ) . "Page " . ( $page+1 ) . " / " . ( $numpages ) . "</TITLE>\n";
	echo "  <META NAME=\"generator\" CONTENT=\"GFXIndex v$VERSION (PHP) by Fredrik Rambris (fredrik@rambris.com)\">\n";
	if( $style ) echo "  <LINK REL=\"stylesheet\" HREF=\"$style\" TYPE=\"text/css\">\n";
	echo " </HEAD>\n";
	echo " <BODY $bodyargs>\n";
	echo "   <DIV ALIGN=\"center\">\n";

	if( $numpages>1 )
	{
		echo "    <SPAN ID=\"navbar\">[ ";
		if( $page>0 ) echo "<A HREF=\"$REQUEST_DOCUMENT?" . (($page-1)?"page=" . ( $page-1 ) . "&":"") . "album=$lalbum\">Previous</A> | ";
		for( $count=0 ; $count<$numpages ; $count++ )
		{
			if( $count!=$page ) echo "<A HREF=\"$REQUEST_DOCUMENT?page=$count&album=$lalbum\">";
			else echo "<SPAN ID=\"current\">";
			echo ($count+1);
			if( $count!=$page ) echo "</A>";
			else echo "</SPAN>";
			if( $count<($numpages-1) || $page<($numpages-1) ) echo " |";
			echo " ";
		}
		if( $page<($numpages-1) ) echo "<A HREF=\"$REQUEST_DOCUMENT?album=$lalbum&page=" . ( $page+1 ) . "\">Next</A> ";
		echo "]</SPAN><BR>\n";
	}

	echo "    <TABLE BORDER=\"0\" CELLPADDING=\"2\" CELLSPACING=\"0\">\n";
	for( $picnum=$PicStart ; $picnum<$PicStop ; $picnum++ )
	{
		$td=$Thumbs[$picnum];
		if( $xcount==0 ) echo "     <TR>\n";
		echo "      <TD VALIGN=\"middle\" ALIGN=\"center\">";
		echo "<A HREF=\"$REQUEST_DOCUMENT?show=$picnum" . ($album?"&album=$lalbum":"") . "\">";
		$ts=max( $td[4], $td[5] );
		$HS=(($ts-$td[4])/2)+2;
		$VS=(($ts-$td[5])/2)+1;
		echo "<IMG SRC=\"$album$td[3]\" WIDTH=\"$td[4]\" HEIGHT=\"$td[5]\" HSPACE=\"$HS\" VSPACE=\"$VS\" BORDER=\"0\"><BR>";
		echo "$td[0]";
		echo "</A>";
		echo "</TD>\n";
		$xcount++;
		if( $xcount>=$xstop )
		{
			echo "     </TR>\n";
			$xcount=0;
			$ycount++;
		}
	}
	if( $xcount>0 ) echo "     </TR>\n";
	echo "    </TABLE>\n";
	echo "    <HR>\n";
	echo "    <SPAN ID=\"credits\">This page was created using GFXIndex v$VERSION (PHP) by <A HREF=\"mailto:fredrik_at_rambris_dot_com\">Fredrik Rambris</A></SPAN>\n";
	echo "   </DIV>\n";
	echo " </BODY>\n";
	echo "</HTML>\n";
?>
