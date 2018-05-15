/*****************************************************************************
* mayu Mate Are You Up
* Copyright (C) 2018 Syping
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*****************************************************************************/

#ifndef MAYU_H
#define MAYU_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QJsonArray>
#include <QObject>
#include <QList>

enum class mayuMode : int{Ping = 0, Resolve = 1};
struct mayuResult {
    QString host;
    QString result;
};

class mayu : public QObject
{
    Q_OBJECT
public:
    explicit mayu(const QString &hostsFile = QString(), const QString &jsonFile = QString(), QObject *parent = nullptr);
    void setMayuMode(mayuMode mode);
    void setHostsFile(const QString &fileName);
    void setHosts(const QStringList &hostsList);
    void setJsonFile(const QString &fileName);
    void setPingTimeout(double timeout);
    void setPingTries(int tries);
    mayuMode getMayuMode();
    const QString getHostsFile();
    const QStringList getHosts();
    const QString getJsonFile();
    double getPingTimeout();
    int getPingTries();
    int getResult();
#ifdef MAYU_UNIX
    static double ping(const QString &host, int tries, double timeout = 2.5);
#endif
    static const QList<mayuResult> resolve(const QString &host);

public slots:
    void parse_hosts();
    void work();

private:
#ifdef PRIVILEGE_DROP_REQUIRED
    bool p_dropPrivileges();
    bool p_regainPrivileges();
#endif
    void p_saveWork(QJsonObject jsonObject);
#ifdef MAYU_UNIX
    void p_workPing();
#endif
    void p_workResolve();
    QStringList p_hostsList;
    QString p_hostsFile;
    QString p_jsonFile;
    mayuMode p_mayuMode;
    bool p_hostsParsed;
    double p_timeout;
    int p_return;
    int p_tries;
#ifdef PRIVILEGE_DROP_REQUIRED
    uid_t p_uid;
#endif
};

#endif // MAYU_H
