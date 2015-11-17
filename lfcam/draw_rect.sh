#!/bin/bash
# Draw transparent rectangles over images
# $1 = x coordinate of the top left corner
# $2 = y coordinate of the top left corner
# $3 = x coordinate of the bottom right corner
# $4 = y coordinate of the bottom right corner
# $5 = input directory with files to convert
# $6 = output directory to store converted files
#
# ./draw_rect.sh 10 50 200 300 ./pngs  ./output
#
# convert input.jpg -strokewidth 0 -fill "rgba( 255, 215, 0 , 0.5 )" -draw "rectangle 66,50 200,150 " output.jpg

echo "This script doesn't work for file names with space characters"

for f in $5/*
do
    echo "Processing $f"
    fbase=$(basename "$f")
    echo $fbase
    fbase_no_ext=${fbase%.*}
    echo $fbase_no_ext
    output=$6/$fbase
    echo $output

    # Draw rectangle and annotate image
    convert $f \
        -strokewidth 0 -fill "rgba( 255, 215, 0 , 0.5 )" \
        -draw "rectangle $1,$2 $3,$4" \
        -stroke Black -strokewidth 1 \
        -background Khaki  label:"Leaf Alden LLC, $fbase_no_ext" \
        -gravity West -append \
        $output
done

# Create animated gif from images
fbase_no_ext_no_num=$(echo $fbase_no_ext | sed -e 's/[0-9]*$//g')
convert -delay 20 -loop 0 $5/* $6/animation.gif
