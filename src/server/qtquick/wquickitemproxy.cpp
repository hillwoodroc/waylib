// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "wquickitemproxy.h"
#include "wquickitemproxy_p.h"

#include <QSGImageNode>
#include <private/qquickitem_p.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

WQuickItemProxyPrivate::~WQuickItemProxyPrivate()
{
    initSourceItem(sourceItem, nullptr);
}

#define LAYER "__layer_enabled_by_WQuickItemProxy"

void WQuickItemProxyPrivate::initSourceItem(QQuickItem *old, QQuickItem *item)
{
    if (textureChangedConnection)
        QObject::disconnect(textureChangedConnection);

    if (old) {
        QQuickItemPrivate *sd = QQuickItemPrivate::get(old);
        sd->derefFromEffectItem(hideSource);

        if (old->property(LAYER).toBool()) {
            sd->layer()->setEnabled(false);
            old->setProperty(LAYER, QVariant());
        }
    }

    if (item) {
        QQuickItemPrivate *sd = QQuickItemPrivate::get(item);
        sd->refFromEffectItem(hideSource);

        if (!item->isTextureProvider()) {
            item->setProperty(LAYER, true);
            sd->layer()->setEnabled(true);
        }

        auto tp = item->textureProvider();
        textureChangedConnection = QObject::connect(tp, SIGNAL(textureChanged()),
                                                    q_func(), SLOT(onTextureChanged()));
    }

    updateImplicitSize();
}

void WQuickItemProxyPrivate::onTextureChanged()
{
    W_Q(WQuickItemProxy);

    const auto texture = sourceItem->textureProvider()->texture();
    QSize newTextureSize = texture ? texture->textureSize() : QSize();
    if (textureSize != newTextureSize) {
        textureSize = newTextureSize;
        updateImplicitSize();
    }
    q->update();
}

void WQuickItemProxyPrivate::updateImplicitSize()
{
    if (!textureSize.isEmpty()) {
        W_Q(WQuickItemProxy);
        const qreal dpr = q->window() ? q->window()->effectiveDevicePixelRatio() : qApp->devicePixelRatio();
        q->setImplicitSize(textureSize.width() / dpr, textureSize.height() / dpr);
    }
}

WQuickItemProxy::WQuickItemProxy(QQuickItem *parent)
    : QQuickItem (parent)
    , WObject(*new WQuickItemProxyPrivate(this))
{
    setFlag(ItemHasContents);
}

WQuickItemProxy::~WQuickItemProxy()
{
    if (window()) {
        WQuickItemProxy::releaseResources();
    }
}

QQuickItem *WQuickItemProxy::sourceItem() const
{
    W_DC(WQuickItemProxy);
    return d->sourceItem;
}

QRectF WQuickItemProxy::sourceRect() const
{
    W_DC(WQuickItemProxy);
    return d->sourceRect;
}

bool WQuickItemProxy::hideSource() const
{
    W_DC(WQuickItemProxy);
    return d->hideSource;
}

void WQuickItemProxy::setHideSource(bool newHideSource)
{
    W_D(WQuickItemProxy);
    if (d->hideSource == newHideSource)
        return;

    if (d->sourceItem) {
        QQuickItemPrivate::get(d->sourceItem)->refFromEffectItem(newHideSource);
        QQuickItemPrivate::get(d->sourceItem)->derefFromEffectItem(d->hideSource);
    }
    d->hideSource = newHideSource;
    Q_EMIT hideSourceChanged();
}

bool WQuickItemProxy::mipmap() const
{
    W_DC(WQuickItemProxy);
    return d->mipmap;
}

void WQuickItemProxy::setMipmap(bool newMipmap)
{
    W_D(WQuickItemProxy);
    if (d->mipmap == newMipmap)
        return;
    d->mipmap = newMipmap;
    update();
    Q_EMIT mipmapChanged();
}

bool WQuickItemProxy::isTextureProvider() const
{
    if (QQuickItem::isTextureProvider())
        return true;

    W_DC(WQuickItemProxy);
    return d->sourceItem && d->sourceItem->isTextureProvider();
}

QSGTextureProvider *WQuickItemProxy::textureProvider() const
{
    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    W_DC(WQuickItemProxy);
    if (!d->sourceItem)
        return nullptr;

    return d->sourceItem->textureProvider();
}

void WQuickItemProxy::setSourceItem(QQuickItem *sourceItem)
{
    W_D(WQuickItemProxy);

    if (d->sourceItem == sourceItem)
        return;

    if (isComponentComplete()) {
        d->initSourceItem(d->sourceItem, sourceItem);
    }

    d->sourceItem = sourceItem;
    Q_EMIT sourceItemChanged();
    update();
}

void WQuickItemProxy::setSourceRect(const QRectF &sourceRect)
{
    W_D(WQuickItemProxy);
    if (d->sourceRect == sourceRect)
        return;

    d->sourceRect = sourceRect;
    Q_EMIT sourceRectChanged();
    update();
}

QSGNode *WQuickItemProxy::updatePaintNode(QSGNode *old, QQuickItem::UpdatePaintNodeData *)
{
    W_D(WQuickItemProxy);

    if (Q_UNLIKELY(!d->sourceItem || d->sourceItem->width() <=0 || d->sourceItem->height() <= 0)) {
        delete old;
        return nullptr;
    }

    const auto tp = d->sourceItem->textureProvider();
    if (Q_LIKELY(!tp || !tp->texture())) {
        delete old;
        return nullptr;
    }

    auto node = static_cast<QSGImageNode*>(old);
    if (Q_UNLIKELY(!node)) {
        auto texture = tp->texture();
        node = window()->createImageNode();
        node->setOwnsTexture(false);
        node->setTexture(texture);
    } else {
        node->markDirty(QSGNode::DirtyMaterial);
    }

    QRectF sourceRect = d->sourceRect;
    if (!sourceRect.isValid())
        sourceRect = QRectF(QPointF(0, 0), d->textureSize);

    node->setSourceRect(sourceRect);
    node->setRect(QRectF(QPointF(0, 0), size()));
    node->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
    node->setMipmapFiltering(d->mipmap ? node->filtering() : QSGTexture::None);
    node->setAnisotropyLevel(antialiasing() ? QSGTexture::Anisotropy4x : QSGTexture::AnisotropyNone);

    return node;
}

void WQuickItemProxy::componentComplete()
{
    W_D(WQuickItemProxy);

    if (d->sourceItem)
        d->initSourceItem(nullptr, d->sourceItem);

    return QQuickItem::componentComplete();
}

void WQuickItemProxy::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);

    if (change == ItemDevicePixelRatioHasChanged || change == ItemSceneChange)
        d_func()->updateImplicitSize();
}

WAYLIB_SERVER_END_NAMESPACE

#include "moc_wquickitemproxy.cpp"