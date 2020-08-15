
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "meta_detections.h"

// called for every frame
static GstFlowReturn
on_sample (GstElement * element, gpointer data)
{
  GstSample *sample = gst_app_sink_pull_sample (GST_APP_SINK (element));
  GstBuffer *buf = gst_sample_get_buffer (sample);

  // get detections
  static const GstMetaInfo *metainfo = NULL;
  if (metainfo == NULL) {
    metainfo = gst_meta_get_info ("GstDarknetMetaDetections");
  }
  GstDarknetMetaDetections *meta =
      (GstDarknetMetaDetections *) gst_buffer_get_meta (buf,
      metainfo->api);

  // detection count is available in the detection_count variable
  g_print ("detections: %d\n", meta->detection_count);

  // detections are available in the detections variable
  for (guint i = 0; i < meta->detection_count; i++) {
    GstDarknetMetaDetection *det = &meta->detections[i];
    g_print ("* prob=%.2f%% box=[%u %u %u %u] class=%d\n",
        det->probability * 100.0f,
        det->xmin, det->ymin, det->xmax, det->ymax, det->classid);
  }

  gst_sample_unref (sample);
  return GST_FLOW_OK;
}

int
main (int argc, char *argv[])
{
  gst_init (&argc, &argv);

  // create the pipeline. edit to suit your needs. Leave appsink at the end.
  GstElement *pipeline =
      gst_parse_launch ("filesrc location=test.mp4 ! decodebin \
        ! videoconvert \
        ! darknetinfer name=d config=yolov4.cfg weights=yolov4.weights \
        ! appsink emit-signals=1", NULL);
  if (pipeline == NULL) {
    g_print ("cannot create pipeline\n");
    return -1;
  }
  // get appsink and link it to on_sample
  GstElement *appsink0 = gst_bin_get_by_name (GST_BIN (pipeline), "appsink0");
  g_signal_connect (appsink0, "new-sample", G_CALLBACK (on_sample), NULL);

  // launch the pipeline
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_print ("running\n");

  // wait for errors or EOS.
  GstBus *bus = gst_element_get_bus (pipeline);
  GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  if (msg->type == GST_MESSAGE_ERROR) {
    GError *error;
    gchar *debug;
    gst_message_parse_error (msg, &error, &debug);
    g_print ("ERR: %s\n", debug);
  }
  g_print ("exited\n");
  return 0;
}
