#!/bin/sh -e

rm -rf /hostlibs2/*
ln -s /hostlibs/libcuda* /hostlibs/libnvidia* /hostlibs2/
export LD_LIBRARY_PATH=/hostlibs2

MODEL=yolov4-sam-mish

sed -i 's/^width=.*$/width=512/' /${MODEL}.cfg
sed -i 's/^height=.*$/height=512/' /${MODEL}.cfg

# gst-launch-1.0 \
# darknetinfer name=d config=/${MODEL}.cfg weights=/${MODEL}.weights print-fps-period=2 \
# multifilesrc location=/dog.jpg caps="image/jpeg,framerate=20/1" ! jpegdec ! videoconvert ! video/x-raw,format=RGB \
# ! d.sink_0 d.src_0 ! darknetrender labels=/coco.names ! videoconvert ! xvimagesink sync=1 \
# multifilesrc location=/giraffe.jpg caps="image/jpeg,framerate=20/1" ! jpegdec ! videoconvert ! video/x-raw,format=RGB \
# ! d.sink_1 d.src_1 ! darknetrender labels=/coco.names ! videoconvert ! xvimagesink sync=1

# gst-launch-1.0 \
# filesrc location=/test.mp4 ! decodebin \
# ! videoconvert \
# ! darknetinfer name=d config=/${MODEL}.cfg weights=/${MODEL}.weights print-fps-period=2 \
# ! darknetrender labels=/coco.names \
# ! videoconvert \
# ! xvimagesink sync=1

gst-launch-1.0 \
filesrc location=/test.mp4 ! decodebin \
! videoconvert \
! darknetinfer name=d config=/${MODEL}.cfg weights=/${MODEL}.weights print-fps-period=2 \
! darknetrender labels=/coco.names \
! videoconvert \
! x264enc bitrate=1000 \
! mp4mux \
! filesink location=/orig/out.mp4

# /export
