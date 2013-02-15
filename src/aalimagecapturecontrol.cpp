/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "aalimagecapturecontrol.h"
#include "aalimageencodercontrol.h"
#include "aalcameraservice.h"
#include "aalvideorenderercontrol.h"
#include "storagemanager.h"

#include <camera_compatibility_layer.h>
#include <camera_compatibility_layer_capabilities.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <cmath>

#include <ubuntu/application/ui/ubuntu_application_ui.h>

const int PREVIEW_WIDTH_MAX = 360;
const int PREVIEW_HEIGHT_MAX = 360;
const int PREVIEW_QUALITY = 70;
const char* PREVIEW_FILE_FORMAT = "JPEG";
const QLatin1String PREVIEW_FILE_EXT = QLatin1String("JPG");
const QLatin1String PREVIEW_DIR = QLatin1String(".thumbs");

AalImageCaptureControl::AalImageCaptureControl(AalCameraService *service, QObject *parent)
   : QCameraImageCaptureControl(parent),
    m_service(service),
    m_cameraControl(service->cameraControl()),
    m_lastRequestId(0),
    m_ready(false),
    m_pendingCaptureFile(),
    m_photoWidth(320),
    m_photoHeight(240),
    m_aspectRatio(0.0),
    m_screenAspectRatio(0.0)
{
    m_galleryPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

AalImageCaptureControl::~AalImageCaptureControl()
{
}

bool AalImageCaptureControl::isReadyForCapture() const
{
    return m_ready;
}

int AalImageCaptureControl::capture(const QString &fileName)
{
    m_lastRequestId++;
    if (!m_ready || !m_service->androidControl()) {
        emit error(m_lastRequestId, QCameraImageCapture::NotReadyError,
                   QLatin1String("Camera not ready to capture"));
        return m_lastRequestId;
    }

    QFileInfo fi(fileName);
    if (fileName.isEmpty() || fi.isDir()) {
        m_pendingCaptureFile = m_storageManager.nextPhotoFileName(fileName);
    } else {
        m_pendingCaptureFile = fileName;
    }
    bool diskOk = m_storageManager.checkDirectory(m_pendingCaptureFile);
    if (!diskOk) {
        emit error(m_lastRequestId, QCameraImageCapture::ResourceError,
                   QString("Won't be able to save file %1 to disk").arg(m_pendingCaptureFile));
        return m_lastRequestId;
    }

    int rotation = 90;
    if (!m_service->isBackCameraUsed())
        rotation = 270;
    android_camera_set_rotation(m_service->androidControl(), rotation);

    android_camera_take_snapshot(m_service->androidControl());

    m_service->updateCaptureReady();

    m_service->videoOutputControl()->createPreview();
    Q_EMIT imageCaptured(m_lastRequestId, m_service->videoOutputControl()->preview());

    return m_lastRequestId;
}

void AalImageCaptureControl::cancelCapture()
{
}

void AalImageCaptureControl::shutterCB(void *context)
{
    Q_UNUSED(context);
    QMetaObject::invokeMethod(AalCameraService::instance()->imageCaptureControl(),
                              "shutter", Qt::QueuedConnection);
}

void AalImageCaptureControl::saveJpegCB(void *data, uint32_t data_size, void *context)
{
    Q_UNUSED(context);
    AalCameraService::instance()->imageCaptureControl()->saveJpeg(data, data_size);
}

void AalImageCaptureControl::init(CameraControl *control, CameraControlListener *listener)
{
    Q_UNUSED(control);

    // Set the optimal image resolution that will be used by the camera
    QImageEncoderSettings settings;
    AalImageEncoderControl *imageEncoderControl = AalCameraService::instance()->imageEncoderControl();
    if (!imageEncoderControl->supportedResolutions(settings).empty()) {
        imageEncoderControl->setSize(
                chooseOptimalSize(imageEncoderControl->supportedResolutions(settings)));
    }
    else
        qWarning() << "No supported resolutions detected for currently selected camera device." << endl;

    listener->on_msg_shutter_cb = &AalImageCaptureControl::shutterCB;
    listener->on_data_compressed_image_cb = &AalImageCaptureControl::saveJpegCB;
}

void AalImageCaptureControl::setReady(bool ready)
{
    if (m_ready != ready) {
        m_ready = ready;
        Q_EMIT readyForCaptureChanged(m_ready);
    }
}

bool AalImageCaptureControl::isCaptureRunning() const
{
    return !m_pendingCaptureFile.isNull();
}

float AalImageCaptureControl::getAspectRatio() const
{
    return m_aspectRatio;
}

void AalImageCaptureControl::shutter()
{
    Q_EMIT imageExposed(m_lastRequestId);
}

QSize AalImageCaptureControl::chooseOptimalSize(const QList<QSize> &sizes)
{
    QSize optimalSize;

    if (!sizes.empty()) {
        getPriorityAspectRatios();

        // Loop over all reported camera resolutions until we find the highest
        // one that matches the current prioritized aspect ratio. If it doesn't
        // find one on the current aspect ration, it selects the next ratio and
        // tries again.
        QList<float>::const_iterator ratioIt = m_prioritizedAspectRatios.begin();
        while (ratioIt != m_prioritizedAspectRatios.end() && optimalSize.isEmpty()) {
            m_aspectRatio = m_prioritizedAspectRatios.front();

            QList<QSize>::const_iterator it = sizes.begin();
            while (it != sizes.end()) {
                const float ratio = (float)(*it).width() / (float)(*it).height();
                const float EPSILON = 10e-3;
                if (fabs(ratio - m_aspectRatio) < EPSILON) {
                    optimalSize = *it;
                    break;
                }
                ++it;
            }
            ++ratioIt;
        }
    }

    return optimalSize;
}

float AalImageCaptureControl::getScreenAspectRatio()
{
    // Only get the screen aspect ratio once, otherwise use the cached copy
    if (m_screenAspectRatio == 0.0) {
        // Get screen resolution.
        ubuntu_application_ui_physical_display_info info;
        ubuntu_application_ui_create_display_info(&info, 0);

        const int kScreenWidth = ubuntu_application_ui_query_horizontal_resolution(info);
        const int kScreenHeight = ubuntu_application_ui_query_vertical_resolution(info);
        Q_ASSERT(kScreenWidth > 0 && kScreenHeight > 0);

        ubuntu_application_ui_destroy_display_info(info);

        m_screenAspectRatio = (kScreenWidth > kScreenHeight) ?
            ((float)kScreenWidth / (float)kScreenHeight) : ((float)kScreenHeight / (float)kScreenWidth);
    }

    return m_screenAspectRatio;
}

void AalImageCaptureControl::getPriorityAspectRatios()
{
    m_prioritizedAspectRatios.clear();

    if (m_service->isBackCameraUsed()) {
        if (m_screenAspectRatio > 0.0f) {
            m_prioritizedAspectRatios.append(getScreenAspectRatio());
        }
        // Prioritized list of aspect ratios for the back camera
        const float backAspectRatios[4] = { 16.0f/9.0f, 15.0f/10.0f, 4.0f/3.0f, 5.0f/4.0f };
        for (uint8_t i=0; i<4; ++i) {
            if (!m_prioritizedAspectRatios.contains(backAspectRatios[i])) {
                m_prioritizedAspectRatios.append(backAspectRatios[i]);
            }
        }
    } else {
        // Prioritized list of aspect ratios for the front camera
        const float frontAspectRatios[4] = { 4.0f/3.0f, 5.0f/4.0f, 16.0f/9.0f, 15.0f/10.0f };
        for (uint8_t i=0; i<4; ++i) {
            m_prioritizedAspectRatios.append(frontAspectRatios[i]);
        }
    }
}

void AalImageCaptureControl::saveJpeg(void *data, uint32_t dataSize)
{
    if (m_pendingCaptureFile.isNull() || !m_service->androidControl())
        return;

    QTemporaryFile file;
    if (!file.open()) {
        emit error(m_lastRequestId, QCameraImageCapture::ResourceError,
                   QString("Could not open temprary file %1").arg(file.fileName()));
        m_pendingCaptureFile.clear();
        m_service->updateCaptureReady();
        return;
    }

    qint64 writtenSize = file.write((const char*)data, dataSize);
    file.close();
    if (writtenSize != dataSize) {
        emit error(m_lastRequestId, QCameraImageCapture::ResourceError,
                   QString("Could not write file %1").arg(file.fileName()));
        m_pendingCaptureFile.clear();
        m_service->updateCaptureReady();
        return;
    }

    if (imageIsInGallery(m_pendingCaptureFile))
        saveThumbnail((const uchar*)data, dataSize);

    QFile finalFile(file.fileName());
    bool ok = finalFile.rename(m_pendingCaptureFile);
    if (!ok) {
        emit error(m_lastRequestId, QCameraImageCapture::ResourceError,
                   QString("Could not save image to %1").arg(m_pendingCaptureFile));
        m_pendingCaptureFile.clear();
        m_service->updateCaptureReady();
        return;
    }

    Q_EMIT imageSaved(m_lastRequestId, m_pendingCaptureFile);
    m_pendingCaptureFile.clear();

    android_camera_start_preview(m_service->androidControl());
    m_service->updateCaptureReady();
}

bool AalImageCaptureControl::imageIsInGallery(const QString &fileName) const
{
    QFileInfo fi(fileName);
    return fi.absolutePath() == m_galleryPath;
}

bool AalImageCaptureControl::saveThumbnail(const uchar *data, int dataSize)
{
    QString thumbnailDir = m_galleryPath + "/" + PREVIEW_DIR;
    QDir tdir(thumbnailDir);
    if (!tdir.exists()) {
        tdir.mkpath(thumbnailDir);
        if (!tdir.exists()) {
            qWarning() << "Can't create directory for the gallery thumbnail " << thumbnailDir;
            return false;
        }
    }

    QImage fullsized;
    fullsized.loadFromData(data, dataSize);
    if (fullsized.isNull()) {
        qWarning() << "Can't load the full sized image for thumbnail generation";
        return false;
    }

    QImage scaled = (fullsized.height() > fullsized.width())
      ? fullsized.scaledToWidth(PREVIEW_WIDTH_MAX, Qt::SmoothTransformation)
      : fullsized.scaledToHeight(PREVIEW_HEIGHT_MAX, Qt::SmoothTransformation);

    QFileInfo fi(m_pendingCaptureFile);
    QString thumbnailName =  thumbnailDir + "/" + fi.completeBaseName() + "_th." + PREVIEW_FILE_EXT;

    return scaled.save(thumbnailName, PREVIEW_FILE_FORMAT, PREVIEW_QUALITY);
}
