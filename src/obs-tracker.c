#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-aruco-tracker", "en-US")

extern struct obs_source_info aruco_tracker_filter;

bool obs_module_load(void) {
    obs_register_source(&aruco_tracker_filter);
    return true;
}