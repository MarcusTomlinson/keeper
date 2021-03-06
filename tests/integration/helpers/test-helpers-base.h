/*
 * Copyright 2013-2016 Canonical Ltd.
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
 *     Ted Gould <ted.gould@canonical.com>
 *     Xavi Garcia <xavi.garcia.mena@canonical.com>
 *     Charles Kerr <charles.kerr@canonical.com>
 */
#pragma once

#include <helper/backup-helper.h>
#include <qdbus-stubs/dbus-types.h>
#include "DBusPropertiesInterface.h"
#include "qdbus-stubs/keeper_user_interface.h"

#include "tests/utils/file-utils.h"
#include "tests/utils/xdg-user-dirs-sandbox.h"
#include "tests/utils/storage-framework-local.h"
#include "../../../src/service/app-const.h"

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <libqtdbusmock/DBusMock.h>

#include <ubuntu-app-launch.h>
#include <ubuntu-app-launch/registry.h>

#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

namespace
{
constexpr char const UPSTART_SERVICE[] = "com.ubuntu.Upstart";
constexpr char const UPSTART_PATH[] = "/com/ubuntu/Upstart";
constexpr char const UPSTART_INTERFACE[] = "com.ubuntu.Upstart0_6";
constexpr char const UPSTART_INSTANCE[] = "com.ubuntu.Upstart0_6.Instance";
constexpr char const UPSTART_JOB[] = "com.ubuntu.Upstart0_6.Job";
constexpr char const UNTRUSTED_HELPER_PATH[] = "/com/test/untrusted/helper";
}

#define WAIT_FOR_SIGNALS(signalSpy, signalsExpected, time)\
{\
    while (signalSpy.size() < signalsExpected)\
    {\
        ASSERT_TRUE(signalSpy.wait(time));\
    }\
    ASSERT_EQ(signalsExpected, signalSpy.size());\
}

struct BackupItem
{
    QString display_name;
    QString type;
    QString uuid;
};

class TestHelpersBase : public ::testing::Test
{
public:
    TestHelpersBase();

    ~TestHelpersBase() = default;

protected:
    QTemporaryDir xdg_data_home_dir;
    QtDBusTest::DBusTestRunner dbus_test_runner;
    QtDBusMock::DBusMock dbus_mock;
    QSharedPointer<QtDBusTest::QProcessDBusService> keeper_service;
    QSharedPointer<QtDBusTest::QProcessDBusService> upstart_service;
    QScopedPointer<QProcess> dbus_monitor_process;

protected:
    void start_tasks();

    virtual void SetUp() override;

    virtual void TearDown() override;

    bool init_helper_registry(QString const& registry);

    bool wait_for_all_tasks_have_action_state(QStringList const & uuids, QString const & action_state, QSharedPointer<DBusInterfaceKeeperUser> const & keeper_user_iface, int max_timeout_msec = 15000);

    bool check_task_has_action_state(keeper::Items const & state, QString const & uuid, QString const & action_state);

    bool get_task_value_now(QString const & uuid, QSharedPointer<DBusInterfaceKeeperUser> const & keeper_user_iface, keeper::Item & value) const;

    bool capture_and_check_state_until_all_tasks_complete(QSignalSpy & spy, QStringList const & uuids, QString const & action_state, int max_timeout_msec = 15000);

    bool cancel_first_task_at_percentage(QSignalSpy & spy, double expected_percentage, QSharedPointer<DBusInterfaceKeeperUser> const & user_iface, int max_timeout_msec = 15000);

    QString get_uuid_for_xdg_folder_path(QString const &path, keeper::Items const & choices) const;
    QString get_type_for_xdg_folder_path(QString const &path, keeper::Items const & choices) const;
    QString get_display_name_for_xdg_folder_path(QString const &path, keeper::Items const & choices) const;

    bool check_manifest_file(QVector<BackupItem> const & backup_items);

    bool start_dbus_monitor();

    bool prepare_for_cancellation();

    bool cleanup_cancellation();
};

#define EXPECT_ENV(expected, envvars, key) EXPECT_EQ(expected, get_env(envvars, key)) << "for key " << key
#define ASSERT_ENV(expected, envvars, key) ASSERT_EQ(expected, get_env(envvars, key)) << "for key " << key
