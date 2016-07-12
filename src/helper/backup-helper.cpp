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
 *     Charles Kerr <charles.kerr@canonical.com>
 */

#include <helper/backup-helper.h>
#include <service/app-const.h> // HELPER_TYPE

#include <QByteArray>
#include <QDebug>
#include <QLocalSocket>
#include <QMap>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QTimer>
#include <QVector>

#include <ubuntu-app-launch/registry.h>
#include <service/app-const.h>
extern "C" {
    #include <ubuntu-app-launch.h>
}

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <functional> // std::bind()


class BackupHelperPrivate
{
public:

    BackupHelperPrivate(
        BackupHelper* backup_helper,
        const QString& appid
    )
        : q_ptr(backup_helper)
        , appid_(appid)
        , timer_(new QTimer())
        , registry_(new ubuntu::app_launch::Registry())
        , storage_framework_socket_(new QLocalSocket())
        , helper_socket_(new QLocalSocket())
        , read_socket_(new QLocalSocket())
        , upload_buffer_{}
    {
        ual_init();

        // listen for inactivity
        QObject::connect(timer_.data(), &QTimer::timeout,
            std::bind(&BackupHelperPrivate::on_inactivity_detected, this)
        );

        // listen for data uploaded
        QObject::connect(storage_framework_socket_.data(), &QLocalSocket::bytesWritten,
            std::bind(&BackupHelperPrivate::on_data_uploaded, this, std::placeholders::_1)
        );

        // listen for data ready to read
        QObject::connect(read_socket_.data(), &QLocalSocket::readyRead,
            std::bind(&BackupHelperPrivate::on_ready_read, this)
        );

        // fire up the sockets
        int fds[2];
        int rc = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds);
        if (rc == -1)
        {
            // TODO throw exception.
            qWarning() << "BackupHelperPrivate: error creating socket pair to communicate with helper ";
            return;
        }

        // helper socket is for the client.
        helper_socket_->setSocketDescriptor(fds[1], QLocalSocket::ConnectedState, QIODevice::WriteOnly);

        read_socket_->setSocketDescriptor(fds[0], QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    }

    ~BackupHelperPrivate()
    {
        ual_uninit();
    }

    Q_DISABLE_COPY(BackupHelperPrivate)

    void start()
    {
        ual_start();
        reset_inactivity_timer();
    }

    void set_storage_framework_socket(qint64 /*n_bytes*/, int sd)
    {
        storage_framework_socket_->setSocketDescriptor(sd, QLocalSocket::ConnectedState, QIODevice::WriteOnly);

        reset_inactivity_timer();
    }

    void stop()
    {
        ual_stop();
    }

    int get_helper_socket() const
    {
        return int(helper_socket_->socketDescriptor());
    }

private:

    void on_inactivity_detected()
    {
        qWarning() << "Inactivity detected in the helper...stopping it";
        stop();
    }

    void on_ready_read()
    {
        process_more();
    }

    void on_data_uploaded(qint64 /*n*/)
    {
        process_more();
    }

    void process_more()
    {
        qint64 bytes_uploaded {};
        char readbuf[UPLOAD_BUFFER_MAX_];
        do
        {
            // try to fill the upload buf
            int max_bytes = UPLOAD_BUFFER_MAX_ - upload_buffer_.size();
            if (max_bytes > 0) {
                const auto n = read_socket_->read(readbuf, max_bytes);
                if (n > 0) {
                    upload_buffer_.append(readbuf, int(n));
                    qDebug("upload_buffer_.size() is %zu after reading %zu from helper", size_t(upload_buffer_.size()), size_t(n));
                }
            }

            // try to empty the upload buf
            const auto n = storage_framework_socket_->write(upload_buffer_);
            if (n > 0) {
                upload_buffer_.remove(0, int(n));
                qDebug("upload_buffer_.size() is %zu after writing %zu to cloud", size_t(upload_buffer_.size()), size_t(n));
            }
        }
        while (bytes_uploaded > 0);

        reset_inactivity_timer();
    }

