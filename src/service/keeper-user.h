/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canoincal.com>
 */

#pragma once

#include "qdbus-stubs/dbus-types.h"

#include <QDBusContext>
#include <QObject>
#include <QStringList>

class Keeper;

class KeeperUser : public QObject, protected QDBusContext
{
    Q_OBJECT

public:

    explicit KeeperUser(Keeper* parent);
    virtual ~KeeperUser();
    Q_DISABLE_COPY(KeeperUser)

    Q_PROPERTY(QVariantDictMap State
               READ get_state
               NOTIFY state_changed)

    QVariantDictMap get_state() const;

Q_SIGNALS:

    void state_changed();

public Q_SLOTS:

    QVariantDictMap GetBackupChoices();
    void StartBackup(const QStringList&);

    QVariantDictMap GetRestoreChoices();
    void StartRestore(const QStringList&);

    void Cancel();

private:

    Keeper& keeper_;
};