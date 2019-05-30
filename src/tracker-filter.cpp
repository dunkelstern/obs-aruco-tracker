#include <obs.h>
#include <obs-module.h>
#include <obs-source.h>
#include <util/platform.h>
#include <media-io/video-scaler.h>

#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <opencv2/imgcodecs.hpp>

#include "serial.h"

struct aruco_data {
    // self reference
	obs_source_t *context;

    // settings
	int aruco_id;
    bool draw_marker;
    uint32_t min_move_distance;
    const char *output_device;
    
    // aruco dict
    cv::Ptr<cv::aruco::Dictionary> dictionary;

    // colorspace conversion (using the sws scalers)
    video_scaler_t *scaler_in;
    video_scaler_t *scaler_out;
    video_scaler_t *scaler_simple;

    // last marker position
    double last_x, last_y;

    // serial port
    int baudrate;
    int serial_fd;

};

static void aruco_update(void *data, obs_data_t *settings);

static void check_markers(
    struct aruco_data *filter,
    struct obs_source_frame *frame,
    std::vector<int> ids,
    std::vector<std::vector<cv::Point2f> > corners
) {
    if (ids.size() > 0) {
        for(std::size_t i = 0; i < ids.size(); i++) {
            if (ids[i] != filter->aruco_id) {
                continue;
            }

            double x,y;
            for(std::size_t j = 0; j < 4; j++) {
                x += corners[i][j].x;
                y += corners[i][j].y;
            }
            x /= 4.0;
            y /= 4.0;

            if ((fabs(x - filter->last_x) >= filter->min_move_distance) ||
                (fabs(y - filter->last_y) >= filter->min_move_distance)) {

                blog(LOG_INFO, "Marker with id %d at %f x %f", ids[i], x, y);
                if (filter->serial_fd >= 0) {
                    serial_write(
                        filter->serial_fd,
                        frame->timestamp,
                        (uint32_t)x, (uint32_t)y,
                        frame->width, frame->height
                    );
                }
                filter->last_x = x;
                filter->last_y = y;                       
            }
        }
    }
}

static struct obs_source_frame *aruco_filter_video(void *data, struct obs_source_frame *frame) {
	struct aruco_data *filter = (struct aruco_data *)data;
    obs_source_t *parent = obs_filter_get_parent(filter->context);

    if (filter->draw_marker) {

        if (filter->scaler_in == NULL) {
            struct video_scale_info src = {
                .format = frame->format,
                .width = frame->width,
                .height = frame->height,
                .range = (frame->full_range ? VIDEO_RANGE_FULL: VIDEO_RANGE_PARTIAL),
                .colorspace = VIDEO_CS_DEFAULT
            };
            struct video_scale_info dst = {
                .format = VIDEO_FORMAT_BGRX,
                .width = frame->width,
                .height = frame->height,
                .range = VIDEO_RANGE_FULL,
                .colorspace = VIDEO_CS_DEFAULT
            };

            int ret = video_scaler_create(&filter->scaler_in, &dst, &src, VIDEO_SCALE_DEFAULT);
            blog(LOG_INFO, "ArUco: Video scaler_in init returned %d", ret);
        }

        if (filter->scaler_out == NULL) {
            struct video_scale_info src = {
                .format = VIDEO_FORMAT_BGRX,
                .width = frame->width,
                .height = frame->height,
                .range = VIDEO_RANGE_FULL,
                .colorspace = VIDEO_CS_DEFAULT
            };
            struct video_scale_info dst = {
                .format = frame->format,
                .width = frame->width,
                .height = frame->height,
                .range = (frame->full_range ? VIDEO_RANGE_FULL: VIDEO_RANGE_PARTIAL),
                .colorspace = VIDEO_CS_DEFAULT
            };

            int ret = video_scaler_create(&filter->scaler_out, &dst, &src, VIDEO_SCALE_DEFAULT);
            blog(LOG_INFO, "ArUco: Video scaler_out init returned %d", ret);
        }

        struct obs_source_frame *new_frame = obs_source_frame_create(VIDEO_FORMAT_BGRX, frame->width, frame->height);
        new_frame->timestamp = frame->timestamp;

        bool ok = video_scaler_scale(
            filter->scaler_in,
            new_frame->data, new_frame->linesize,
            frame->data, frame->linesize
        );
        if (!ok) {
            blog(LOG_ERROR, "ArUco: Video scaler_in returned fail");
        }

        cv::Mat image(new_frame->height, new_frame->width, CV_8UC4, new_frame->data[0]);
        cv::Mat bgr_image(frame->height, frame->width, CV_8UC3);
        cv::cvtColor(image, bgr_image, cv::COLOR_BGRA2BGR);

        if (image.data != NULL) {
            std::vector<int> ids;
            std::vector<std::vector<cv::Point2f> > corners;

            cv::aruco::detectMarkers(bgr_image, filter->dictionary, corners, ids);
            check_markers(filter, frame, ids, corners);
            if (ids.size() > 0) {
                cv::aruco::drawDetectedMarkers(bgr_image, corners, ids);
                cv::cvtColor(bgr_image, image, cv::COLOR_BGR2BGRA);

                bool ok = video_scaler_scale(
                    filter->scaler_out,
                    frame->data, frame->linesize,
                    new_frame->data, new_frame->linesize
                );
            }

            if (!ok) {
                blog(LOG_ERROR, "ArUco: Video scaler_out returned fail");
            }
        } else {
            blog(LOG_ERROR, "ArUco: failed to load image into openCV");
        }

        obs_source_frame_destroy(new_frame);
        return frame;
    } else {
        if (filter->scaler_simple == NULL) {
            struct video_scale_info src = {
                .format = frame->format,
                .width = frame->width,
                .height = frame->height,
                .range = (frame->full_range ? VIDEO_RANGE_FULL: VIDEO_RANGE_PARTIAL),
                .colorspace = VIDEO_CS_DEFAULT
            };
            struct video_scale_info dst = {
                .format = VIDEO_FORMAT_Y800,
                .width = frame->width,
                .height = frame->height,
                .range = VIDEO_RANGE_FULL,
                .colorspace = VIDEO_CS_DEFAULT
            };

            int ret = video_scaler_create(&filter->scaler_simple, &dst, &src, VIDEO_SCALE_DEFAULT);
            blog(LOG_INFO, "ArUco: Video scaler_simple init returned %d", ret);
        }

        struct obs_source_frame *new_frame = obs_source_frame_create(VIDEO_FORMAT_Y800, frame->width, frame->height);
        new_frame->timestamp = frame->timestamp;

        bool ok = video_scaler_scale(
            filter->scaler_simple,
            new_frame->data, new_frame->linesize,
            frame->data, frame->linesize
        );
        if (!ok) {
            blog(LOG_ERROR, "ArUco: Video scaler_in returned fail");
        }

        cv::Mat image(frame->height, frame->width, CV_8UC1, new_frame->data[0]);

        if (image.data != NULL) {
            std::vector<int> ids;
            std::vector<std::vector<cv::Point2f> > corners;

            cv::aruco::detectMarkers(image, filter->dictionary, corners, ids);
            check_markers(filter, frame, ids, corners);
        }

        obs_source_frame_destroy(new_frame);
        return frame;
    }
}

