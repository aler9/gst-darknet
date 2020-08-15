
#include "meta_detections.h"

GType
gst_darknetmetadetections_api_get_type (void)
{
  static GType type;

  if (g_once_init_enter (&type)) {
    const gchar *tags[] = { NULL };
    GType _type =
        gst_meta_api_type_register ("GstDarknetMetaDetectionsAPI", tags);
    g_once_init_leave (&type, _type);
  }

  return type;
}

static gboolean
gst_darknetmetadetections_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer)
{
  return TRUE;
}

static gboolean
gst_darknetmetadetections_transform (GstBuffer * dest_buf, GstMeta * src_meta,
    GstBuffer * src_buf, GQuark type, gpointer data)
{
  GstDarknetMetaDetections *src_dets = (GstDarknetMetaDetections *) src_meta;
  GstDarknetMetaDetections *dest_dets =
      GST_DARKNETMETADETECTIONS_ADD (dest_buf);

  dest_dets->detection_count = src_dets->detection_count;
  for (guint i = 0; i < dest_dets->detection_count; i++) {
    dest_dets->detections[i] = src_dets->detections[i];
  }

  return TRUE;
}

static void
gst_darknetmetadetections_free (GstMeta * meta, GstBuffer * buffer)
{
}

const GstMetaInfo *
gst_darknetmetadetections_get_info (void)
{
  static const GstMetaInfo *meta_info = NULL;

  if (g_once_init_enter (&meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (gst_darknetmetadetections_api_get_type (),
        "GstDarknetMetaDetections",
        sizeof (GstDarknetMetaDetections),
        gst_darknetmetadetections_init,
        gst_darknetmetadetections_free,
        gst_darknetmetadetections_transform);
    g_once_init_leave (&meta_info, meta);
  }

  return meta_info;
}
