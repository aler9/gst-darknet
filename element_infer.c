
#include "meta_detections.h"
#include "element_infer.h"

#define gst_darknetinfer_parent_class parent_class
G_DEFINE_TYPE (GstDarknetInfer, gst_darknetinfer, GST_TYPE_ELEMENT);

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("video/x-raw,format=RGB")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("video/x-raw,format=RGB")
    );

enum
{
  PROP_0,
  PROP_CONFIG,
  PROP_WEIGHTS,
  PROP_PROBABILITY_THRESHOLD,
  PROP_NMS_THRESHOLD,
  PROP_PRINT_FPS,
  PROP_PRINT_FPS_PERIOD,
};

#define PROP_CONFIG_DEFAULT                 ""
#define PROP_WEIGHTS_DEFAULT                ""
#define PROP_PROBABILITY_THRESHOLD_DEFAULT  0.7
#define PROP_NMS_THRESHOLD_DEFAULT          0.45
#define PROP_PRINT_FPS_DEFAULT              TRUE
#define PROP_PRINT_FPS_PERIOD_DEFAULT       5

static void
gst_darknetinfer_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDarknetInfer *filter = GST_DARKNETINFER (object);

  switch (prop_id) {
    case PROP_CONFIG:
      g_free (filter->config);
      filter->config = g_value_dup_string (value);
      break;

    case PROP_WEIGHTS:
      g_free (filter->weights);
      filter->weights = g_value_dup_string (value);
      break;

    case PROP_PROBABILITY_THRESHOLD:
      filter->probability_threshold = g_value_get_float (value);
      break;

    case PROP_NMS_THRESHOLD:
      filter->nms_threshold = g_value_get_float (value);
      break;

    case PROP_PRINT_FPS:
      filter->print_fps = g_value_get_boolean (value);
      break;

    case PROP_PRINT_FPS_PERIOD:
      filter->print_fps_period = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_darknetinfer_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDarknetInferPad *filterpad = gst_pad_get_element_private (pad);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CAPS) {
    GstCaps *caps;
    gst_event_parse_caps (event, &caps);

    GstVideoInfo video_info;
    gst_video_info_from_caps (&video_info, caps);
    filterpad->width = video_info.width;
    filterpad->height = video_info.height;
    filterpad->image = make_image (filterpad->width, filterpad->height, 3);

    gst_pad_set_caps (filterpad->srcpad, caps);
    return TRUE;
  }

  return gst_pad_event_default (pad, parent, event);
}

// modified version of letterbox_image_into
static void
gst_darknetinfer_letterbox (image im, int w, int h, image boxed, int *posx,
    int *posy, int *wo, int *ho)
{
  int new_w = im.w;
  int new_h = im.h;
  if (((float) w / im.w) < ((float) h / im.h)) {
    new_w = w;
    new_h = (im.h * w) / im.w;
  } else {
    new_h = h;
    new_w = (im.w * h) / im.h;
  }
  image resized = resize_image (im, new_w, new_h);
  *wo = new_w;
  *ho = new_h;
  *posx = (w - new_w) / 2;
  *posy = (h - new_h) / 2;
  embed_image (resized, boxed, *posx, *posy);
  free_image (resized);
}

static GstFlowReturn
gst_darknetinfer_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstDarknetInfer *filter = GST_DARKNETINFER (parent);
  GstDarknetInferPad *filterpad = gst_pad_get_element_private (pad);

  // convert frame from INT8 to FP32
  GstMapInfo info;
  gst_buffer_map (buf, &info, GST_MAP_READ);
  for (guint k = 0; k < 3; k++) {
    for (guint j = 0; j < filterpad->height; j++) {
      for (guint i = 0; i < filterpad->width; i++) {
        int dst_index =
            i + filterpad->width * j + filterpad->width * filterpad->height * k;
        int src_index = k + 3 * i + 3 * filterpad->width * j;
        filterpad->image.data[dst_index] =
            ((float) info.data[src_index]) / 255.0;
      }
    }
  }
  gst_buffer_unmap (buf, &info);

  // scale to network size
  int lb_px, lb_py, lb_w, lb_h;
  gst_darknetinfer_letterbox (filterpad->image, filter->net->w, filter->net->h,
      filterpad->image_scaled, &lb_px, &lb_py, &lb_w, &lb_h);

  g_mutex_lock (&filter->mutex);

  // predict
  network_predict (*filter->net, filterpad->image_scaled.data);

  // get results
  int nboxes = 0;
  detection *dets =
      get_network_boxes (filter->net, filter->net->w, filter->net->h,
      filter->probability_threshold, 0, 0, 0,
      &nboxes, 0);
  layer *lastLayer = &filter->net->layers[filter->net->n - 1];
  do_nms_sort (dets, nboxes, lastLayer->classes, filter->nms_threshold);

  // add results to GstDarknetMetaDetections
  GstDarknetMetaDetections *meta = GST_DARKNETMETADETECTIONS_ADD (buf);
  meta->detection_count = 0;
  for (int i = 0; i < nboxes; ++i) {
    for (int j = 0; j < lastLayer->classes; ++j) {
      if (dets[i].prob[j] > filter->probability_threshold) {
        GstDarknetMetaDetection *metadet =
            &meta->detections[meta->detection_count++];

        metadet->classid = j;
        metadet->probability = dets[i].prob[j];

        metadet->xmin =
            (dets[i].bbox.x - (dets[i].bbox.w / 2.0) -
            lb_px) * filterpad->width / lb_w;
        metadet->ymin =
            (dets[i].bbox.y - (dets[i].bbox.h / 2.0) -
            lb_py) * filterpad->height / lb_h;
        metadet->xmax =
            (dets[i].bbox.x + (dets[i].bbox.w / 2.0) -
            lb_px) * filterpad->width / lb_w;
        metadet->ymax =
            (dets[i].bbox.y + (dets[i].bbox.h / 2.0) -
            lb_py) * filterpad->height / lb_h;
      }
    }
  }
  free_detections (dets, nboxes);

  if (filter->print_fps) {
    filter->frame_count++;

    GstClockTime now = gst_util_get_timestamp ();
    GstClockTimeDiff timediff = GST_CLOCK_DIFF (filter->print_fps_prev, now);

    if (timediff / GST_SECOND >= filter->print_fps_period) {
      float fps =
          (float) filter->frame_count * (float) GST_SECOND / (float) timediff;
      g_print ("[darknetinfer] frames per second: %.2f\n", fps);
      filter->frame_count = 0;
      filter->print_fps_prev = now;
    }
  }

  g_mutex_unlock (&filter->mutex);

  return gst_pad_push (filterpad->srcpad, buf);
}

