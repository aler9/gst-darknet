
########################################
# options

CC ?= cc
CXX ?= g++
NVCC ?= nvcc
AR ?= ar

BUILD_DIR ?= build
INSTALL_DIR ?= /usr/lib/$(shell gcc -dumpmachine)/gstreamer-1.0
C_OPTIMIZE_FLAGS ?= -Ofast
NV_OPTIMIZE_FLAGS ?= \
	-O3 \
	-gencode arch=compute_50,code=sm_50 \
	-gencode arch=compute_52,code=sm_52 \
	-gencode arch=compute_60,code=sm_60 \
	-gencode arch=compute_61,code=compute_61

CUDA_DIR ?= /usr/local/cuda

########################################
# dependency check

ifeq ($(shell ls darknet),)
    $(info $(shell git submodule update --init))
endif

ifeq ($(shell which pkg-config),)
    $(error pkg-config not found)
endif

ifeq ($(shell ls $(CUDA_DIR)),)
    $(error cuda not found)
endif
CUDA_CFLAGS := -I$(CUDA_DIR)/include
CUDA_LDFLAGS := -L$(CUDA_DIR)/lib64 -L$(CUDA_DIR)/lib64/stubs -lcuda -lcudart -lcublas -lcurand
PATH := $(PATH):$(CUDA_DIR)/bin

GSTREAMER_VIDEO_CFLAGS := $(shell pkg-config --cflags gstreamer-video-1.0)
ifeq ($(GSTREAMER_VIDEO_CFLAGS),)
    $(error gstreamer-video-1.0 not found)
endif
GSTREAMER_VIDEO_LDFLAGS := $(shell pkg-config --libs gstreamer-video-1.0)

CAIRO_CFLAGS := $(shell pkg-config --cflags cairo)
ifeq ($(CAIRO_CFLAGS),)
    $(error cairo not found)
endif
CAIRO_LDFLAGS := $(shell pkg-config --libs cairo)

########################################
# main targets

GSTPLUGIN_NAME := libgstdarknet.so

all: $(BUILD_DIR)/$(GSTPLUGIN_NAME)

clean:
	rm -rf $(BUILD_DIR)

install:
	install -d $(INSTALL_DIR)
	install -m 644 $(BUILD_DIR)/$(GSTPLUGIN_NAME) $(INSTALL_DIR)

uninstall:
	-rm $(INSTALL_DIR)/$(GSTPLUGIN_NAME)
	-rmdir $(INSTALL_DIR)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

########################################
# darknet

DARKNET_NAME := libdarknet.a

DARKNET_OBJS := \
	$(BUILD_DIR)/darknet_image_opencv.o \
	$(BUILD_DIR)/darknet_http_stream.o \
	$(BUILD_DIR)/darknet_gemm.o \
	$(BUILD_DIR)/darknet_utils.o \
	$(BUILD_DIR)/darknet_dark_cuda.o \
	$(BUILD_DIR)/darknet_convolutional_layer.o \
	$(BUILD_DIR)/darknet_list.o \
	$(BUILD_DIR)/darknet_image.o \
	$(BUILD_DIR)/darknet_activations.o \
	$(BUILD_DIR)/darknet_im2col.o \
	$(BUILD_DIR)/darknet_col2im.o \
	$(BUILD_DIR)/darknet_blas.o \
	$(BUILD_DIR)/darknet_crop_layer.o \
	$(BUILD_DIR)/darknet_dropout_layer.o \
	$(BUILD_DIR)/darknet_maxpool_layer.o \
	$(BUILD_DIR)/darknet_softmax_layer.o \
	$(BUILD_DIR)/darknet_data.o \
	$(BUILD_DIR)/darknet_matrix.o \
	$(BUILD_DIR)/darknet_network.o \
	$(BUILD_DIR)/darknet_connected_layer.o \
	$(BUILD_DIR)/darknet_cost_layer.o \
	$(BUILD_DIR)/darknet_parser.o \
	$(BUILD_DIR)/darknet_option_list.o \
	$(BUILD_DIR)/darknet_darknet.o \
	$(BUILD_DIR)/darknet_detection_layer.o \
	$(BUILD_DIR)/darknet_captcha.o \
	$(BUILD_DIR)/darknet_route_layer.o \
	$(BUILD_DIR)/darknet_writing.o \
	$(BUILD_DIR)/darknet_box.o \
	$(BUILD_DIR)/darknet_nightmare.o \
	$(BUILD_DIR)/darknet_normalization_layer.o \
	$(BUILD_DIR)/darknet_avgpool_layer.o \
	$(BUILD_DIR)/darknet_coco.o \
	$(BUILD_DIR)/darknet_dice.o \
	$(BUILD_DIR)/darknet_yolo.o \
	$(BUILD_DIR)/darknet_detector.o \
	$(BUILD_DIR)/darknet_layer.o \
	$(BUILD_DIR)/darknet_compare.o \
	$(BUILD_DIR)/darknet_classifier.o \
	$(BUILD_DIR)/darknet_local_layer.o \
	$(BUILD_DIR)/darknet_swag.o \
	$(BUILD_DIR)/darknet_shortcut_layer.o \
	$(BUILD_DIR)/darknet_activation_layer.o \
	$(BUILD_DIR)/darknet_rnn_layer.o \
	$(BUILD_DIR)/darknet_gru_layer.o \
	$(BUILD_DIR)/darknet_rnn.o \
	$(BUILD_DIR)/darknet_rnn_vid.o \
	$(BUILD_DIR)/darknet_crnn_layer.o \
	$(BUILD_DIR)/darknet_demo.o \
	$(BUILD_DIR)/darknet_tag.o \
	$(BUILD_DIR)/darknet_cifar.o \
	$(BUILD_DIR)/darknet_go.o \
	$(BUILD_DIR)/darknet_batchnorm_layer.o \
	$(BUILD_DIR)/darknet_art.o \
	$(BUILD_DIR)/darknet_region_layer.o \
	$(BUILD_DIR)/darknet_reorg_layer.o \
	$(BUILD_DIR)/darknet_reorg_old_layer.o \
	$(BUILD_DIR)/darknet_super.o \
	$(BUILD_DIR)/darknet_voxel.o \
	$(BUILD_DIR)/darknet_tree.o \
	$(BUILD_DIR)/darknet_yolo_layer.o \
	$(BUILD_DIR)/darknet_gaussian_yolo_layer.o \
	$(BUILD_DIR)/darknet_upsample_layer.o \
	$(BUILD_DIR)/darknet_lstm_layer.o \
	$(BUILD_DIR)/darknet_conv_lstm_layer.o \
	$(BUILD_DIR)/darknet_scale_channels_layer.o \
	$(BUILD_DIR)/darknet_sam_layer.o \
	$(BUILD_DIR)/darknet_convolutional_kernels.o \
	$(BUILD_DIR)/darknet_activation_kernels.o \
	$(BUILD_DIR)/darknet_im2col_kernels.o \
	$(BUILD_DIR)/darknet_col2im_kernels.o \
	$(BUILD_DIR)/darknet_blas_kernels.o \
	$(BUILD_DIR)/darknet_crop_layer_kernels.o \
	$(BUILD_DIR)/darknet_dropout_layer_kernels.o \
	$(BUILD_DIR)/darknet_maxpool_layer_kernels.o \
	$(BUILD_DIR)/darknet_network_kernels.o \
	$(BUILD_DIR)/darknet_avgpool_layer_kernels.o

