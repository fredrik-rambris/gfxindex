<?php
/*
 * GFXIndex (c) 1999-2000 Fredrik Rambris <fredrik.rambris@amiga.nu>. All rights reserved.
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
	if( !isset( $bgcolor ) ) $bgcolor="e0e0e0"; // Number of rows per page
	$TITLE="MyPictures";
	/*********************************************************/

	$VERSION="1.0";

	/* Get document part of URI */
	unset( $temp );
	$temp=explode( "?", $REQUEST_URI );
	$REQUEST_DOCUMENT=$temp[0];
	unset( $temp );

	if( !isset( $page ) ) $page=0;

	if( !file_exists( ".thumbnails/files.db" ) )
	{
		echo "<HTML><BODY>There exist no images this directory</BODY></HTML>";
		exit;
	}

	$arr=file( ".thumbnails/files.db" );
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
			echo "  <META NAME=\"generator\" CONTENT=\"GFXIndex v$VERSION (PHP) by Fredrik Rambris (fredrik.rambris@amiga.nu)\">\n";
			echo " </HEAD>\n";
			echo " <BODY BGCOLOR=\"#$bgcolor\">\n";
			echo "  <DIV ALIGN=\"center\">\n";

			if( $numpics>1 ) echo "   [ ";
			if( $show>0 ) echo "<A HREF=\"$REQUEST_DOCUMENT?show=" . ( $show-1 ) . "\">Previous</A> | ";
			$page=intval( $show/$ppp );
			if( $page==0 ) $index="";
			else $index="?page=$page";
			echo "<A HREF=\"$REQUEST_DOCUMENT$index\">Index</A> ";
			if( $show<($numpics-1) ) echo "| <A HREF=\"$REQUEST_DOCUMENT?show=" . ( $show+1 ) . "\">Next</A> ";
			if( $numpics>1 ) echo "]<P>\n";

			echo "   <IMG SRC=\"$td[0]\" WIDTH=\"$td[1]\" HEIGHT=\"$td[2]\">\n";
			echo "   <HR>\n";
			echo "   <FONT SIZE=\"-1\">Created using GFXIndex v$VERSION (PHP) by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A></FONT>\n";
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
	echo "  <META NAME=\"generator\" CONTENT=\"GFXIndex v$VERSION (PHP) by Fredrik Rambris (fredrik.rambris@amiga.nu)\">\n";
	echo " </HEAD>\n";
	echo " <BODY BGCOLOR=\"#$bgcolor\">\n";
	echo "  <FONT FACE=\"Helvetica,Arial,Swiss\">\n";
	echo "   <DIV ALIGN=\"center\">\n";

	if( $numpages>1 )
	{
		echo "    <FONT SIZE=\"-1\">[ ";
		if( $page>0 ) echo "<A HREF=\"$REQUEST_DOCUMENT" . (($page-1)?"?page=" . ( $page-1 ):"") . "\">Previous</A> | ";
		for( $count=0 ; $count<$numpages ; $count++ )
		{
			if( $count!=$page ) echo "<A HREF=\"$REQUEST_DOCUMENT?page=$count\">";
			else echo "<B>";
			echo ($count+1);
			if( $count!=$page ) echo "</A>";
			else echo "</B>";
			if( $count<($numpages-1) || $page<($numpages-1) ) echo " |";
			echo " ";
		}
		if( $page<($numpages-1) ) echo "<A HREF=\"$REQUEST_DOCUMENT?page=" . ( $page+1 ) . "\">Next</A> ";
		echo "]</FONT><BR>\n";
	}

	echo "    <TABLE BORDER=\"0\" CELLPADDING=\"2\" CELLSPACING=\"0\">\n";
	for( $picnum=$PicStart ; $picnum<$PicStop ; $picnum++ )
	{
		$td=$Thumbs[$picnum];
		if( $xcount==0 ) echo "     <TR>\n";
		echo "      <TD VALIGN=\"middle\" ALIGN=\"center\">";
		echo "<A HREF=\"$REQUEST_DOCUMENT?show=$picnum\">";
		echo "<IMG SRC=\"$td[3]\" WIDTH=\"$td[4]\" HEIGHT=\"$td[5]\" HSPACE=\"2\" VSPACE=\"1\" BORDER=\"0\"><BR>";
		echo "<FONT SIZE=\"-1\" FACE=\"Helvetica,Arial,Swiss\">$td[0]</FONT>";
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
	echo "    <FONT SIZE=\"-1\">Created using GFXIndex v$VERSION (PHP) by <A HREF=\"mailto:fredrik.rambris@amiga.nu\">Fredrik Rambris</A></FONT>\n";
	echo "   </DIV>\n";
	echo "  </FONT>\n";
	echo " </BODY>\n";
	echo "</HTML>\n";
?>