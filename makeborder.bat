ffmpeg -vcodec png -i "gbborder.png" -vcodec rawvideo -f rawvideo -pix_fmt rgb565 "gbborder.raw"
bin2h -cz gbborder<"gbborder.raw" > gbborder.h
