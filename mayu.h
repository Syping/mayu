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
#include <QObject>

class mayu : public QObject
{
    Q_OBJECT
public:
    explicit mayu(QObject *parent = nullptr);
    void setHostsFile(const QString &fileName);
    void setHosts(const QStringList &hostsList);
    void setJsonFile(const QString &fileName);
    const QString getHostsFile();
    const QStringList getHosts();
    const QString getJsonFile();
    int getResult();
    static double ping(const QString &host, int tries, double timeout = 2.5);

public slots:
    void parse_hosts();
    void work();

private:
    bool dropPrivileges();
    bool regainPrivileges();
    QStringList p_hostsList;
    QString p_hostsFile;
    QString p_jsonFile;
    bool p_hostsParsed;
    int p_return;
    int p_tries;
    uid_t p_uid;
};

#endif // MAYU_H