static const char *aruco_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ArUcoTrackerFilter");
}


static void aruco_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "id", 15);
	obs_data_set_default_bool(settings, "draw_marker", true);
	obs_data_set_default_int(settings, "min_move", 20);
	obs_data_set_default_string(settings, "output_path", "/dev/null");
    obs_data_set_default_string(settings, "baudrate", "115200");
}

static void *aruco_create(obs_data_t *settings, obs_source_t *context)
{
	struct aruco_data *filter =
		(struct aruco_data *)bzalloc(sizeof(struct aruco_data));

	filter->context = context;
    filter->dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
    filter->scaler_in = NULL;
    filter->scaler_out = NULL;
    filter->scaler_simple = NULL;
    filter->draw_marker = false;
    filter->min_move_distance = 20;
    filter->output_device = "/dev/null";
    filter->last_x = 0;
    filter->last_y = 0;
    filter->serial_fd = -1;
    filter->baudrate = 115200;

	aruco_update(filter, settings);
    obs_source_update(context, settings);

	return filter;
}

static obs_properties_t *aruco_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, "id", "ID", 0, 49, 1);
	obs_properties_add_bool(props, "draw_marker", "Draw Marker");
	obs_properties_add_int(props, "min_move", "Minimal trigger distance", 0, 2000, 10);
    obs_properties_add_path(props, "output_device", "Serial output device", OBS_PATH_FILE, NULL, "/dev/null");
    obs_property_t *baudrates = obs_properties_add_list(props, "baudrate", "Serial Baudrate", OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(baudrates, "9600", "9600");
    obs_property_list_add_string(baudrates, "19200", "19200");
    obs_property_list_add_string(baudrates, "38400", "38400");
    obs_property_list_add_string(baudrates, "57600", "57600");
    obs_property_list_add_string(baudrates, "115200", "115200");
    obs_property_list_add_string(baudrates, "230400", "230400");
    obs_property_list_add_string(baudrates, "460800", "460800");
    obs_property_list_add_string(baudrates, "921600", "921600");

	UNUSED_PARAMETER(data);
	return props;
}

static void aruco_update(void *data, obs_data_t *settings)
{
	struct aruco_data *filter = (struct aruco_data *)data;
	int id = obs_data_get_int(settings, "id");
    bool draw_marker = obs_data_get_bool(settings, "draw_marker");
    int min_move = obs_data_get_int(settings, "min_move");
    const char *device = obs_data_get_string(settings, "output_device");
    int baudrate = atoi(obs_data_get_string(settings, "baudrate"));

    blog(LOG_INFO, "ArUco: update tracked id to %d", id);

	filter->aruco_id = id;
    filter->draw_marker = draw_marker;
    filter->min_move_distance = min_move;

    int len = strlen(device) > strlen(filter->output_device) ? strlen(filter->output_device) : strlen(device);
    
    if ((strncmp(device, filter->output_device, len) != 0) || (filter->baudrate != baudrate)) {
        filter->output_device = device;
        filter->baudrate = baudrate;

        if (filter->serial_fd >= 0) {
            serial_close(filter->serial_fd);
        }
        filter->serial_fd = serial_open(filter->output_device, filter->baudrate);
    }
}

static void aruco_destroy(void *data)
{
	struct aruco_data *filter = (struct aruco_data *)data;

    if (filter->scaler_in) {
        video_scaler_destroy(filter->scaler_in);
    }
    if (filter->scaler_out) {
        video_scaler_destroy(filter->scaler_out);
    }
    if (filter->scaler_simple) {
        video_scaler_destroy(filter->scaler_simple);
    }
    if (filter->serial_fd >= 0) {
        serial_close(filter->serial_fd);
    }
	bfree(data);
}

struct obs_source_info aruco_tracker_filter = {
	.id = "aruco_tracker_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name = aruco_getname,
	.create = aruco_create,
	.destroy = aruco_destroy,
	.get_defaults = aruco_defaults,
	.get_properties = aruco_properties,
	.update = aruco_update,
	.filter_video = aruco_filter_video
};