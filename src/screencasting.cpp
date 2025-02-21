// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "screencasting.h"
#include "qwayland-zkde-screencast-unstable-v1.h"
#include <QRect>
#include <QDebug>
#include <DWayland/Client/registry.h>
#include <DWayland/Client/output.h>
#include <DWayland/Client/plasmawindowmanagement.h>

using namespace KWayland::Client;

class ScreencastingStreamPrivate : public QtWayland::zkde_screencast_stream_unstable_v1
{
public:
    ScreencastingStreamPrivate(ScreencastingStream* q) : q(q) {}
    ~ScreencastingStreamPrivate() {
        close();
        q->deleteLater();
    }

    void zkde_screencast_stream_unstable_v1_created(uint32_t node) override {
        m_nodeid = node;
        Q_EMIT q->created(node);
    }

    void zkde_screencast_stream_unstable_v1_closed() override {
        Q_EMIT q->closed();
    }

    void zkde_screencast_stream_unstable_v1_failed(const QString &error) override {
        Q_EMIT q->failed(error);
    }

    uint m_nodeid = 0;
    QPointer<ScreencastingStream> q;
};

ScreencastingStream::ScreencastingStream(QObject* parent)
    : QObject(parent)
    , d(new ScreencastingStreamPrivate(this))
{
}

ScreencastingStream::~ScreencastingStream() = default;

quint32 ScreencastingStream::nodeid() const
{
    return d->m_nodeid;
}

class ScreencastingPrivate : public QtWayland::zkde_screencast_unstable_v1
{
public:
    ScreencastingPrivate(Registry *registry, int id, int version, Screencasting *q)
        : QtWayland::zkde_screencast_unstable_v1(*registry, id, version)
        , q(q)
    {
    }

    ScreencastingPrivate(::zkde_screencast_unstable_v1* screencasting, Screencasting *q)
        : QtWayland::zkde_screencast_unstable_v1(screencasting)
        , q(q)
    {
    }

    ~ScreencastingPrivate()
    {
        destroy();
    }

    Screencasting *const q;
};

Screencasting::Screencasting(QObject* parent)
    : QObject(parent)
{}

Screencasting::Screencasting(Registry *registry, int id, int version, QObject* parent)
    : QObject(parent)
    , d(new ScreencastingPrivate(registry, id, version, this))
{}

Screencasting::~Screencasting() = default;

ScreencastingStream* Screencasting::createOutputStream(Output* output, CursorMode mode)
{
    auto stream = new ScreencastingStream(this);
    stream->setObjectName(output->model());
    stream->d->init(d->stream_output(*output, mode));
    return stream;
}

ScreencastingStream* Screencasting::createWindowStream(PlasmaWindow *window, CursorMode mode)
{
    auto stream = createWindowStream(QString::number(window->internalId()), mode);
    stream->setObjectName(window->appId());
    return stream;
}

ScreencastingStream* Screencasting::createWindowStream(const QString &uuid, CursorMode mode)
{
    auto stream = new ScreencastingStream(this);
    stream->d->init(d->stream_window(uuid, mode));
    return stream;
}

void Screencasting::setup(::zkde_screencast_unstable_v1* screencasting)
{
    d.reset(new ScreencastingPrivate(screencasting, this));
}

void Screencasting::destroy()
{
    d.reset(nullptr);
}
