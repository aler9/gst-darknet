
#ifndef __GST_DARKNET_ELEMENT_PRINT_H__
#define __GST_DARKNET_ELEMENT_PRINT_H__

#include <gst/gst.h>
#include <gst/video/video.h>

// darknet
extern char **get_labels (char *filename);

G_BEGIN_DECLS
/* */
    GType gst_darknetprint_get_type ();

#define GST_TYPE_DARKNETPRINT (gst_darknetprint_get_type())
#define GST_DARKNETPRINT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DARKNETPRINT,GstDarknetPrint))

typedef struct
{
  GstElement element;

  /* properties */
  gchar *labels;

  GstPad *sinkpad;
  GstPad *srcpad;
  guint width;
  guint height;
  gchar **label_lines;

} GstDarknetPrint;

typedef struct
{
  GstElementClass parent_class;

} GstDarknetPrintClass;

G_END_DECLS
/* */
#endif /* __GST_DARKNET_ELEMENT_PRINT_H__ */
