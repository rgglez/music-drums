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

#include <QMainWindow>
#include <QMap>
#include "drumengine.h"

class QPushButton;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUI();
    void hitDrum(DrumType type);
    void flashPad(DrumType type);

    DrumEngine               *m_engine;
    QMap<DrumType, QPushButton *> m_pads;
    QLabel                   *m_statusLabel;
    QMap<int, DrumType>       m_keyMap;   // Qt::Key -> DrumType
};
