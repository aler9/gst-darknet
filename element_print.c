
#include "meta_detections.h"
#include "element_print.h"

#define gst_darknetprint_parent_class parent_class
G_DEFINE_TYPE (GstDarknetPrint, gst_darknetprint, GST_TYPE_ELEMENT);

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
};

#define PROP_LABELS_DEFAULT     ""

static void
gst_darknetprint_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDarknetPrint *filter = GST_DARKNETPRINT (object);

  switch (prop_id) {
    case PROP_LABELS:
      g_free (filter->labels);
      filter->labels = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_darknetprint_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDarknetPrint *filter = GST_DARKNETPRINT (parent);

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
gst_darknetprint_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstElement *element = GST_ELEMENT (parent);
  GstDarknetPrint *filter = GST_DARKNETPRINT (parent);

  GstDarknetMetaDetections *meta = GST_DARKNETMETADETECTIONS_GET (buf);

  g_print ("[darknetprint] name=%s size=[%d %d] detections=%d\n",
      GST_ELEMENT_NAME (element), filter->width, filter->height,
      meta->detection_count);

  for (guint i = 0; i < meta->detection_count; i++) {
    GstDarknetMetaDetection *det = &meta->detections[i];
    g_print ("* prob=%.2f%% box=[%u %u %u %u] class=%s\n",
        det->probability * 100.0f,
        det->xmin, det->ymin, det->xmax, det->ymax,
        filter->label_lines[det->classid]);
  }

  return gst_pad_push (filter->srcpad, buf);
}

static void
gst_darknetprint_init_after_props (GstDarknetPrint * filter)
{
  filter->label_lines = get_labels (filter->labels);
}

static void
gst_darknetprint_state_changed (GstElement * element, GstState oldstate,
    GstState newstate, GstState pending)
{
  GstDarknetPrint *filter = GST_DARKNETPRINT (element);

  if (newstate == GST_STATE_PAUSED) {
    gst_darknetprint_init_after_props (filter);
  }
}

static void
gst_darknetprint_class_init (GstDarknetPrintClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = (GstElementClass *) klass;

  object_class->set_property = gst_darknetprint_set_property;

  g_object_class_install_property (object_class, PROP_LABELS,
      g_param_spec_string ("labels", "labels", "Path of the labels file",
          "", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_details_simple (element_class,
      "Plugin",
      "FIXME:Generic",
      "FIXME:Generic Template Element", "AUTHOR_NAME AUTHOR_EMAIL");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  element_class->state_changed = gst_darknetprint_state_changed;
}

static void
gst_darknetprint_init (GstDarknetPrint * filter)
{
  GstElement *element = GST_ELEMENT (filter);

  g_print ("[darknetprint] init()\n");

  filter->labels = g_strdup (PROP_LABELS_DEFAULT);

  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad, gst_darknetprint_sink_event);
  gst_pad_set_chain_function (filter->sinkpad, gst_darknetprint_chain);
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (element, filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (element, filter->srcpad);
}
