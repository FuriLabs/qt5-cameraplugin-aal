/*
 * Copyright (C) 2012 Canonical, Ltd.
 *
 * Authors:
 *  Guenter Schwann <guenter.schwann@canonical.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "aalcamerafocuscontrol.h"
#include "aalcameraservice.h"
#include "aalimagecapturecontrol.h"

#include <QDebug>
#include <QTimer>

#include <algorithm>

/// The size of the full sensor is +/- of this one in x and y
const int focusFullSize = 1000;
/// The size of the focus area is  +/- of this one in x and y
const int focusRegionSize = 100;

AalCameraFocusControl::AalCameraFocusControl(AalCameraService *service, QObject *parent)
    : QCameraFocusControl(parent),
      m_service(service),
      m_focusPoint(0.0, 0.0),
      setOnInit(false),
      m_focusRunning(false)
{
    m_focusRegion.weight = -9.9;
}

QPointF AalCameraFocusControl::customFocusPoint() const
{
    return m_focusPoint;
}

QCameraFocus::FocusModes AalCameraFocusControl::focusMode() const
{
    return m_focusMode;
}

QCameraFocus::FocusPointMode AalCameraFocusControl::focusPointMode() const
{
    return m_focusPointMode;
}

QCameraFocusZoneList AalCameraFocusControl::focusZones() const
{
    return QCameraFocusZoneList();
}

bool AalCameraFocusControl::isFocusModeSupported(QCameraFocus::FocusModes mode) const
{
    if (mode == QCameraFocus::HyperfocalFocus)
        return false;

    return true;
}

bool AalCameraFocusControl::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
{
    if (mode == QCameraFocus::FocusPointFaceDetection)
        return false;

    return true;
}

void AalCameraFocusControl::setCustomFocusPoint(const QPointF &point)
{
    if (m_focusPoint == point)
        return;

    m_focusPoint = point;
    m_focusRegion = point2Region(m_focusPoint);
    Q_EMIT customFocusPointChanged(m_focusPoint);

    if (m_service->androidControl()) {
        android_camera_set_focus_region(m_service->androidControl(), &m_focusRegion);
        startFocus();
    }
}

void AalCameraFocusControl::setFocusMode(QCameraFocus::FocusModes mode)
{
    if (m_focusMode == mode || !isFocusModeSupported(mode))
        return;

    AutoFocusMode focusMode = qt2Android(mode);
    m_focusMode = mode;
    if (m_service->androidControl()) {
        android_camera_set_auto_focus_mode(m_service->androidControl(), focusMode);
    } else {
        setOnInit = true;
    }

    Q_EMIT focusModeChanged(m_focusMode);
}

void AalCameraFocusControl::setFocusPointMode(QCameraFocus::FocusPointMode mode)
{
    if (m_focusPointMode == mode || !isFocusPointModeSupported(mode))
        return;

    m_focusPointMode = mode;
    Q_EMIT focusPointModeChanged(m_focusPointMode);
}

void AalCameraFocusControl::focusCB(void *context)
{
    Q_UNUSED(context);
    AalCameraService::instance()->focusControl()->m_focusRunning = false;
    QMetaObject::invokeMethod(AalCameraService::instance()->imageCaptureControl(),
                              "updateReady", Qt::QueuedConnection);
}

bool AalCameraFocusControl::isFocusBusy() const
{
    return m_focusRunning;
}

void AalCameraFocusControl::init(CameraControl *control, CameraControlListener *listener)
{
    listener->on_msg_focus_cb = &AalCameraFocusControl::focusCB;

    if (setOnInit) {
        AutoFocusMode mode = qt2Android(m_focusMode);
        android_camera_set_auto_focus_mode(control, mode);
        if (m_focusRegion.weight > 0.0) {
            android_camera_set_focus_region(control, &m_focusRegion);
            QTimer::singleShot(1, this, SLOT(startFocus()));
        }
        setOnInit = false;
    } else {
        AutoFocusMode mode;
        android_camera_get_auto_focus_mode(control, &mode);
        m_focusMode = android2Qt(mode);
    }
}

void AalCameraFocusControl::startFocus()
{
    if (!m_service->androidControl())
        return;

    m_focusRunning = true;
    m_service->imageCaptureControl()->updateReady();
    android_camera_start_autofocus(m_service->androidControl());
}

AutoFocusMode AalCameraFocusControl::qt2Android(QCameraFocus::FocusModes mode)
{
    switch(mode) {
    case QCameraFocus::ManualFocus:
        return AUTO_FOCUS_MODE_OFF;
    case QCameraFocus::InfinityFocus:
        return AUTO_FOCUS_MODE_INFINITY;
    case QCameraFocus::ContinuousFocus:
        return AUTO_FOCUS_MODE_CONTINUOUS_PICTURE;
    case QCameraFocus::MacroFocus:
        return AUTO_FOCUS_MODE_MACRO;
    case QCameraFocus::AutoFocus:
    case QCameraFocus::HyperfocalFocus:
    default:
        return AUTO_FOCUS_MODE_AUTO;
    }
}

QCameraFocus::FocusModes AalCameraFocusControl::android2Qt(AutoFocusMode mode)
{
    switch(mode) {
    case AUTO_FOCUS_MODE_OFF:
        return QCameraFocus::ManualFocus;
    case AUTO_FOCUS_MODE_INFINITY:
        return QCameraFocus::InfinityFocus;
    case AUTO_FOCUS_MODE_CONTINUOUS_PICTURE:
        return QCameraFocus::ContinuousFocus;
    case AUTO_FOCUS_MODE_MACRO:
        return QCameraFocus::MacroFocus;
    case AUTO_FOCUS_MODE_AUTO:
    case AUTO_FOCUS_MODE_CONTINUOUS_VIDEO:
    default:
        return QCameraFocus::AutoFocus;
    }
}

FocusRegion AalCameraFocusControl::point2Region(const QPointF &point) const
{
    int centerX = (point.x() * (2* focusFullSize)) - focusFullSize;
    int maxCenterPosition = focusFullSize - focusRegionSize;
    centerX = std::max(std::min(centerX, maxCenterPosition), -maxCenterPosition);
    int centerY = -1 * ((point.y() * (2 * focusFullSize)) - focusFullSize);
    centerY = std::max(std::min(centerY, maxCenterPosition), -maxCenterPosition);

    FocusRegion region;
    region.left = centerX - focusRegionSize;
    region.right = centerX + focusRegionSize;
    region.top = centerY - focusRegionSize;
    region.bottom = centerY + focusRegionSize;
    region.weight = 5;

    return region;
}
