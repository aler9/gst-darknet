
UBUNTU_BASE_IMAGE := ubuntu:18.04

.PHONY: $(shell ls)

help:
	@echo "usage: make [action]"
	@echo ""
	@echo "available actions:"
	@echo ""
	@echo "  indent"
	@echo "  test"
	@echo ""

indent:
	# https://gstreamer.freedesktop.org/documentation/frequently-asked-questions/developing.html?gi-language=c#what-is-the-coding-style-for-gstreamer-code
	docker run --rm -it -v $(PWD):/s $(UBUNTU_BASE_IMAGE) sh -c \
	"apt update && apt install -y indent wget \
	&& wget -O /gst-indent https://raw.githubusercontent.com/GStreamer/common/master/gst-indent \
	&& chmod +x /gst-indent \
	&& cd /s \
	&& /gst-indent *.c *.h \
	&& rm *~ \
	&& /gst-indent examples/*.c \
	&& rm examples/*~"

test:
	git submodule update --init

	docker build . -f test/Dockerfile -t temp

	xhost +local:root
	docker run --rm -it \
	--privileged \
	--tmpfs /hostlibs2 \
	-e DISPLAY \
	-v /usr/lib/x86_64-linux-gnu:/hostlibs:ro \
	-v /tmp/.X11-unix:/tmp/.X11-unix:ro \
	-v $(PWD):/orig \
	temp
