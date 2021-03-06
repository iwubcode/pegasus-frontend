// Pegasus Frontend
// Copyright (C) 2017  Mátyás Mustoha
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.


#include "AppSettings.h"
#include "AppContext.h"
#include "Backend.h"
#include "LocaleUtils.h"
#include "Log.h"
#include "platform/TerminalKbd.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QSettings>


void handle_cli_args(QGuiApplication&);

int main(int argc, char *argv[])
{
    TerminalKbd::on_startup();

    QCoreApplication::addLibraryPath(QStringLiteral("lib/plugins"));
    QCoreApplication::addLibraryPath(QStringLiteral("lib"));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("pegasus-frontend"));
    app.setApplicationVersion(QStringLiteral(GIT_REVISION));
    app.setOrganizationName(QStringLiteral("pegasus-frontend"));
    app.setOrganizationDomain(QStringLiteral("pegasus-frontend.org"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icon.png")));

    handle_cli_args(app);
    Log::init();
    AppSettings::load_config();

    backend::AppContext context;
    backend::Backend backend;
    backend.start();

    return app.exec();
}

void handle_cli_args(QGuiApplication& app)
{
    QCommandLineParser argparser;
    argparser.setApplicationDescription(tr_log(
        "\nPegasus is a graphical frontend for browsing your game library (especially\n"
        "retro games) and launching them from one place. It's focusing on customization,\n"
        "cross platform support (including embedded devices) and high performance."));

    const QCommandLineOption arg_portable(QStringLiteral("portable"),
        tr_log("Do not read or write config files outside the program's directory"));
    argparser.addOption(arg_portable);

    const QCommandLineOption arg_silent(QStringLiteral("silent"),
        tr_log("Do not print log messages to the terminal"));
    argparser.addOption(arg_silent);

    argparser.addHelpOption();
    argparser.addVersionOption();
    argparser.process(app); // may quit!


    AppSettings::general.portable = argparser.isSet(arg_portable);
    AppSettings::general.silent = argparser.isSet(arg_silent);
}
