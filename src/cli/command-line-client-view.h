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
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#pragma once

#include <client/keeper-errors.h>

#include <QMap>
#include <QObject>
#include <QTimer>

class CommandLineClientView : public QObject
{
    Q_OBJECT
public:
    explicit CommandLineClientView(QObject * parent = nullptr);
    ~CommandLineClientView() = default;

    Q_DISABLE_COPY(CommandLineClientView)

    void progress_changed(double percentage);
    void status_changed(QString const & status);

    void add_task(QString const & display_name, QString const & initial_status, double initial_percentage);
    void clear_all_tasks();
    void start_printing_tasks();
    void clear_all();
    void print_sections(QStringList const & sections);
    void print_error_message(QString const & error_message);
    QString get_error_string(keeper::Error error);

public Q_SLOTS:
    void show_info();
    void on_task_state_changed(QString const & displayName, QString const & status, double percentage, keeper::Error error);

private:
    char get_next_spin_char();
    QString get_task_string(QString const & displayName, QString const & status, double percentage, keeper::Error error);

    QString status_;
    QTimer timer_status_;
    double percentage_ = 0.0;
    int spin_value_ = 0;
    QMap<QString, QString> tasks_strings_;
};
