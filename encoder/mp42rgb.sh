#!/bin/bash

function printUsage {
	echo 'Mp4 2 Rgb565'
	echo "Usage: $0 -f <fps> (optional) -i <input>.mp4 -o <output>.rgb"
	exit 2
}

while getopts 'f:i:o:?h' c
do
  case $c in
    f) FPS=$OPTARG ;;
    i) INPUT=$OPTARG  ;;
    o) OUTPUT=$OPTARG ;;
    h|?) printUsage ;;
  esac
done

if [ -z "$INPUT" ]
then
	echo 'No input file provided.'
	exit 3
fi

if [ -z "$OUTPUT" ]
then
	echo 'No output file provided.'
	exit 4
fi

if [ -z "$FPS" ]
then
	FPS=30
fi

echo "ffmpeg -i $INPUT -vf scale=400:240,transpose=1,fps=$FPS -vcodec rawvideo -pix_fmt rgb565 $OUTPUT"
ffmpeg -i $INPUT -vf scale=400:240,transpose=1,fps=$FPS -vcodec rawvideo -pix_fmt rgb565 $OUTPUT