DARKNET_CFLAGS := \
	$(C_OPTIMIZE_FLAGS) \
	-fPIC \
	-DGPU \
	-Idarknet/include \
	-Idarknet/3rdparty/stb/include \
	-Wno-unused-result \
	-Wno-unknown-pragmas \
	-Wno-unused-variable \
	-Wno-format-security \
	-Wno-incompatible-pointer-types \
	-Wno-format \
	-Wno-stringop-overflow \
	-Wno-alloc-size-larger-than \
	$(CUDA_CFLAGS)

DARKNET_CXXFLAGS := \
	$(C_OPTIMIZE_FLAGS) \
	-fPIC \
	-DGPU \
	-Idarknet/include \
	-Idarknet/3rdparty/stb/include \
	$(CUDA_CFLAGS)

DARKNET_NVCCFLAGS := $(NV_OPTIMIZE_FLAGS)

$(BUILD_DIR)/darknet_%.o: darknet/src/%.c | $(BUILD_DIR) $(wildcard darknet/src/*.h) darknet/include/darknet.h
	$(CC) $(DARKNET_CFLAGS) -c $< -o $@

$(BUILD_DIR)/darknet_%.o: darknet/src/%.cpp | $(BUILD_DIR) $(wildcard darknet/src/*.h) darknet/include/darknet.h
	$(CXX) $(DARKNET_CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/darknet_%.o: darknet/src/%.cu | $(BUILD_DIR) $(wildcard darknet/src/*.h) darknet/include/darknet.h
	$(NVCC) $(DARKNET_NVCCFLAGS) --compiler-options "$(DARKNET_CXXFLAGS)" -c $< -o $@

$(BUILD_DIR)/$(DARKNET_NAME): $(DARKNET_OBJS)
	$(AR) rcs $@ $^

########################################
# gstplugin

GSTPLUGIN_OBJS := \
	$(BUILD_DIR)/gstplugin_element_infer.o \
	$(BUILD_DIR)/gstplugin_element_print.o \
	$(BUILD_DIR)/gstplugin_element_render.o \
	$(BUILD_DIR)/gstplugin_meta_detections.o \
	$(BUILD_DIR)/gstplugin_plugin.o

GSTPLUGIN_CFLAGS := \
 	$(C_OPTIMIZE_FLAGS) \
	-fPIC \
	-Werror \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	$(CUDA_CFLAGS) \
	$(GSTREAMER_VIDEO_CFLAGS) \
	$(CAIRO_CFLAGS)

GSTPLUGIN_LDFLAGS := \
	-shared \
	-Wl,-soname,$(GSTPLUGIN_NAME) \
	-Wl,--no-undefined \
	$(BUILD_DIR)/$(DARKNET_NAME) \
	-lm \
	-lstdc++ \
	$(CUDA_LDFLAGS) \
	$(GSTREAMER_VIDEO_LDFLAGS) \
	$(CAIRO_LDFLAGS)

$(BUILD_DIR)/gstplugin_%.o: %.c | $(BUILD_DIR) $(wildcard *.h) darknet/include/darknet.h
	$(CC) $(GSTPLUGIN_CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(GSTPLUGIN_NAME): $(GSTPLUGIN_OBJS) | $(BUILD_DIR)/$(DARKNET_NAME)
	$(CC) $(GSTPLUGIN_CFLAGS) $^ -o $@ $(GSTPLUGIN_LDFLAGS)
