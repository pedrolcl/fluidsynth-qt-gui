// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("TestFluidSynthCLI");
    QCoreApplication::setApplicationVersion(QT_STRINGIFY(VERSION));
    QApplication::setStyle(QLatin1String("Fusion"));
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption audioDriverOption({"a", "audio-driver"},
                                         "The name of the audio driver to use.",
                                         "audio-driver");
    parser.addOption(audioDriverOption);
    QCommandLineOption midiDriverOption({"m", "midi-driver"},
                                        "The name of the midi driver to use.",
                                        "midi-driver");
    parser.addOption(midiDriverOption);
    QCommandLineOption configurationOption({"f", "config-file"},
                                           "The (optional) configuration file.",
                                           "config-file");
    parser.addOption(configurationOption);
    parser.addPositionalArgument("SoundFont", "Soundfont File [*.sf2]");
    parser.addPositionalArgument("MidiFile", "MIDI File [*.mid]");
    parser.process(app);

    QString audioDriver = parser.isSet(audioDriverOption) ? parser.value(audioDriverOption)
                                                          : QString();
    QString midiDriver = parser.isSet(midiDriverOption) ? parser.value(midiDriverOption)
                                                        : QString();
    QString configFile = parser.isSet(configurationOption) ? parser.value(configurationOption)
                                                           : QString();
    QStringList args = parser.positionalArguments();

    MainWindow w(audioDriver, midiDriver, configFile, args);
    w.show();

    return app.exec();
}
