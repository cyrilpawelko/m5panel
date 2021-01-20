#!/bin/bash
svgdir=mdi
pngdir=png

while IFS=";" read -r svg png
do
    #echo "svg:$svg png:$png"
    svgfile=$svg.svg
    pngfile=$png.png
    echo "$svgfile -> $pngfile"
    if [ ! -f $svgdir/$svgfile ]
    then
        echo "ERROR: $svgfile not found, downloading"
        wget --no-hsts https://api.iconify.design/mdi-$svgfile -O $svgdir/$svgfile
    fi
    rsvg-convert $svgdir/$svgfile -o $pngdir/$pngfile -w 48
done < png_icons.csv