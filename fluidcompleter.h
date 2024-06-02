// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

#ifndef FLUIDCOMPLETER_H
#define FLUIDCOMPLETER_H

#include "ConsoleWidget.h"

class FluidCompleter : public ConsoleWidgetCompleter
{
    QStringList m_keywords;
    int m_insert_pos;

public:
    FluidCompleter(QObject *parent = nullptr);

    int updateCompletionModel(const QString &code) override;
    int insertPos() override { return m_insert_pos; }
};

#endif // FLUIDCOMPLETER_H
