
#ifndef __GST_MARK_META_DETECTIONS_H__
#define __GST_MARK_META_DETECTIONS_H__

#include <gst/gst.h>
#include <gst/gstmeta.h>

G_BEGIN_DECLS
/* */
    GType gst_darknetmetadetections_api_get_type (void);
const GstMetaInfo *gst_darknetmetadetections_get_info (void);

#define GST_DARKNETMETADETECTIONS_GET(buf) ((GstDarknetMetaDetections *)gst_buffer_get_meta(buf,gst_darknetmetadetections_api_get_type()))
#define GST_DARKNETMETADETECTIONS_ADD(buf) ((GstDarknetMetaDetections *)gst_buffer_add_meta(buf,gst_darknetmetadetections_get_info(),(gpointer)NULL))

#define MAX_DETECTIONS 1024

typedef struct
{
  guint classid;
  gfloat probability;
  guint xmin, ymin, xmax, ymax;

} GstDarknetMetaDetection;

typedef struct
{
  GstMeta meta;

  GstDarknetMetaDetection detections[MAX_DETECTIONS];
  guint detection_count;

} GstDarknetMetaDetections;

G_END_DECLS
/* */
#endif /* __GST_MARK_META_DETECTIONS_H__ */
