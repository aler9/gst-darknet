
#ifndef __GST_DARKNET_ELEMENT_RENDER_H__
#define __GST_DARKNET_ELEMENT_RENDER_H__

#include <gst/gst.h>
#include <gst/video/video.h>

// darknet
extern char **get_labels (char *filename);

G_BEGIN_DECLS
/* */
    GType gst_darknetrender_get_type ();

#define GST_TYPE_DARKNETRENDER (gst_darknetrender_get_type())
#define GST_DARKNETRENDER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DARKNETRENDER,GstDarknetRender))

typedef struct
{
} GstDarknetRenderPad;

typedef struct
{
  GstElement element;

  /* properties */
  gchar *labels;
  gchar *box_color;
  gchar *text_color;

  GstPad *sinkpad;
  GstPad *srcpad;
  guint width;
  guint height;
  gchar **label_lines;
  guint8 box_color_r, box_color_b, box_color_g;
  guint8 text_color_r, text_color_b, text_color_g;

} GstDarknetRender;

typedef struct
{
  GstElementClass parent_class;

} GstDarknetRenderClass;

G_END_DECLS
/* */
#endif /* __GST_DARKNET_ELEMENT_RENDER_H__ */
