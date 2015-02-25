/************************************************************************************
 * Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>                *
 *                                                                                  *
 * This program is free software; you can redistribute it and/or                    *
 * modify it under the terms of the GNU General Public License                      *
 * as published by the Free Software Foundation; either version 2                   *
 * of the License, or (at your option) any later version.                           *
 *                                                                                  *
 * This program is distributed in the hope that it will be useful,                  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 * GNU General Public License for more details.                                     *
 *                                                                                  *
 * You should have received a copy of the GNU General Public License                *
 * along with this program; if not, write to the Free Software                      *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 ************************************************************************************/

#include <purpose/pluginbase.h>
#include <QDebug>
#include <QTimer>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KJob>
#include <KJobTrackerInterface>
#include <KIO/TransferJob>
#include <KIO/StoredTransferJob>

EXPORT_SHARE_VERSION

// Taken from "share" Data Engine
// key associated with plasma-devel@kde.org
// thanks to Alan Schaaf of Pastebin (alan@pastebin.com)
static const QString apiKey = QStringLiteral("d0757bc2e94a0d4652f28079a0be9379");

class PastebinJob : public Purpose::Job
{
    Q_OBJECT
    public:
        PastebinJob(QObject* parent)
            : Purpose::Job(parent)
            , m_pendingJobs(0)
        {}

        virtual void start() override
        {
            QJsonArray urls = data().value(QStringLiteral("urls")).toArray();

            if (urls.isEmpty()) {
                qWarning() << "no urls to share" << urls << data();
                emitResult();
                return;
            }

            foreach(const QJsonValue &val, urls) {
                QString u = val.toString();
                KIO::StoredTransferJob* job = KIO::storedGet(QUrl(u));
                connect(job, &KJob::finished, this, &PastebinJob::fileFetched);
                m_pendingJobs++;
            }
            Q_ASSERT(m_pendingJobs>0);
        }

        void fileFetched(KJob* j)
        {
            KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);
            m_data += job->data();
            --m_pendingJobs;
            if (m_pendingJobs == 0)
                performUpload();
        }

        void performUpload()
        {
            if (m_data.isEmpty()) {
                setError(1);
                setErrorText(i18n("No information to send"));
                emitResult();
                return;
            }

            const QByteArray apiKey = "0c8b6add8e0f6d53f61fe5ce870a1afa";
//             qCDebug(PLUGIN_PASTEBIN) << "exporting patch to pastebin" << source->file();
            QByteArray bytearray = "api_option=paste&api_paste_private=1&api_paste_name=kde-purpose-pastebin-plugin&api_paste_expire_date=1D&api_paste_format=diff&api_dev_key="+apiKey+"&api_paste_code=";
            bytearray += QUrl::toPercentEncoding(QString::fromUtf8(m_data));

            const QUrl url(QStringLiteral("http://pastebin.com/api/api_post.php"));

            KIO::TransferJob *tf = KIO::http_post(url, bytearray);

            tf->addMetaData(QStringLiteral("content-type"), QStringLiteral("Content-Type: application/x-www-form-urlencoded"));
            connect(tf, &KIO::TransferJob::data, this, [this](KIO::Job*, const QByteArray& data) { m_resultData += data; });
            connect(tf, &KJob::result, this, &PastebinJob::textUploaded);

            m_resultData.clear();
            KIO::getJobTracker()->registerJob(tf);
        }

        void textUploaded(KJob* job) {
            Q_EMIT output( { { QStringLiteral("url"), QString::fromUtf8(m_resultData) } });
            emitResult();
        }

        virtual QUrl configSourceCode() const override
        {
            return QUrl();
        }

    private:
        int m_pendingJobs;
        QByteArray m_data;
        QByteArray m_resultData;
};

class Q_DECL_EXPORT PastebinPlugin : public Purpose::PluginBase
{
    Q_OBJECT
    public:
        PastebinPlugin(QObject* p, const QVariantList& ) : Purpose::PluginBase(p) {}

        virtual Purpose::Job* share() const override
        {
            return new PastebinJob(nullptr);
        }
};

K_PLUGIN_FACTORY_WITH_JSON(Pastebin, "pastebinplugin.json", registerPlugin<PastebinPlugin>();)

#include "pastebinplugin.moc"
