#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <bcm_host.h>
#include <VG/openvg.h>

#include "v8_helpers.h"
#include "fbcp.h"

using namespace node;
using namespace v8;

DISPMANX_DISPLAY_HANDLE_T display;
DISPMANX_MODEINFO_T display_info;
DISPMANX_RESOURCE_HANDLE_T screen_resource;
char *fbp = 0;
VC_RECT_T target_rect;
int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
uint32_t stride;

bool broken = false;

void init(Handle<Object> target) {
  NODE_SET_METHOD(target, "startup"       , fbcp::Startup);
  NODE_SET_METHOD(target, "shutdown"      , fbcp::Shutdown);
  NODE_SET_METHOD(target, "fullScreenCopy", fbcp::FullScreenCopy);
  NODE_SET_METHOD(target, "regionCopy"    , fbcp::RegionCopy);
}
NODE_MODULE(fbcp, init)

V8_METHOD(fbcp::Startup) {
  HandleScope scope;

  uint32_t image_prt;
  int ret;

  Local<Object> options = args[0].As<Object>();
  bool debug = options->Get(String::NewSymbol("debug"))->BooleanValue();

  bcm_host_init();

  display = vc_dispmanx_display_open(0);
  if (!display) {
    fprintf(stderr, "Unable to open primary display\n");
    broken = true;
    V8_RETURN(Integer::New(-1));
  }

  ret = vc_dispmanx_display_get_info(display, &display_info);
  if (ret) {
    fprintf(stderr, "Unable to get primary display information\n");
    broken = true;
    V8_RETURN(Integer::New(-1));
  }
  if (debug) {
    fprintf(stdout, "Primary display is %d x %d\n", display_info.width, display_info.height);
  }

  options->Set(String::NewSymbol("primaryWidth" ), Integer::New(display_info.width));
  options->Set(String::NewSymbol("primaryHeight"), Integer::New(display_info.height));


  fbfd = open("/dev/fb1", O_RDWR);
  if (!fbfd) {
    fprintf(stderr, "Unable to open secondary display\n");
    broken = true;
    V8_RETURN(Integer::New(-1));
  }
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    fprintf(stderr, "Unable to get secondary display information\n");
    broken = true;
    V8_RETURN(Integer::New(-1));
  }
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    fprintf(stderr, "Unable to get secondary display information\n");
    broken = true;
    V8_RETURN(Integer::New(-1));
  }

  if (debug) {
    fprintf(stdout, "Second display is %d x %d %dbps\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
  }
  options->Set(String::NewSymbol("secondaryWidth" ), Integer::New(vinfo.xres));
  options->Set(String::NewSymbol("secondaryHeight"), Integer::New(vinfo.yres));
  options->Set(String::NewSymbol("secondaryBPP"   ), Integer::New(vinfo.bits_per_pixel));

  screen_resource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, vinfo.xres, vinfo.yres, &image_prt);
  if (!screen_resource) {
    fprintf(stderr, "Unable to create screen buffer\n");
    close(fbfd);
    vc_dispmanx_display_close(display);
    broken = true;
    V8_RETURN(Integer::New(-1));
  }

  fbp = (char *) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
  if (fbp == MAP_FAILED) {
    fprintf(stderr, "Unable to create mamory mapping\n");
    close(fbfd);
    ret = vc_dispmanx_resource_delete(screen_resource);
    vc_dispmanx_display_close(display);
    broken = true;
    V8_RETURN(Integer::New(-1));
  }

  vc_dispmanx_rect_set(&target_rect, 0, 0, vinfo.xres, vinfo.yres);

  stride = vinfo.xres * vinfo.bits_per_pixel / 8;

  V8_RETURN(Undefined());
}

V8_METHOD(fbcp::Shutdown) {
  HandleScope scope;

  if (!broken) {
    munmap(fbp, finfo.smem_len);
    close(fbfd);
    vc_dispmanx_resource_delete(screen_resource);
    vc_dispmanx_display_close(display);
  }

  V8_RETURN(Undefined());
}

V8_METHOD(fbcp::FullScreenCopy) {
  HandleScope scope;

  if (!broken) {
    vc_dispmanx_snapshot(display, screen_resource, (DISPMANX_TRANSFORM_T) 0);
    vc_dispmanx_resource_read_data(screen_resource, &target_rect, fbp, stride);
  }

  V8_RETURN(Undefined());
}

V8_METHOD(fbcp::RegionCopy) {
  HandleScope scope;

  if (!broken) {
    vgReadPixels(fbp + stride * (vinfo.yres - 1), -stride, VG_sRGB_565,
                 0, display_info.height - vinfo.yres, vinfo.xres, vinfo.yres);
  }

  V8_RETURN(Undefined());
}
