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

#include <QHostAddress>
#include <QTextStream>
#include <QDataStream>
#include <QEventLoop>
#include <QHostInfo>
#include <QSaveFile>
#include <QDebug>
#include <QFile>
#include "mayu.h"

#include <iostream>
using namespace std;

#ifdef MAYU_UNIX
extern "C" {
#include "oping.h"
}
#endif

mayu::mayu(const QString &hostsFile, const QString &jsonFile, QObject *parent) : QObject(parent)
{
    p_return = -1;
    p_timeout = 2.5;
    p_tries = 4;
    p_clean = false;
    p_mayuMode = mayuMode::Ping;
    if (!hostsFile.isEmpty())
        setHostsFile(hostsFile);
    if (!jsonFile.isEmpty())
        setJsonFile(jsonFile);
}

void mayu::setMayuMode(mayuMode mode)
{
    p_mayuMode = mode;
}

void mayu::setHostsFile(const QString &fileName)
{
    p_hostsFile = fileName;
    p_hostsParsed = false;
}

void mayu::setHosts(const QStringList &hostsList)
{
    p_hostsList = hostsList;
    p_hostsParsed = true;
}

void mayu::setJsonFile(const QString &fileName)
{
    p_jsonFile = fileName;
}

void mayu::setPingTimeout(double timeout)
{
    p_timeout = timeout;
}

void mayu::setPingTries(int tries)
{
    p_tries = tries;
}

void mayu::setCleanUp(bool clean)
{
    p_clean = clean;
}

mayuMode mayu::getMayuMode()
{
    return p_mayuMode;
}

const QString mayu::getHostsFile()
{
    return p_hostsFile;
}

const QStringList mayu::getHosts()
{
    return p_hostsList;
}

const QString mayu::getJsonFile()
{
    return p_jsonFile;
}

double mayu::getPingTimeout()
{
    return p_timeout;
}

int mayu::getPingTries()
{
    return p_tries;
}

bool mayu::getCleanUp()
{
    return p_clean;
}

int mayu::getResult()
{
    return p_return;
}

#ifdef MAYU_UNIX
double mayu::ping(const QString &host, int tries, double timeout)
{
    double latency;
    pingobj_t *pingObj;
    pingobj_iter_t *pingIter;
    if ((pingObj = ping_construct()) == NULL) {
        QTextStream(stderr) << "Ping construction failed " << endl;
        return -1;
    }
    if (ping_setopt(pingObj, PING_OPT_TIMEOUT, (void*)(&timeout)) < 0) {
        QTextStream(stderr) << "Setting timeout to"  << timeout << " have failed" << endl;
        ping_destroy(pingObj);
        return -1;
    }
    QHostAddress hostAddress(host);
    if (QAbstractSocket::IPv4Protocol == hostAddress.protocol()) {
        if (ping_host_add(pingObj, hostAddress.toString().toStdString().c_str()) < 0) {
            ping_destroy(pingObj);
            return -1;
        }
#ifdef E_DEBUG
        QTextStream(stderr) << "IPv4 Address " << hostAddress.toString() << " found" << endl;
#endif
    }
    else if (QAbstractSocket::IPv6Protocol == hostAddress.protocol()) {
        if (ping_host_add(pingObj, hostAddress.toString().toStdString().c_str()) < 0) {
            ping_destroy(pingObj);
            return -1;
        }
#ifdef E_DEBUG
        QTextStream(stderr) << "IPv6 Address " << hostAddress.toString() << " found" << endl;
#endif
    }
    else {
        QList<QHostAddress> hostAddresses = QHostInfo::fromName(host).addresses();
        if (hostAddresses.length() >= 1) {
            QString ipStr = hostAddresses.at(0).toString();
            if (ping_host_add(pingObj, ipStr.toStdString().c_str()) < 0) {
                ping_destroy(pingObj);
                return -1;
            }
#ifdef E_DEBUG
            QTextStream(stderr) << "Hostname " << host << " found and resolved " << ipStr << endl;
#endif
        }
        else {
#ifdef E_DEBUG
            QTextStream(stderr) << "Hostname " << host << " not found" << endl;
#endif
            ping_destroy(pingObj);
            return -1;
        }
    }
    bool hostUp = false;
    int curTry = 0;
    while (!hostUp && curTry != tries) {
        if (ping_send(pingObj) < 0) {
            QTextStream(stderr) << "Pinging host " << host << " has failed" << endl;
            ping_destroy(pingObj);
            return -1;
        }
        bool pingSuccess = false;
        for (pingIter = ping_iterator_get(pingObj); pingIter != NULL; pingIter =
             ping_iterator_next(pingIter)) {
            size_t len;
            len = sizeof(double);
            ping_iterator_get_info(pingIter, PING_INFO_LATENCY, &latency, &len);
            pingSuccess = !(latency < 0);
#ifdef E_DEBUG
            char hostname[100];
            len = 100;
            ping_iterator_get_info(pingIter, PING_INFO_HOSTNAME, hostname, &len);
            QString latencyString;
            if (latency != -1) {
                latencyString = QString::number(latency) + "ms";
            }
            else {
                latencyString = QString::number(latency);
            }
            QTextStream(stderr) << "Host: " << hostname << " Ping: " << latencyString << " Status: " << (pingSuccess ? "true" : "false") << endl;
#endif
        }
        if (pingSuccess) {
            hostUp = true;
        }
        curTry++;
    }
    ping_destroy(pingObj);
    if (hostUp)
        return latency;
    return -1;
}
#endif

