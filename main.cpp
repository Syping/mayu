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

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include "mayu.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setSetuidAllowed(true);
    QCoreApplication a(argc, argv);
    a.setApplicationName("mayu");

    QStringList arguments = a.arguments();
    arguments.removeAt(0);

    mayuMode a_mode = mayuMode::Ping;
    for (int i = arguments.length(); i > 0; i--) {
        const QString &argument = arguments.at(i-1);
        if (argument == "-p" || argument == "--ping") {
            a_mode = mayuMode::Ping;
            arguments.removeAt(i-1);
        }
        else if (argument == "-r" || argument == "--resolve") {
            a_mode = mayuMode::Resolve;
            arguments.removeAt(i-1);
        }
    }

    if (arguments.length() >= 2) {
        mayu a_mayu(arguments.at(0), arguments.at(1));
        a_mayu.setMayuMode(a_mode);
        a_mayu.work();
        return a_mayu.getResult();
    }
    else {
        QTextStream(stdout) << "Usage: " << a.arguments().at(0) << " [-p ping]" << " [-r resolve]" << " input.txt" << " output.json" << endl;
    }

    return 0;
}
