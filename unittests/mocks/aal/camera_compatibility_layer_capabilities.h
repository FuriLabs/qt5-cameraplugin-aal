/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef CAMERA_COMPATIBILITY_LAYER_CONFIGURATION_H_
#define CAMERA_COMPATIBILITY_LAYER_CONFIGURATION_H_

#ifdef __cplusplus
extern "C" {
#endif

struct CameraControl;

typedef enum
{
    FLASH_MODE_OFF,
    FLASH_MODE_AUTO,
    FLASH_MODE_ON,
    FLASH_MODE_TORCH,
    FLASH_MODE_RED_EYE
} FlashMode;

typedef enum
{
    WHITE_BALANCE_MODE_AUTO,
    WHITE_BALANCE_MODE_DAYLIGHT,
    WHITE_BALANCE_MODE_CLOUDY_DAYLIGHT,
    WHITE_BALANCE_MODE_FLUORESCENT,
    WHITE_BALANCE_MODE_INCANDESCENT
} WhiteBalanceMode;

typedef enum
{
    SCENE_MODE_AUTO,
    SCENE_MODE_ACTION,
    SCENE_MODE_NIGHT,
    SCENE_MODE_PARTY,
    SCENE_MODE_SUNSET,
    SCENE_MODE_HDR
} SceneMode;

typedef enum
{
    AUTO_FOCUS_MODE_OFF,
    AUTO_FOCUS_MODE_CONTINUOUS_VIDEO,
    AUTO_FOCUS_MODE_AUTO,
    AUTO_FOCUS_MODE_MACRO,
    AUTO_FOCUS_MODE_CONTINUOUS_PICTURE,
    AUTO_FOCUS_MODE_INFINITY
} AutoFocusMode;

typedef enum
{
    EFFECT_MODE_NONE,
    EFFECT_MODE_MONO,
    EFFECT_MODE_NEGATIVE,
    EFFECT_MODE_SOLARIZE,
    EFFECT_MODE_SEPIA,
    EFFECT_MODE_POSTERIZE,
    EFFECT_MODE_WHITEBOARD,
    EFFECT_MODE_BLACKBOARD,
    EFFECT_MODE_AQUA
} EffectMode;

typedef enum
{
    CAMERA_PIXEL_FORMAT_YUV422SP,
    CAMERA_PIXEL_FORMAT_YUV420SP,
    CAMERA_PIXEL_FORMAT_YUV422I,
    CAMERA_PIXEL_FORMAT_YUV420P,
    CAMERA_PIXEL_FORMAT_RGB565,
    CAMERA_PIXEL_FORMAT_RGBA8888,
    CAMERA_PIXEL_FORMAT_BAYER_RGGB
} CameraPixelFormat;

typedef struct
{
    int top;
    int left;
    int bottom;
    int right;
    int weight;
} FocusRegion;

typedef struct
{
    int top;
    int left;
    int bottom;
    int right;
    int weight;
} MeteringRegion;

typedef void (*size_callback)(void* ctx, int width, int height);
typedef void (*scene_mode_callback)(void* ctx, SceneMode mode);
typedef void (*flash_mode_callback)(void* ctx, FlashMode mode);

// Dumps the camera parameters to stdout.
void android_camera_dump_parameters(CameraControl* control);

// Query camera parameters

int android_camera_get_number_of_devices();
void android_camera_enumerate_supported_preview_sizes(CameraControl* control, size_callback cb, void* ctx);
void android_camera_get_preview_fps_range(CameraControl* control, int* min, int* max);
void android_camera_get_preview_fps(CameraControl* control, int* fps);
void android_camera_enumerate_supported_picture_sizes(CameraControl* control, size_callback cb, void* ctx);
void android_camera_get_preview_size(CameraControl* control, int* width, int* height);
void android_camera_get_picture_size(CameraControl* control, int* width, int* height);

void android_camera_get_current_zoom(CameraControl* control, int* zoom);
void android_camera_get_max_zoom(CameraControl* control, int* max_zoom);

void android_camera_get_effect_mode(CameraControl* control, EffectMode* mode);
void android_camera_get_flash_mode(CameraControl* control, FlashMode* mode);
void android_camera_enumerate_supported_flash_modes(CameraControl* control, flash_mode_callback cb, void* ctx);
void android_camera_get_white_balance_mode(CameraControl* control, WhiteBalanceMode* mode);
void android_camera_enumerate_supported_scene_modes(CameraControl* control, scene_mode_callback cb, void* ctx);
void android_camera_get_scene_mode(CameraControl* control, SceneMode* mode);
void android_camera_get_auto_focus_mode(CameraControl* control, AutoFocusMode* mode);
void android_camera_get_preview_format(CameraControl* control, CameraPixelFormat* format);
void android_camera_get_jpeg_quality(struct CameraControl* control, int* quality);

// Adjusts camera parameters
void android_camera_set_preview_size(CameraControl* control, int width, int height);
void android_camera_set_preview_fps(CameraControl* control, int fps);
void android_camera_set_picture_size(CameraControl* control, int width, int height);
void android_camera_set_effect_mode(CameraControl* control, EffectMode mode);
void android_camera_set_flash_mode(CameraControl* control, FlashMode mode);
void android_camera_set_white_balance_mode(CameraControl* control, WhiteBalanceMode mode);
void android_camera_set_scene_mode(CameraControl* control, SceneMode mode);
void android_camera_set_auto_focus_mode(CameraControl* control, AutoFocusMode mode);
void android_camera_set_preview_format(CameraControl* control, CameraPixelFormat format);
void android_camera_set_jpeg_quality(struct CameraControl* control, int quality);

void android_camera_set_focus_region(CameraControl* control, FocusRegion* region);
void android_camera_reset_focus_region(CameraControl* control);
void android_camera_set_metering_region(CameraControl* control, MeteringRegion* region);

// Set photo metadata
void android_camera_set_rotation(CameraControl* control, int rotation);
void android_camera_set_location(CameraControl* control, const float* latitude, const float* longitude, const float* altitude, int timestamp, const char* method);


#ifdef __cplusplus
}
#endif

#endif // CAMERA_COMPATIBILITY_LAYER_CONFIGURATION_H_
