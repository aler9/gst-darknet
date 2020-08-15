
#include <cairo.h>

#include "meta_detections.h"
#include "element_render.h"

#define LINE_WIDTH      2
#define TEXT_FONT       "Arial"
#define TEXT_SIZE       13
#define TEXT_PADDING    5

#define gst_darknetrender_parent_class parent_class
G_DEFINE_TYPE (GstDarknetRender, gst_darknetrender, GST_TYPE_ELEMENT);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,format=RGB")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,format=RGB")
    );

enum
{
  PROP_0,
  PROP_LABELS,
  PROP_BOX_COLOR,
  PROP_TEXT_COLOR,
};

#define PROP_LABELS_DEFAULT     ""
#define PROP_BOX_COLOR_DEFAULT  "00FFFF"
#define PROP_TEXT_COLOR_DEFAULT "000000"

static void
gst_darknetrender_parse_html_color (const gchar * str, guint8 * r, guint8 * g,
    guint8 * b)
{
  if (strlen (str) != 6) {
    g_print ("ERR: invalid HTML color length");
    exit (1);
  }

  gchar tmp[3];
  tmp[2] = 0;

  tmp[0] = str[0];
  tmp[1] = str[1];
  *r = strtol (tmp, NULL, 16);

  tmp[0] = str[2];
  tmp[1] = str[3];
  *g = strtol (tmp, NULL, 16);

  tmp[0] = str[4];
  tmp[1] = str[5];
  *b = strtol (tmp, NULL, 16);
}

static void
gst_darknetrender_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDarknetRender *filter = GST_DARKNETRENDER (object);

  switch (prop_id) {
    case PROP_LABELS:
      g_free (filter->labels);
      filter->labels = g_value_dup_string (value);
      break;

    case PROP_BOX_COLOR:
      g_free (filter->box_color);
      filter->box_color = g_value_dup_string (value);
      gst_darknetrender_parse_html_color (filter->box_color,
          &filter->box_color_r, &filter->box_color_g, &filter->box_color_b);
      break;

    case PROP_TEXT_COLOR:
      g_free (filter->text_color);
      filter->text_color = g_value_dup_string (value);
      gst_darknetrender_parse_html_color (filter->text_color,
          &filter->text_color_r, &filter->text_color_g, &filter->text_color_b);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_darknetrender_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstDarknetRender *filter = GST_DARKNETRENDER (parent);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
    GstCaps *caps;
    gst_event_parse_caps (event, &caps);

    GstVideoInfo video_info;
    gst_video_info_from_caps (&video_info, caps);
    filter->width = video_info.width;
    filter->height = video_info.height;
  }

  return gst_pad_event_default (pad, parent, event);
}

static GstFlowReturn
gst_darknetrender_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstDarknetRender *filter = GST_DARKNETRENDER (parent);

  GstDarknetMetaDetections *meta = GST_DARKNETMETADETECTIONS_GET (buf);

  GstMapInfo info;
  gst_buffer_map (buf, &info, GST_MAP_WRITE);

  // from RGB to BGRA
  guint8 *data = g_malloc (filter->width * filter->height * 4);
  for (guint i = 0, j = 0; i < (filter->width * filter->height * 3);) {
    data[j++] = info.data[i + 2];
    data[j++] = info.data[i + 1];
    data[j++] = info.data[i];
    data[j++] = 0;
    i += 3;
  }

  cairo_surface_t *surface = cairo_image_surface_create_for_data (data,
      CAIRO_FORMAT_RGB24,
      filter->width,
      filter->height,
      cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, filter->width));
  cairo_t *cairo = cairo_create (surface);

  for (guint i = 0; i < meta->detection_count; i++) {
    GstDarknetMetaDetection *det = &meta->detections[i];

    cairo_set_source_rgb (cairo, filter->box_color_r, filter->box_color_g,
        filter->box_color_b);
    cairo_set_line_width (cairo, LINE_WIDTH);

    cairo_rectangle (cairo, det->xmin, det->ymin, det->xmax - det->xmin,
        det->ymax - det->ymin);
    cairo_stroke (cairo);

    cairo_set_font_size (cairo, TEXT_SIZE);
    cairo_select_font_face (cairo, TEXT_FONT,
        CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

    gchar *text =
        g_strdup_printf ("%s %.2f%%", filter->label_lines[det->classid],
        det->probability * 100.0);

    cairo_text_extents_t extents;
    cairo_text_extents (cairo, text, &extents);

    cairo_rectangle (cairo, det->xmin,
        det->ymin - extents.height - TEXT_PADDING * 2,
        extents.width + TEXT_PADDING * 2, extents.height + TEXT_PADDING * 2);
    cairo_fill (cairo);

    cairo_set_source_rgb (cairo, filter->text_color_r, filter->text_color_g,
        filter->text_color_b);
    cairo_move_to (cairo, det->xmin + TEXT_PADDING, det->ymin - TEXT_PADDING);
    cairo_show_text (cairo, text);

    g_free (text);
  }

  cairo_destroy (cairo);
  cairo_surface_destroy (surface);

  // from BGRA to RGB
  for (guint i = 0, j = 0; i < (filter->width * filter->height * 3);) {
    info.data[i + 2] = data[j++];
    info.data[i + 1] = data[j++];
    info.data[i] = data[j++];
    j++;
    i += 3;
  }

  g_free (data);
  gst_buffer_unmap (buf, &info);

  return gst_pad_push (filter->srcpad, buf);
}

static void
gst_darknetrender_init_after_props (GstDarknetRender * filter)
{
  filter->label_lines = get_labels (filter->labels);
}

static void
gst_darknetrender_state_changed (GstElement * element, GstState oldstate,
    GstState newstate, GstState pending)
{
  GstDarknetRender *filter = GST_DARKNETRENDER (element);

  if (newstate == GST_STATE_PAUSED) {
    gst_darknetrender_init_after_props (filter);
  }
}

static void
gst_darknetrender_class_init (GstDarknetRenderClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = (GstElementClass *) klass;

  object_class->set_property = gst_darknetrender_set_property;

  g_object_class_install_property (object_class, PROP_LABELS,
      g_param_spec_string ("labels", "labels", "path to a label file",
          "", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_BOX_COLOR,
      g_param_spec_string ("box-color", "box-color", "color of the boxes",
          "", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_TEXT_COLOR,
      g_param_spec_string ("text-color", "text-color",
          "color of the text in HTML format", "",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_details_simple (element_class,
      "Plugin",
      "FIXME:Generic",
      "FIXME:Generic Template Element", "AUTHOR_NAME AUTHOR_EMAIL");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  element_class->state_changed = gst_darknetrender_state_changed;
}

static void
gst_darknetrender_init (GstDarknetRender * filter)
{
  GstElement *element = GST_ELEMENT (filter);

  g_print ("[darknetrender] init()\n");

  filter->labels = g_strdup (PROP_LABELS_DEFAULT);
  filter->box_color = g_strdup (PROP_BOX_COLOR_DEFAULT);
  filter->text_color = g_strdup (PROP_TEXT_COLOR_DEFAULT);

  gst_darknetrender_parse_html_color (filter->box_color,
      &filter->box_color_r, &filter->box_color_g, &filter->box_color_b);

  gst_darknetrender_parse_html_color (filter->text_color,
      &filter->text_color_r, &filter->text_color_g, &filter->text_color_b);

  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, gst_darknetrender_sink_event);
  gst_pad_set_chain_function (filter->sinkpad, gst_darknetrender_chain);
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (element, filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (element, filter->srcpad);
}