static void
gst_darknetinfer_init_after_props (GstDarknetInfer * filter)
{
  g_mutex_init (&filter->mutex);
  filter->net = load_network_custom (filter->config, filter->weights, 0, 1);

  if (filter->print_fps) {
    filter->frame_count = 0;
    filter->print_fps_prev = gst_util_get_timestamp ();
  }
}

static GstPad *
gst_darknetinfer_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstDarknetInfer *filter = GST_DARKNETINFER (element);

  if (filter->net == NULL) {
    gst_darknetinfer_init_after_props (filter);
  }

  GstPad *pad = gst_pad_new_from_template (templ, name);
  GstDarknetInferPad *filterpad = g_new (GstDarknetInferPad, 1);
  filterpad->id = filter->pad_count++;
  filterpad->image_scaled = make_image (filter->net->w, filter->net->h, 3);
  gst_pad_set_element_private (pad, filterpad);
  gst_pad_set_event_function (pad, gst_darknetinfer_sink_event);
  gst_pad_set_chain_function (pad, gst_darknetinfer_chain);
  gst_element_add_pad (element, pad);

  gchar *srcname = g_strdup_printf (src_factory.name_template, filterpad->id);
  filterpad->srcpad = gst_pad_new_from_static_template (&src_factory, srcname);
  g_free (srcname);
  gst_element_add_pad (element, filterpad->srcpad);

  return pad;
}

static void
gst_darknetinfer_class_init (GstDarknetInferClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = (GstElementClass *) klass;

  object_class->set_property = gst_darknetinfer_set_property;

  g_object_class_install_property (object_class, PROP_CONFIG,
      g_param_spec_string ("config", "config", "path to a Darknet config file",
          "", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_WEIGHTS,
      g_param_spec_string ("weights", "weights",
          "path to a Darknet weights file", "",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PROBABILITY_THRESHOLD,
      g_param_spec_float ("probability-threshold", "probability-threshold",
          "probability threshold of detected objects", 0, 1.0, 0,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_NMS_THRESHOLD,
      g_param_spec_float ("nms-threshold", "nms-threshold",
          "NMS threshold of detected objects", 0, 1.0, 0,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PRINT_FPS,
      g_param_spec_boolean ("print-fps", "print-fps",
          "Periodically print FPS to stdout", FALSE,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_PRINT_FPS_PERIOD,
      g_param_spec_uint ("print-fps-period", "print-fps-period",
          "Period of FPS printing in seconds", 1, 100, 1,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_details_simple (element_class,
      "Plugin",
      "FIXME:Generic",
      "FIXME:Generic Template Element", "AUTHOR_NAME AUTHOR_EMAIL");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

  element_class->request_new_pad = gst_darknetinfer_request_new_pad;
}

static void
gst_darknetinfer_init (GstDarknetInfer * filter)
{
  g_print ("[darknetinfer] init()\n");

  filter->config = g_strdup (PROP_CONFIG_DEFAULT);
  filter->weights = g_strdup (PROP_WEIGHTS_DEFAULT);
  filter->probability_threshold = PROP_PROBABILITY_THRESHOLD_DEFAULT;
  filter->nms_threshold = PROP_NMS_THRESHOLD_DEFAULT;
  filter->print_fps = PROP_PRINT_FPS_DEFAULT;
  filter->print_fps_period = PROP_PRINT_FPS_PERIOD_DEFAULT;

  filter->pad_count = 0;
}
