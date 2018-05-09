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
#include <QEventLoop>
#include <QHostInfo>
#include <QSaveFile>
#include <QDebug>
#include <QFile>
#include "mayu.h"

#include <iostream>
using namespace std;

extern "C" {
#include "oping.h"
}

mayu::mayu(const QString &hostsFile, const QString &jsonFile, int tries, QObject *parent) : QObject(parent)
{
    p_return = -1;
    p_tries = tries;
    if (!hostsFile.isEmpty())
        setHostsFile(hostsFile);
    if (!jsonFile.isEmpty())
        setJsonFile(jsonFile);
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

void mayu::setMaxTries(int tries)
{
    p_tries = tries;
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

int mayu::getMaxTries()
{
    return p_tries;
}

int mayu::getResult()
{
    return p_return;
}

double mayu::ping(const QString &host, int tries, double timeout)
{
    double latency;
    pingobj_t *pingObj;
    pingobj_iter_t *pingIter;
    if ((pingObj = ping_construct()) == NULL) {
#ifdef E_DEBUG
        qDebug() << "Ping construction failed";
#endif
        return -1;
    }
    if (ping_setopt(pingObj, PING_OPT_TIMEOUT, (void*)(&timeout)) < 0) {
#ifdef E_DEBUG
        qDebug() << "Setting timeout to" << timeout << "have failed";
#endif
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
        qDebug() << "IPv4 Address" << hostAddress.toString() << "found";
#endif
    }
    else if (QAbstractSocket::IPv6Protocol == hostAddress.protocol()) {
        if (ping_host_add(pingObj, hostAddress.toString().toStdString().c_str()) < 0) {
            ping_destroy(pingObj);
            return -1;
        }
#ifdef E_DEBUG
        qDebug() << "IPv6 Address" << hostAddress.toString() << "found";
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
            qDebug() << "Hostname" << host << "found and resolved" << ipStr;
#endif
        }
        else {
#ifdef E_DEBUG
            qDebug() << "Hostname" << host << "not found";
#endif
            ping_destroy(pingObj);
            return -1;
        }
    }
    bool hostUp = false;
    int curTry = 0;
    while (!hostUp && curTry != tries) {
        if (ping_send(pingObj) < 0) {
#ifdef E_DEBUG
            qDebug() << "Pinging host" << host << " has failed";
#endif
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
            qDebug() << hostname << latency << pingSuccess;
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

void mayu::parse_hosts()
{
    p_hostsList.clear();
    if (!dropPrivileges()) {
        p_return = 2;
        return;
    }
    QFile hostsFile(p_hostsFile);
    if (hostsFile.open(QFile::ReadOnly)) {
        const QList<QByteArray> hostsArray = hostsFile.readAll().split('\n');
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
        hostsFile.close();
        p_hostsParsed = true;
    }
    else
    {
        qCritical() << "Failed read hosts from" << p_hostsFile;
    }
    if (!regainPrivileges()) {
        p_return = 3;
        return;
    }
}

void mayu::work()
{
    if (!p_hostsParsed)
        parse_hosts();
    QJsonObject jsonObject;
    const QStringList hostsList = getHosts();
    for (const QString &host : hostsList) {
        double result = ping(host, p_tries);
        jsonObject[host] = result;
    }
    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    QByteArray jsonArray = jsonDocument.toJson();
    if (!dropPrivileges()) {
        p_return = 2;
        return;
    }
    QSaveFile jsonFile(p_jsonFile);
    if (jsonFile.open(QSaveFile::WriteOnly)) {
        jsonFile.write(jsonArray);
        if (!jsonFile.commit()) {
            qCritical() << "Failed save result to" << p_jsonFile << "because file can't be saved!";
            p_return = 1;
        }
    }
    else {
        qCritical() << "Failed save result to" << p_jsonFile << "because file can't be opened!";
        p_return = 1;
    }
    if (!regainPrivileges()) {
        p_return = 3;
        return;
    }
    p_return = 0;
}

bool mayu::dropPrivileges()
{
#if _POSIX_SAVED_IDS
    p_uid = geteuid();
    int status = seteuid(getuid());
    if (status != 0) {
        qCritical() << "Dropping of privileges has failed!";
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool mayu::regainPrivileges()
{
#if _POSIX_SAVED_IDS
    int status = seteuid(p_uid);
    if (status != 0) {
        qCritical() << "Regaining of privileges has failed!";
        return false;
    }
    return true;
#else
    return false;
#endif
}
