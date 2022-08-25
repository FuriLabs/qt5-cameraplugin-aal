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

#ifndef AALVIDEORENDERERCONTROL_H
#define AALVIDEORENDERERCONTROL_H

#include <QImage>
#include <QVideoRendererControl>
#include <qgl.h>

class AalCameraService;
struct CameraControl;
struct CameraControlListener;
class AalTextureBufferMapper;

class AalVideoRendererControl : public QVideoRendererControl
{
    Q_OBJECT
public:
    AalVideoRendererControl(AalCameraService *service, QObject *parent = 0);
    ~AalVideoRendererControl();

    QAbstractVideoSurface *surface() const;
    void setSurface(QAbstractVideoSurface *surface);

    static void updateViewfinderFrameCB(void *context);

    const QImage &preview() const;
    void createPreview();

    bool isPreviewStarted() const;

public Q_SLOTS:
    void init(CameraControl *control, CameraControlListener *listener);
    void startPreview();
    void stopPreview();

Q_SIGNALS:
    void surfaceChanged(QAbstractVideoSurface *surface);
    void previewReady();

private Q_SLOTS:
    void updateViewfinderFrame();
    void onTextureCreated(unsigned int textureID);
    void onSnapshotTaken(QImage snapshotImage);

private:
    QAbstractVideoSurface *m_surface;
    AalCameraService *m_service;
    AalTextureBufferMapper* m_mapper;

    bool m_viewFinderRunning;
    bool m_previewStarted;
    GLuint m_textureId;
    QImage m_preview;
};

#endif
