/*
 * Copyright 2026 Rodolfo González González
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mainwindow.h"

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <cmath>

// ============================================================================
// Per-drum UI metadata
// ============================================================================
struct DrumDef {
    DrumType type;
    QString  name;
    Qt::Key  key;
    QString  keyLabel;
    QString  baseColor;
    QString  hoverColor;
    QString  activeColor;
};

static const DrumDef kDrums[] = {
    { DrumType::BOMBO,    "Bombo",    Qt::Key_Q, "Q", "#4A0D0D", "#6B1212", "#E02020" },
    { DrumType::TAROLA,   "Tarola",   Qt::Key_W, "W", "#4A2A00", "#6B3D00", "#E07000" },
    { DrumType::HIHAT,    "Hi-Hat",   Qt::Key_E, "E", "#424200", "#606000", "#D8C000" },
    { DrumType::PLATILLO, "Platillo", Qt::Key_R, "R", "#003C42", "#005560", "#00B0BB" },
    { DrumType::CENCERRO, "Cencerro", Qt::Key_A, "A", "#0E3A10", "#165218", "#22A02C" },
    { DrumType::CLAVE,    "Clave",    Qt::Key_S, "S", "#2A0840", "#3E1060", "#8820CC" },
    { DrumType::QUIJADA,  "Quijada",  Qt::Key_D, "D", "#440A38", "#620E52", "#C018A0" },
};

// ============================================================================
// Icon drawing — one function per instrument, called from paintEvent
// ============================================================================

// Common pen for instrument outlines
static QPen iconPen(float width = 2.4f)
{
    return QPen(QColor(255, 255, 255, 210), width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

static void drawBombo(QPainter &p, float cx, float cy)
{
    // Bass drum — front view: outer shell + inner head + two sticks
    p.setPen(iconPen());
    p.setBrush(QColor(255, 255, 255, 18));
    p.drawEllipse(QPointF(cx, cy + 6), 33, 33);   // outer shell
    p.setBrush(QColor(255, 255, 255, 28));
    p.drawEllipse(QPointF(cx, cy + 6), 20, 20);   // drum head
    p.setBrush(Qt::NoBrush);

    // Drumsticks (two lines converging toward head)
    p.drawLine(QPointF(cx - 20, cy - 26), QPointF(cx - 7,  cy - 11));
    p.drawLine(QPointF(cx + 20, cy - 26), QPointF(cx + 7,  cy - 11));

    // Stick tips
    p.setBrush(QColor(255, 255, 255, 200));
    p.setPen(QPen(QColor(255, 255, 255, 200), 1.2f));
    p.drawEllipse(QPointF(cx - 21, cy - 28), 3.5f, 3.5f);
    p.drawEllipse(QPointF(cx + 21, cy - 28), 3.5f, 3.5f);
    p.setBrush(Qt::NoBrush);
}

static void drawTarola(QPainter &p, float cx, float cy)
{
    // Snare drum — 3/4 perspective: two ellipses connected by side walls
    const float rx = 30, ry = 9, bodyH = 18;
    const float topY = cy - bodyH / 2.0f;
    const float botY = cy + bodyH / 2.0f;

    p.setPen(iconPen());
    // Top head (filled lightly)
    p.setBrush(QColor(255, 255, 255, 28));
    p.drawEllipse(QPointF(cx, topY), rx, ry);
    // Body sides
    p.setBrush(QColor(255, 255, 255, 10));
    QPainterPath body;
    body.moveTo(cx - rx, topY);
    body.lineTo(cx - rx, botY);
    body.arcTo(QRectF(cx - rx, botY - ry, rx * 2, ry * 2), 180, 180);
    body.lineTo(cx + rx, topY);
    p.drawPath(body);
    p.setBrush(Qt::NoBrush);
    // Bottom ellipse
    p.drawEllipse(QPointF(cx, botY), rx, ry);

    // Snare wires: short dashed lines just below the drum
    p.setPen(QPen(QColor(255, 255, 255, 130), 1.3f, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 4; ++i)
        p.drawLine(QPointF(cx - 20, botY + 4 + i * 4), QPointF(cx + 20, botY + 4 + i * 4));
}

static void drawHihat(QPainter &p, float cx, float cy)
{
    // Hi-hat — two flat cymbal discs + stand rod
    const float rx = 32, ry = 7;
    const float topCy = cy - 10, botCy = cy + 4;

    p.setPen(iconPen());
    // Bottom cymbal
    p.setBrush(QColor(255, 255, 255, 18));
    p.drawEllipse(QPointF(cx, botCy), rx, ry);
    // Top cymbal
    p.setBrush(QColor(255, 255, 255, 28));
    p.drawEllipse(QPointF(cx, topCy), rx, ry);
    p.setBrush(Qt::NoBrush);

    // Bell dome on top cymbal
    p.setBrush(QColor(255, 255, 255, 55));
    p.drawEllipse(QPointF(cx, topCy), 7, 4);
    p.setBrush(Qt::NoBrush);

    // Center rod (stand)
    p.setPen(iconPen(1.8f));
    p.drawLine(QPointF(cx, topCy - ry - 1), QPointF(cx, topCy - 16));  // above
    p.drawLine(QPointF(cx, botCy + ry + 1), QPointF(cx, botCy + 22));  // below
}

static void drawPlatillo(QPainter &p, float cx, float cy)
{
    // Crash cymbal — angled oval + bell + stand
    p.save();
    p.translate(cx, cy);
    p.rotate(-16);

    p.setPen(iconPen());
    p.setBrush(QColor(255, 255, 255, 18));
    p.drawEllipse(QPointF(0, 0), 36, 9);

    // Bell dome
    p.setBrush(QColor(255, 255, 255, 60));
    p.drawEllipse(QPointF(0, 0), 9, 5);
    p.setBrush(Qt::NoBrush);
    p.restore();

    // Stand line below center
    p.setPen(iconPen(1.8f));
    p.drawLine(QPointF(cx + 4, cy + 12), QPointF(cx + 4, cy + 34));
}

static void drawCencerro(QPainter &p, float cx, float cy)
{
    // Cowbell — trapezoidal body + mounting hole + striker
    const float topW = 18, botW = 30, h = 46;
    const float ty = cy - h / 2.0f + 2;

    // Bell body
    QPainterPath bell;
    bell.moveTo(cx - topW, ty);
    bell.lineTo(cx + topW, ty);
    bell.lineTo(cx + botW, ty + h);
    bell.lineTo(cx - botW, ty + h);
    bell.closeSubpath();

    p.setPen(iconPen());
    p.setBrush(QColor(255, 255, 255, 18));
    p.drawPath(bell);
    p.setBrush(Qt::NoBrush);

    // Rounded top edge
    p.drawArc(QRectF(cx - topW - 1, ty - 6, (topW + 1) * 2, 12), 0, 180 * 16);

    // Mounting hole
    p.setPen(iconPen(1.8f));
    p.setBrush(QColor(0, 0, 0, 100));
    p.drawEllipse(QPointF(cx, ty + 8), 4, 4);
    p.setBrush(Qt::NoBrush);

    // Interior striker line
    p.setPen(QPen(QColor(255, 255, 255, 100), 1.5f, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(cx - 4, ty + 16), QPointF(cx + 10, ty + h - 10));
}

static void drawClave(QPainter &p, float cx, float cy)
{
    // Two wooden sticks in a V (ready to strike each other)
    struct Stick { float tx, ty, angle; };
    Stick sticks[] = {
        { cx - 10, cy,  -32 },   // left stick
        { cx + 10, cy,   32 },   // right stick
    };

    for (auto &s : sticks) {
        p.save();
        p.translate(s.tx, s.ty);
        p.rotate(s.angle);
        QPainterPath stick;
        stick.addRoundedRect(-5.5f, -28, 11, 56, 5.5f, 5.5f);
        p.setPen(iconPen());
        p.setBrush(QColor(255, 255, 255, 22));
        p.drawPath(stick);
        p.setBrush(Qt::NoBrush);
        // Wood grain highlight
        p.setPen(QPen(QColor(255, 255, 255, 60), 1.2f, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(-2, -20), QPointF(-2, 20));
        p.restore();
    }
}

static void drawQuijada(QPainter &p, float cx, float cy)
{
    // Jawbone — U-shaped arc + teeth (small radial ticks)
    const float rx = 34, ry = 26;
    const float topY = cy - 8;

    // U-shape arc (bottom semicircle of an ellipse)
    p.setPen(iconPen(2.6f));
    p.drawArc(QRectF(cx - rx, topY - ry, rx * 2, ry * 2), 0, -180 * 16);

    // Horizontal bar across the top (upper jaw / skull)
    p.drawLine(QPointF(cx - rx, topY), QPointF(cx + rx, topY));

    // Teeth: 7 short radial lines along the arc's inner edge
    p.setPen(QPen(QColor(255, 255, 255, 170), 1.6f, Qt::SolidLine, Qt::RoundCap));
    const int N = 7;
    for (int i = 0; i < N; ++i) {
        // angle 0° = right, 180° = left, going through the bottom
        float angleDeg = float(i) / (N - 1) * 180.0f;
        float rad = float(M_PI) * (1.0f + angleDeg / 180.0f); // 180°..360° arc (bottom half)
        float ax = cx + rx * std::cos(rad);
        float ay = topY + ry * std::sin(rad);          // +y is down

        // Inward normal (toward center of ellipse)
        float nx = (ax - cx) / rx;
        float ny = (ay - topY) / ry;
        float toothLen = 7.0f;
        p.drawLine(QPointF(ax, ay),
                   QPointF(ax - nx * toothLen, ay - ny * toothLen));
    }
}

// Dispatch
static void drawDrumIcon(QPainter &p, DrumType type, float cx, float cy)
{
    switch (type) {
    case DrumType::BOMBO:    drawBombo   (p, cx, cy); break;
    case DrumType::TAROLA:   drawTarola  (p, cx, cy); break;
    case DrumType::HIHAT:    drawHihat   (p, cx, cy); break;
    case DrumType::PLATILLO: drawPlatillo(p, cx, cy); break;
    case DrumType::CENCERRO: drawCencerro(p, cx, cy); break;
    case DrumType::CLAVE:    drawClave   (p, cx, cy); break;
    case DrumType::QUIJADA:  drawQuijada (p, cx, cy); break;
    }
}

// ============================================================================
// DrumPad — custom QPushButton that draws the instrument icon in paintEvent
// ============================================================================
class DrumPad : public QPushButton
{
public:
    DrumType type;
    QString  keyLabel;
    QString  instrumentName;

    DrumPad(const DrumDef &d, QWidget *parent = nullptr)
        : QPushButton(parent)
        , type(d.type)
        , keyLabel(d.keyLabel)
        , instrumentName(d.name)
    {
        setFixedSize(164, 154);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void paintEvent(QPaintEvent *ev) override
    {
        // Draw standard button background + border via stylesheet
        QPushButton::paintEvent(ev);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const float cx = width()  / 2.0f;
        const float cy = height() / 2.0f - 10.0f; // shift icon slightly up

        // Draw instrument icon
        drawDrumIcon(p, type, cx, cy);

        // Instrument name — small, bottom center
        QFont nameFont("Noto Sans", 8, QFont::Bold);
        nameFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        p.setFont(nameFont);
        p.setPen(QColor(255, 255, 255, 160));
        QRectF nameRect(0, height() - 34, width(), 16);
        p.drawText(nameRect, Qt::AlignHCenter | Qt::AlignVCenter,
                   instrumentName.toUpper());

        // Key shortcut — small, bottom-right corner
        QFont keyFont("Noto Sans", 8);
        p.setFont(keyFont);
        p.setPen(QColor(255, 255, 255, 70));
        QRectF keyRect(0, height() - 20, width() - 8, 16);
        p.drawText(keyRect, Qt::AlignRight | Qt::AlignVCenter,
                   QString("[%1]").arg(keyLabel));
    }
};

// ============================================================================
// Stylesheet helpers
// ============================================================================
static QString padStyle(const DrumDef &d)
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 3px solid %2;"
        "  border-radius: 16px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "  border-color: %3;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %3;"
        "  border-color: #FFFFFF;"
        "}"
    ).arg(d.baseColor, d.hoverColor, d.activeColor);
}

static QString padActiveStyle(const DrumDef &d)
{
    return QString(
        "QPushButton {"
        "  background-color: %1;"
        "  border: 3px solid #FFFFFF;"
        "  border-radius: 16px;"
        "}"
    ).arg(d.activeColor);
}

// ============================================================================
// MainWindow
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_engine(new DrumEngine(this))
{
    setupUI();
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void MainWindow::setupUI()
{
    setWindowTitle("DrumBox");
    setMinimumSize(760, 500);
    resize(800, 540);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    central->setStyleSheet("background-color: #0F0F1E;");

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 18, 24, 14);
    mainLayout->setSpacing(0);

    // Title
    auto *titleLabel = new QLabel("DrumBox", central);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "color: #E8E8F0;"
        "font-size: 30px;"
        "font-weight: bold;"
        "font-family: 'Noto Sans', 'Arial', sans-serif;"
        "letter-spacing: 4px;"
        "margin-bottom: 4px;"
    );
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Percusión latina + batería", central);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "color: #555577;"
        "font-size: 11px;"
        "font-family: 'Noto Sans', 'Arial', sans-serif;"
        "letter-spacing: 2px;"
        "margin-bottom: 20px;"
    );
    mainLayout->addWidget(subtitleLabel);

    // Row 0: BOMBO TAROLA HIHAT PLATILLO
    auto *row0 = new QHBoxLayout();
    row0->setSpacing(14);

    // Row 1: CENCERRO CLAVE QUIJADA (centered)
    auto *row1 = new QHBoxLayout();
    row1->setSpacing(14);
    row1->addStretch(1);

    for (const DrumDef &d : kDrums) {
        auto *btn = new DrumPad(d, central);
        btn->setStyleSheet(padStyle(d));

        DrumType type = d.type;
        connect(btn, &QPushButton::clicked, this, [this, type]() {
            hitDrum(type);
        });

        m_pads[d.type]                   = btn;
        m_keyMap[static_cast<int>(d.key)] = d.type;

        if (type == DrumType::BOMBO  || type == DrumType::TAROLA ||
            type == DrumType::HIHAT  || type == DrumType::PLATILLO) {
            row0->addWidget(btn);
        } else {
            row1->addWidget(btn);
        }
    }
    row1->addStretch(1);

    auto *padsWidget = new QWidget(central);
    auto *padsLayout = new QVBoxLayout(padsWidget);
    padsLayout->setSpacing(14);
    padsLayout->setContentsMargins(0, 0, 0, 0);
    padsLayout->addLayout(row0);
    padsLayout->addLayout(row1);

    mainLayout->addWidget(padsWidget, 1);

    // Keyboard hint
    auto *hintLabel = new QLabel("Teclas:  Q W E R  /  A S D", central);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet(
        "color: #3A3A5A;"
        "font-size: 11px;"
        "font-family: 'Noto Sans', 'Arial', sans-serif;"
        "letter-spacing: 1px;"
        "margin-top: 6px;"
    );
    mainLayout->addWidget(hintLabel);

    // Status bar
    m_statusLabel = new QLabel(" ", central);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedHeight(28);
    m_statusLabel->setStyleSheet(
        "color: #666688;"
        "font-size: 12px;"
        "font-family: 'Noto Sans', 'Arial', sans-serif;"
    );
    mainLayout->addWidget(m_statusLabel);
}

void MainWindow::hitDrum(DrumType type)
{
    m_engine->play(type);
    flashPad(type);
}

void MainWindow::flashPad(DrumType type)
{
    QPushButton *pad = m_pads.value(type, nullptr);
    if (!pad) return;

    for (const DrumDef &d : kDrums) {
        if (d.type != type) continue;

        pad->setStyleSheet(padActiveStyle(d));

        m_statusLabel->setText(QString("▶  %1").arg(d.name));
        m_statusLabel->setStyleSheet(
            QString("color: %1;"
                    "font-size: 13px;"
                    "font-weight: bold;"
                    "font-family: 'Noto Sans', 'Arial', sans-serif;")
            .arg(d.activeColor));

        QTimer::singleShot(130, this, [this, pad, d]() {
            pad->setStyleSheet(padStyle(d));
            m_statusLabel->setText(" ");
            m_statusLabel->setStyleSheet(
                "color: #666688;"
                "font-size: 12px;"
                "font-family: 'Noto Sans', 'Arial', sans-serif;");
        });
        break;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) return;
    auto it = m_keyMap.find(event->key());
    if (it != m_keyMap.end())
        hitDrum(it.value());
    QMainWindow::keyPressEvent(event);
}
