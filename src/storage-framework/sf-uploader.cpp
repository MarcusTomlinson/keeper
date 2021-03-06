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
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "storage-framework/sf-uploader.h"

#include <QFuture>
#include <QFutureWatcher>

StorageFrameworkUploader::StorageFrameworkUploader(
    std::shared_ptr<unity::storage::qt::client::Uploader> const& uploader,
    QObject *parent
):
    Uploader(parent),
    uploader_(uploader)
{
}

std::shared_ptr<QLocalSocket>
StorageFrameworkUploader::socket()
{
    return uploader_->socket();
}

void
StorageFrameworkUploader::commit()
{
    qDebug() << Q_FUNC_INFO << "is committing";

    connections_.connect_future(
        uploader_->finish_upload(),
        std::function<void(std::shared_ptr<unity::storage::qt::client::File> const&)>{
            [this](std::shared_ptr<unity::storage::qt::client::File> const& file){
                auto const success = bool(file);
                qDebug() << "commit finished with" << success;
                if (success)
                {
                    file_name_after_commit_ = file->name();
                }
                Q_EMIT(commit_finished(success));
            }
        }
    );
}

QString
StorageFrameworkUploader::file_name() const
{
    return file_name_after_commit_;
}