    void reset_inactivity_timer()
    {
        static constexpr int MAX_TIME_WAITING_FOR_DATA {10000};
        timer_->start(MAX_TIME_WAITING_FOR_DATA);
    }


    /***
    ****  UAL
    ***/

    void ual_init()
    {
        ubuntu_app_launch_observer_add_helper_started(on_helper_started, HELPER_TYPE, this);
        ubuntu_app_launch_observer_add_helper_stop(on_helper_stopped, HELPER_TYPE, this);
    }

    void ual_uninit()
    {
        if (q_ptr->getState() == Helper::State::STARTED)
            ual_stop();

        ubuntu_app_launch_observer_delete_helper_started(on_helper_started, HELPER_TYPE, this);
        ubuntu_app_launch_observer_delete_helper_stop(on_helper_stopped, HELPER_TYPE, this);
    }

    void ual_start()
    {
        qDebug() << "Starting helper for app: " << appid_;

        auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

        auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
        auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

        std::vector<ubuntu::app_launch::Helper::URL> urls = {
            ubuntu::app_launch::Helper::URL::from_raw(get_helper_path(appid_).toStdString())
        };

        helper->launch(urls);
    }

    void ual_stop()
    {
        qDebug() << "Stopping helper for app: " << appid_;
        auto backupType = ubuntu::app_launch::Helper::Type::from_raw(HELPER_TYPE);

        auto appid = ubuntu::app_launch::AppID::parse(appid_.toStdString());
        auto helper = ubuntu::app_launch::Helper::create(backupType, appid, registry_);

        auto instances = helper->instances();

        if (instances.size() > 0 )
        {
            qDebug() << "We have instances";
            instances[0]->stop();
        }
    }

    static void on_helper_started(const char* appid, const char* /*instance*/, const char* /*type*/, void* vself)
    {
        qDebug() << "HELPER STARTED +++++++++++++++++++++++++++++++++++++ " << appid;
        auto self = static_cast<BackupHelperPrivate*>(vself);
        self->q_ptr->setState(Helper::State::STARTED);
    }

    static void on_helper_stopped(const char* appid, const char* /*instance*/, const char* /*type*/, void* vself)
    {
        qDebug() << "HELPER STOPPED +++++++++++++++++++++++++++++++++++++ " << appid;
        auto self = static_cast<BackupHelperPrivate*>(vself);
        self->q_ptr->setState(Helper::State::COMPLETE);
    }

    QString get_helper_path(QString const & /*appId*/)
    {
        //TODO retrieve the helper path from the package information

        // This is added for testing purposes only
        auto testHelper = qgetenv("KEEPER_TEST_HELPER");
        if (!testHelper.isEmpty())
        {
            qDebug() << "BackupHelperImpl::getHelperPath: returning the helper: " << testHelper;
            return testHelper;
        }

        return DEKKO_HELPER_BIN;
    }

    /***
    ****
    ***/

    static constexpr int UPLOAD_BUFFER_MAX_ {1024*16};

    BackupHelper * const q_ptr;
    const QString appid_;
    QScopedPointer<QTimer> timer_;
    std::shared_ptr<ubuntu::app_launch::Registry> registry_;
    QScopedPointer<QLocalSocket> storage_framework_socket_;
    QScopedPointer<QLocalSocket> helper_socket_;
    QScopedPointer<QLocalSocket> read_socket_;
    QByteArray upload_buffer_;
};

/***
****
***/

BackupHelper::BackupHelper(
    QString const & appid,
    QObject * parent
)
    : Helper(parent)
    , d_ptr(new BackupHelperPrivate(this, appid))
{
}

BackupHelper::~BackupHelper() =default;

void
BackupHelper::start()
{
    Q_D(BackupHelper);

    d->start();
}

void
BackupHelper::stop()
{
    Q_D(BackupHelper);

    d->stop();
}

void
BackupHelper::set_storage_framework_socket(qint64 n_bytes, int sd)
{
    Q_D(BackupHelper);

    d->set_storage_framework_socket(n_bytes, sd);
}

int
BackupHelper::get_helper_socket() const
{
    Q_D(const BackupHelper);

    return d->get_helper_socket();
}