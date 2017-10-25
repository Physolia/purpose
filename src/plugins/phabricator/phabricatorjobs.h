/*
 * This file is part of KDevelop
 * Copyright 2017 René J.V. Bertin <rjvbertin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KDEVPLATFORM_PLUGIN_PHABRICATORJOBS_H
#define KDEVPLATFORM_PLUGIN_PHABRICATORJOBS_H

#include "phabricatorhelpers_export.h"

#include <QList>
#include <QHash>
#include <QPair>
#include <QUrl>

#include <KJob>
#include <QProcess>

class QNetworkReply;

namespace Phabricator
{
    class PHABRICATORHELPERS_EXPORT DifferentialRevision : public KJob
    {
        Q_OBJECT
        public:
            DifferentialRevision(const QString& id, QObject* parent)
                : KJob(parent), m_id(id), m_commit(QString())
            {
              setPercent(0);
            }
            QString requestId() const { return m_id; }
            void setRequestId(const QString& id) { m_id = id; }
            QString commitRef() const { return m_commit; }
            void setCommitRef(const QString& commit) { m_commit = commit; }
            virtual void start() override;
            virtual QString errorString() const override
            {
                return m_errorString;
            }
            void setErrorString(const QString& msg);
            QString scrubbedResult();
            QStringList scrubbedResultList();

        private Q_SLOTS:
            virtual void done(int exitCode, QProcess::ExitStatus exitStatus) = 0;

        protected:
            virtual bool buildArcCommand(const QString& workDir, const QString& patchFile=QString(), bool doBrowse=false);
            QProcess m_arcCmd;
        private:
            QString m_id;
            QString m_commit;
            QString m_errorString;
            QString m_arcInput;
    };

    class PHABRICATORHELPERS_EXPORT NewDiffRev : public DifferentialRevision
    {
        Q_OBJECT
        public:
            NewDiffRev(const QUrl& patch, const QString& project, bool doBrowse = false, QObject* parent = 0);
            QString diffURI() const
            {
                return m_diffURI;
            }

        private Q_SLOTS:
            void done(int exitCode, QProcess::ExitStatus exitStatus) override;

        private:
            QUrl m_patch;
            QString m_project;
            QString m_diffURI;
    };

    class PHABRICATORHELPERS_EXPORT UpdateDiffRev : public DifferentialRevision
    {
        Q_OBJECT
        public:
            UpdateDiffRev(const QUrl& patch, const QString& basedir,
                          const QString& id, const QString& updateComment = QString(), bool doBrowse = false, QObject* parent = 0);
            QString diffURI() const
            {
                return m_diffURI;
            }

        private Q_SLOTS:
            void done(int exitCode, QProcess::ExitStatus exitStatus) override;

        private:
            QUrl m_patch;
            QString m_basedir;
            QString m_diffURI;
    };

    class PHABRICATORHELPERS_EXPORT DiffRevList : public DifferentialRevision
    {
        Q_OBJECT
        public:
            DiffRevList(const QString& projectDir, QObject* parent = 0);
            // return the open diff. revisions as a list of <diffID,diffDescription> pairs
            QList<QPair<QString,QString> > reviews() const
            {
                return m_reviews;
            }
            // return the open diff. revisions as a map of diffDescription->diffID entries
            QHash<QString,QString> reviewMap() const
            {
                return m_revMap;
            }

        private Q_SLOTS:
            void done(int exitCode, QProcess::ExitStatus exitStatus) override;

        protected:
            bool buildArcCommand(const QString& workDir, const QString& unused=QString(), bool ignored=false) override;
        private:
            QList<QPair<QString,QString> > m_reviews;
            QHash<QString,QString> m_revMap;
            QString m_projectDir;
    };
}

#endif
