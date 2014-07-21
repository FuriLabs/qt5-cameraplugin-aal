/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include "audiocapture.h"

#include <QThread>

#include <hybris/media/media_recorder_layer.h>

AudioCapture::AudioCapture(MediaRecorderWrapper *mediaRecorder)
{
    Q_UNUSED(mediaRecorder);
}

AudioCapture::~AudioCapture()
{
}

bool AudioCapture::init()
{
    return true;
}

void AudioCapture::run()
{
}

void AudioCapture::moveToThread(QThread *thread)
{
    Q_UNUSED(thread);
}

void AudioCapture::setStartWorkerThreadCb(StartWorkerThreadCb cb, void *context)
{
    Q_UNUSED(cb);
    Q_UNUSED(context);
}

void AudioCapture::startThreadLoop()
{
}

int AudioCapture::readMicrophone()
{
    return 0;
}

void AudioCapture::onReadMicrophone(void *context)
{
    Q_UNUSED(context);
}
