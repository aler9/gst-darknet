
#include "element_infer.h"
#include "element_print.h"
#include "element_render.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "darknetinfer", GST_RANK_NONE,
          GST_TYPE_DARKNETINFER)) {
    return FALSE;
  }

  if (!gst_element_register (plugin, "darknetprint", GST_RANK_NONE,
          GST_TYPE_DARKNETPRINT)) {
    return FALSE;
  }

  if (!gst_element_register (plugin, "darknetrender", GST_RANK_NONE,
          GST_TYPE_DARKNETRENDER)) {
    return FALSE;
  }

  return TRUE;
}

#define PACKAGE "darknet"

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, darknet,
    "gst-darknet", plugin_init, "1.0", "LGPL", "gst-darknet",
    "https://github.com/aler9/gst-darknet")
