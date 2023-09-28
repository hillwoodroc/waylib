// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wglobal.h>
#include <woutput.h>
#include <qwglobal.h>

#include <QQuickItem>

QW_BEGIN_NAMESPACE
class QWBuffer;
QW_END_NAMESPACE

WAYLIB_SERVER_BEGIN_NAMESPACE

class WOutputViewportPrivate;
class WAYLIB_SERVER_EXPORT WOutputViewport : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WOutputViewport)
    Q_PROPERTY(WOutput* output READ output WRITE setOutput REQUIRED)
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY devicePixelRatioChanged)
    Q_PROPERTY(bool offscreen READ offscreen WRITE setOffscreen NOTIFY offscreenChanged)
    Q_PROPERTY(bool root READ isRoot WRITE setRoot NOTIFY rootChanged FINAL)
    QML_NAMED_ELEMENT(OutputViewport)

public:
    explicit WOutputViewport(QQuickItem *parent = nullptr);
    ~WOutputViewport();

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

    WOutput *output() const;
    void setOutput(WOutput *newOutput);

    void setBuffer(QW_NAMESPACE::QWBuffer *buffer);

    qreal devicePixelRatio() const;
    void setDevicePixelRatio(qreal newDevicePixelRatio);

    bool offscreen() const;
    void setOffscreen(bool newOffscreen);

    bool isRoot() const;
    void setRoot(bool newRoot);

public Q_SLOTS:
    void setOutputScale(float scale);
    void rotateOutput(WOutput::Transform t);

Q_SIGNALS:
    void devicePixelRatioChanged();
    void offscreenChanged();
    void rootChanged();

private:
    void componentComplete() override;
    void releaseResources() override;

    // Using by Qt library
    W_PRIVATE_SLOT(void invalidateSceneGraph())
};

WAYLIB_SERVER_END_NAMESPACE
