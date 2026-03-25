#include "qtframebuffer.h"

#include <array>
#include <cassert>

#include <QImage>
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>

#include "core/debug.h"
#include "core/emu.h"
#include "core/lcd.h"
#include "core/casplus.h"
#include "core/misc.h"

#include "qtkeypadbridge.h"

QImage renderFramebuffer()
{
    static std::array<uint16_t, 320 * 240> framebuffer16;
    static std::array<uint8_t, 240 * 160> framebuffer4bpp;
    static uint16_t sample0 = 0, sample1 = 0;

    if(emulate_cx)
    {
        lcd_cx_draw_frame(framebuffer16.data());
    }
    else
    {
        if(emulate_casplus)
            casplus_lcd_draw_frame(reinterpret_cast<uint8_t (*)[160]>(framebuffer4bpp.data()));
        else
            lcd_draw_frame(framebuffer4bpp.data());

        auto gray4_to_rgb565 = [](uint8_t gray4) -> uint16_t {
            uint8_t gray8 = static_cast<uint8_t>((15 - gray4) * 17);
            return static_cast<uint16_t>(((gray8 >> 3) << 11) | ((gray8 >> 2) << 5) | (gray8 >> 3));
        };

        uint16_t *out = framebuffer16.data();
        for(size_t row = 0; row < 240; ++row)
        {
            const uint8_t *in = framebuffer4bpp.data() + row * 160;
            for(size_t x = 0; x < 160; ++x)
            {
                uint8_t packed = *in++;
                uint8_t gray4a = packed >> 4;
                uint8_t gray4b = packed & 0x0F;

                *out++ = gray4_to_rgb565(gray4a);
                *out++ = gray4_to_rgb565(gray4b);
            }
        }
    }

    sample0 = framebuffer16[0];
    sample1 = framebuffer16[1];

#ifdef __EMSCRIPTEN__
    static int frameCounter = 0;
    if(++frameCounter % 120 == 0)
    {
        qInfo().noquote() << "[EMU] render frame"
                          << "product=" << product
                          << "cx=" << emulate_cx
                          << "casplus=" << emulate_casplus
                          << "px0=" << sample0
                          << "px1=" << sample1;
    }
#endif

    return QImage(reinterpret_cast<const uchar*>(framebuffer16.data()), 320, 240, 320 * 2, QImage::Format_RGB16);
}

void paintFramebuffer(QPainter *p)
{
#ifdef IS_IOS_BUILD
    // Apparently, this is needed (will be 2 on retina screens)
    // TODO: actually make sure Android doesn't need that as well
    static const double devicePixelRatio = ((QGuiApplication*)QCoreApplication::instance())->primaryScreen()->devicePixelRatio();
#else
    // Has to be 1 on desktop, even on retina (tested on OS X 10.11 with one retina, one non-retina, and both ; same on Win VM)
    static const double devicePixelRatio = 1;
#endif

    QRect painterWindowScaled(p->window().topLeft(), p->window().size() / devicePixelRatio);

    const bool forceDrawEvenIfContrastOff =
#if defined(__EMSCRIPTEN__) || defined(IS_IOS_BUILD)
        true;
#else
        false;
#endif

    if(hdq1w.lcd_contrast == 0 && !forceDrawEvenIfContrastOff)
    {
        p->fillRect(painterWindowScaled, emulate_cx ? Qt::black : Qt::white);
        p->setPen(emulate_cx ? Qt::white : Qt::black);
        p->drawText(painterWindowScaled, Qt::AlignCenter, QObject::tr("LCD turned off"));
    }
    else
    {
        QImage image = renderFramebuffer().scaled(p->window().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        image.setDevicePixelRatio(devicePixelRatio);
        p->drawImage((p->window().width() - image.width()) / 2, (p->window().height() - image.height()) / 2, image);
    }

    if(in_debugger)
    {
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
        p->fillRect(painterWindowScaled, QColor(30, 30, 30, 150));
        p->setPen(Qt::white);
        p->drawText(painterWindowScaled, Qt::AlignCenter, QObject::tr("In debugger"));
    }
}

QMLFramebuffer::QMLFramebuffer(QQuickItem *parent)
 : QQuickPaintedItem(parent)
{
    installEventFilter(&qt_keypad_bridge);
}

void QMLFramebuffer::paint(QPainter *p)
{
    paintFramebuffer(p);
}
