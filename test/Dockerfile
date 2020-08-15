###################################
FROM ubuntu:18.04 AS build

RUN apt update && apt install -y --no-install-recommends \
    ca-certificates \
    curl \
    make \
    pkg-config \
    g++ \
    libgstreamer-plugins-base1.0-dev \
    libcairo2-dev \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

RUN export DEBIAN_FRONTEND=noninteractive \
    && curl -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-cudart-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-cudart-dev-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-curand-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-curand-dev-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-driver-dev-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-license-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-misc-headers-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-nvcc-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/libcublas-dev_10.2.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/libcublas10_10.2.2.89-1_amd64.deb \
    && dpkg -i /*deb && rm -rf /*.deb \
    && ln -s cuda-10.2 /usr/local/cuda

WORKDIR /s
COPY darknet ./darknet
COPY Makefile ./
RUN make -j$(nproc) build/libdarknet.a

COPY *.c *.h ./
RUN make -j$(nproc) \
    && make install

WORKDIR /e
COPY examples/export.c ./
RUN gcc \
    -Ofast \
    -Werror \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -I/s \
    export.c \
    -o export \
    $(pkg-config --cflags --libs gstreamer-app-1.0)

###################################
FROM ubuntu:18.04

RUN apt update && apt install -y --no-install-recommends \
    ca-certificates \
    curl \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install gdown

RUN export DEBIAN_FRONTEND=noninteractive \
    && curl -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-cudart-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-curand-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-license-10-2_10.2.89-1_amd64.deb \
    -O http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/libcublas10_10.2.2.89-1_amd64.deb \
    && dpkg -i /*deb && rm -rf /*.deb

RUN curl -O -L https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov3.cfg \
    -O -L https://pjreddie.com/media/files/yolov3.weights

RUN curl -O -L https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov4.cfg \
    -O -L https://github.com/AlexeyAB/darknet/releases/download/darknet_yolo_v3_optimal/yolov4.weights

RUN curl -O -L https://raw.githubusercontent.com/AlexeyAB/darknet/master/cfg/yolov4-tiny.cfg \
    -O -L https://github.com/AlexeyAB/darknet/releases/download/darknet_yolo_v4_pre/yolov4-tiny.weights

RUN gdown -O yolov4-sam-mish.cfg https://drive.google.com/uc?id=1V726wzWW1iBDcJ7uP8c0n_Y4M98lIg-C \
    && gdown -O yolov4-sam-mish.weights https://drive.google.com/uc?id=1wK66ga9YgtjGNSm9fpouJn2GaDxa7SfT

COPY --from=build /s/darknet/data/coco.names /
COPY --from=build /s/darknet/data/dog.jpg /
COPY --from=build /s/darknet/data/giraffe.jpg /

COPY test/test.mp4 /

COPY --from=build /usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstdarknet.so /usr/lib/x86_64-linux-gnu/gstreamer-1.0/
COPY --from=build /e/export /

COPY test/start.sh /
RUN chmod +x /start.sh

ENTRYPOINT [ "/start.sh" ]
