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

#pragma once

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QString>

// ---------------------------------------------------------------------------
// DrumType: one value per instrument
// ---------------------------------------------------------------------------
enum class DrumType {
    BOMBO,
    TAROLA,
    HIHAT,
    PLATILLO,
    CENCERRO,
    CLAVE,
    QUIJADA
};

// ---------------------------------------------------------------------------
// DrumEngine: synthesizes drum sounds and plays them via PipeWire.
//
// Audio is mixed in a dedicated PipeWire thread; play() can be called
// safely from the Qt main thread at any time.
// ---------------------------------------------------------------------------
struct DrumEnginePrivate;   // defined in drumengine.cpp

class DrumEngine : public QObject
{
    Q_OBJECT
public:
    explicit DrumEngine(QObject *parent = nullptr);
    ~DrumEngine() override;

public slots:
    void play(DrumType drum);

private:
    DrumEnginePrivate *d = nullptr;
};
