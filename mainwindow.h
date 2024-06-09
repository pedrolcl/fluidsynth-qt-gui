// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>

class ConsoleWidget;
class FluidCompleter;
class FluidSynthWrapper;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    ConsoleWidget *m_console{nullptr};
    FluidCompleter *m_completer{nullptr};
    FluidSynthWrapper *m_client{nullptr};

public:
    explicit MainWindow(const QString &audioDriver,
                        const QString &midiDriver,
                        const QString &configFile,
                        const QStringList &args,
                        QWidget *parent = nullptr);

public slots:
    void consoleOutput(const QByteArray &data, const int res = 0);
    void diagnosticsOutput(int level, const QByteArray message);
    void consoleInput();
    void startInput();
    void fileDialog();
    void processFiles(const QStringList &files);

protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
};

#endif // MAINWINDOW_H
