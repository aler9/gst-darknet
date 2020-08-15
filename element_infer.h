
#ifndef __GST_DARKNET_ELEMENT_INFER_H__
#define __GST_DARKNET_ELEMENT_INFER_H__

#include <gst/gst.h>
#include <gst/video/video.h>

// darknet
#define GPU
#include "darknet/include/darknet.h"
void embed_image (image source, image dest, int dx, int dy);

G_BEGIN_DECLS
/* */
    GType gst_darknetinfer_get_type ();

#define GST_TYPE_DARKNETINFER (gst_darknetinfer_get_type())
#define GST_DARKNETINFER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DARKNETINFER,GstDarknetInfer))

typedef struct
{
  guint id;
  guint width;
  guint height;
  image image;
  image image_scaled;
  GstPad *srcpad;

} GstDarknetInferPad;

typedef struct
{
  GstElement element;

  /* properties */
  gchar *config;
  gchar *weights;
  float probability_threshold;
  float nms_threshold;
  gboolean print_fps;
  guint print_fps_period;

  guint pad_count;
  GMutex mutex;
  network *net;
  guint frame_count;
  GstClockTime print_fps_prev;

} GstDarknetInfer;

typedef struct
{
  GstElementClass parent_class;

} GstDarknetInferClass;

G_END_DECLS
/* */
#endif /* __GST_DARKNET_ELEMENT_INFER_H__ */