const QList<mayuResult> mayu::resolve(const QString &host, bool emptyWhenError)
{
    QList<mayuResult> resultList;
    QList<QHostAddress> hostAddresses = QHostInfo::fromName(host).addresses();
    if (hostAddresses.length() >= 1) {
        for (const QHostAddress &hostAddress : hostAddresses) {
#ifdef E_DEBUG
            QTextStream(stderr) << "Hostname " << host << " found and resolved " << hostAddress.toString() << endl;
#endif
            mayuResult m_result;
            m_result.host = host;
            m_result.result = hostAddress.toString();
            resultList += m_result;
        }
    }
    else {
#ifdef E_DEBUG
        QTextStream(stderr) << "Hostname " << host << " not found" << endl;
#endif
        if (!emptyWhenError) {
            mayuResult m_result;
            m_result.host = host;
            m_result.result = "-1";
            resultList += m_result;
        }
    }
    return resultList;
}

void mayu::parse_hosts()
{
    p_hostsList.clear();
    if (p_hostsFile != "-") {
#ifdef PRIVILEGE_DROP_REQUIRED
        if (!p_dropPrivileges()) {
            p_return = 2;
            return;
        }
#endif
        QFile hostsFile(p_hostsFile);
        if (hostsFile.open(QFile::ReadOnly)) {
            const QList<QByteArray> hostsArray = hostsFile.readAll().split('\n');
            hostsFile.close();
            p_workHosts(hostsArray);
            p_hostsParsed = true;
        }
        else
        {
            QTextStream(stderr) << "Failed read hosts from " << p_hostsFile << endl;
        }
#ifdef PRIVILEGE_DROP_REQUIRED
        if (!p_regainPrivileges()) {
            p_return = 3;
            return;
        }
#endif
    }
    else {
        QByteArray b_hostsArray = QTextStream(stdin).readAll().replace("\\n", "\n").toUtf8();
        const QList<QByteArray> hostsArray = b_hostsArray.split('\n');
        p_workHosts(hostsArray);
        p_hostsParsed = true;
    }
}

void mayu::p_saveWork(const QJsonObject &jsonObject)
{
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QByteArray jsonArray = jsonDocument.toJson();
    if (p_jsonFile != "-") {
#ifdef PRIVILEGE_DROP_REQUIRED
        if (!p_dropPrivileges()) {
            p_return = 2;
            return;
        }
#endif
        QSaveFile jsonFile(p_jsonFile);
        if (jsonFile.open(QSaveFile::WriteOnly)) {
            jsonFile.write(jsonArray);
            if (!jsonFile.commit()) {
                QTextStream(stderr) << "Failed save result to " << p_jsonFile << " because file can't be saved!" << endl;
                p_return = 1;
            }
        }
        else {
            QTextStream(stderr) << "Failed save result to " << p_jsonFile << " because file can't be opened!" << endl;
            p_return = 1;
        }
#ifdef PRIVILEGE_DROP_REQUIRED
        if (!p_regainPrivileges()) {
            p_return = 3;
            return;
        }
#endif
    }
    else {
        QTextStream(stdout) << jsonArray;
    }
    p_return = 0;
}

void mayu::work()
{
    switch(p_mayuMode) {
    case mayuMode::Ping:
#ifdef MAYU_UNIX
        p_workPing();
#else
        QTextStream(stderr) << "Mayu doesn't support pinging on your Operating System!" << endl;
#endif
        break;
    case mayuMode::Resolve:
        p_workResolve();
        break;
    }
}

void mayu::p_workHosts(const QList<QByteArray> &hostsArray)
{
    for (const QByteArray &lineArray : hostsArray) {
        QString lineStr = QString::fromUtf8(lineArray).trimmed();
        if (!lineStr.isEmpty()) {
            QStringList lineStrList = lineStr.split(',');
            QString hostStr;
            QString alternativeStr;
            if (lineStrList.length() >= 1) {
                hostStr = lineStrList.at(0);
                lineStrList.removeAt(0);
                if (lineStrList.length() >= 1) {
                    alternativeStr = lineStrList.join(','); // Alternative Name in Future Version
                }
                p_hostsList += hostStr;
            }
        }
    }
}

#ifdef MAYU_UNIX
void mayu::p_workPing()
{
    if (!p_hostsParsed) {
        parse_hosts();
        if (!p_hostsParsed)
            return;
    }
    QJsonObject jsonObject;
    const QStringList hostsList = getHosts();
    for (const QString &host : hostsList) {
        double result = ping(host, p_tries, p_timeout);
        if (!(result == -1 && p_clean)) {
            jsonObject[host] = result;
        }
    }
    p_saveWork(jsonObject);
}
#endif

void mayu::p_workResolve()
{
    if (!p_hostsParsed) {
        parse_hosts();
        if (!p_hostsParsed)
            return;
    }
    QJsonObject jsonObject;
    const QStringList hostsList = getHosts();
    for (const QString &host : hostsList) {
        const QList<mayuResult> resultList = resolve(host, p_clean);
        QJsonArray arrayList;
        for (const mayuResult &result : resultList) {
            arrayList += result.result;
        }
        if (!arrayList.isEmpty())
            jsonObject[host] = arrayList;
    }
    p_saveWork(jsonObject);
}

#ifdef PRIVILEGE_DROP_REQUIRED
bool mayu::p_dropPrivileges()
{
#if _POSIX_SAVED_IDS
    p_uid = geteuid();
    int status = seteuid(getuid());
    if (status != 0) {
        QTextStream(stderr) << "Dropping of privileges has failed!" << endl;
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool mayu::p_regainPrivileges()
{
#if _POSIX_SAVED_IDS
    int status = seteuid(p_uid);
    if (status != 0) {
        QTextStream(stderr) << "Regaining of privileges has failed!" << endl;
        return false;
    }
    return true;
#else
    return false;
#endif
}
#endif
