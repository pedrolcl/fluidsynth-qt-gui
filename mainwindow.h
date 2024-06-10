// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>

class ConsoleWidget;
class FluidCompleter;
class FluidSynthWrapper;
class QAction;
class QToolBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    ConsoleWidget *m_console{nullptr};
    FluidCompleter *m_completer{nullptr};
    FluidSynthWrapper *m_client{nullptr};

    QAction *m_stopAction{nullptr};
    QAction *m_contAction{nullptr};
    QAction *m_nextAction{nullptr};
    QAction *m_startAction{nullptr};
    QToolBar *m_bar{nullptr};

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
    void enableCommandButtons(bool enable);
    void processFiles(const QStringList &files);

protected:
    void dropEvent(QDropEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
};

#endif // MAINWINDOW_H
