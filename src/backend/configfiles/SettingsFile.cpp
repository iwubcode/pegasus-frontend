// Pegasus Frontend
// Copyright (C) 2017-2018  Mátyás Mustoha
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


#include "SettingsFile.h"

#include "AppSettings.h"
#include "ConfigFile.h"
#include "LocaleUtils.h"
#include "Paths.h"

#include <QDebug>
#include <QFile>
#include <QKeySequence>


namespace {

std::map<QString, QKeySequence> gen_gamepad_names() {
    std::map<QString, QKeySequence> result;

    for (const auto& pair : AppSettings::gamepadButtonNames)
        result.emplace(QStringLiteral("Gamepad") + pair.second, pair.first);

    return result;
}

} // namespace


namespace appsettings {

ConfigEntryMaps::ConfigEntryMaps()
    : str_to_category {
        { QStringLiteral("general"), Category::GENERAL },
        { QStringLiteral("providers"), Category::PROVIDERS },
        { QStringLiteral("keys"), Category::KEYS },
    }
    , str_to_general_opt {
        { QStringLiteral("portable"), GeneralOption::PORTABLE },
        { QStringLiteral("silent"), GeneralOption::SILENT },
        { QStringLiteral("fullscreen"), GeneralOption::FULLSCREEN },
        { QStringLiteral("locale"), GeneralOption::LOCALE },
        { QStringLiteral("theme"), GeneralOption::THEME },
    }
    , str_to_extprovider {
        { QStringLiteral("es2"), ExtProvider::ES2 },
        { QStringLiteral("steam"), ExtProvider::STEAM },
        { QStringLiteral("gog"), ExtProvider::GOG },
        { QStringLiteral("androidapps"), ExtProvider::ANDROIDAPPS },
        { QStringLiteral("skraper"), ExtProvider::SKRAPER },
    }
    , str_to_key_opt {
        { QStringLiteral("accept"), KeyEvent::ACCEPT },
        { QStringLiteral("cancel"), KeyEvent::CANCEL },
        { QStringLiteral("details"), KeyEvent::DETAILS },
        { QStringLiteral("filters"), KeyEvent::FILTERS },
        { QStringLiteral("next-page"), KeyEvent::NEXT_PAGE },
        { QStringLiteral("prev-page"), KeyEvent::PREV_PAGE },
        { QStringLiteral("page-up"), KeyEvent::PAGE_UP },
        { QStringLiteral("page-down"), KeyEvent::PAGE_DOWN },
        { QStringLiteral("menu"), KeyEvent::MAIN_MENU },
    }
{}


SettingsFileContext::SettingsFileContext()
    : config_path(paths::writableConfigDir() + QStringLiteral("/settings.txt"))
{}


LoadContext::LoadContext()
    : reverse_gamepadButtonNames(gen_gamepad_names())
{}

void LoadContext::load() const
{
    const auto on_error = [this](const int lineno, const QString msg){
        log_error(lineno, msg);
    };
    const auto on_attribute = [this](const config::Entry& entry){
        handle_entry(entry.line, entry.key, config::mergeLines(entry.values)); // TODO!!!!!
    };

    config::readFile(config_path, on_attribute, on_error);
    qInfo().noquote() << tr_log("Program settings loaded");
}

void LoadContext::log_error(const int lineno, const QString& msg) const
{
    qWarning().noquote()
        << tr_log("`%1`, line %2: %3").arg(config_path, QString::number(lineno), msg);
}

void LoadContext::log_unknown_key(const int lineno, const QString& key) const
{
    log_error(lineno,
        tr_log("unrecognized option `%1`, ignored").arg(key));
}

void LoadContext::log_needs_bool(const int lineno, const QString& key) const
{
    log_error(lineno,
        tr_log("this option (`%1`) must be a boolean (true/false) value").arg(key));
}

void LoadContext::handle_entry(const int lineno, const QString& key, const QString& val) const
{
    QStringList sections = key.split('.');
    if (sections.size() < 2) {
        log_unknown_key(lineno, key);
        return;
    }

    const QString category_str = sections.takeFirst();
    const auto category_it = maps.str_to_category.find(category_str);
    if (category_it == maps.str_to_category.cend()) {
        log_unknown_key(lineno, key);
        return;
    }

    switch (category_it->second) {
        case ConfigEntryCategory::GENERAL:
            handle_general_attrib(lineno, key, val, sections);
            break;
        case ConfigEntryCategory::PROVIDERS:
            handle_provider_attrib(lineno, key, val, sections);
            break;
        case ConfigEntryCategory::KEYS:
            handle_key_attrib(lineno, key, val, sections);
            break;
    }
}

void LoadContext::handle_general_attrib(const int lineno, const QString& key, const QString& val,
                                        QStringList& sections) const
{
    const auto option_it = maps.str_to_general_opt.find(sections.constFirst());
    if (option_it == maps.str_to_general_opt.cend()) {
        log_unknown_key(lineno, key);
        return;
    }

    switch (option_it->second) {
        case ConfigEntryGeneralOption::PORTABLE:
            strconv.store_maybe(AppSettings::general.portable, val,
                [&](){ log_needs_bool(lineno, key); });
            break;
        case ConfigEntryGeneralOption::SILENT:
            strconv.store_maybe(AppSettings::general.silent, val,
                [&](){ log_needs_bool(lineno, key); });
            break;
        case ConfigEntryGeneralOption::FULLSCREEN:
            strconv.store_maybe(AppSettings::general.fullscreen, val,
                [&](){ log_needs_bool(lineno, key); });
            break;
        case ConfigEntryGeneralOption::LOCALE:
            AppSettings::general.locale = val;
            break;
        case ConfigEntryGeneralOption::THEME:
            AppSettings::general.theme = val;
            break;
    }
}

void LoadContext::handle_provider_attrib(const int lineno, const QString& key, const QString& val,
                                         QStringList& sections) const
{
    if (sections.size() < 2) {
        log_unknown_key(lineno, key);
        return;
    }

    const auto provider_it = maps.str_to_extprovider.find(sections.takeFirst());
    if (provider_it == maps.str_to_extprovider.cend()) {
        log_unknown_key(lineno, key);
        return;
    }

    const auto option = sections.takeFirst();
    if (option != QStringLiteral("enabled")) {
        log_unknown_key(lineno, key);
        return;
    }

    auto& provider = AppSettings::ext_providers.mut(provider_it->second);
    strconv.store_maybe(provider.enabled, val,
        [&](){ log_needs_bool(lineno, key); });
}

void LoadContext::handle_key_attrib(const int lineno, const QString& key, const QString& val,
                       QStringList& sections) const
{
    const auto key_it = maps.str_to_key_opt.find(sections.constFirst());
    if (key_it == maps.str_to_key_opt.cend()) {
        log_unknown_key(lineno, key);
        return;
    }

    const KeyEvent key_event = key_it->second;

    AppSettings::keys.clear(key_event);
    if (val.toLower() == QStringLiteral("none"))
        return;

    QVector<QKeySequence> keyseqs;

    const auto key_strs = val.splitRef(',');
    for (QStringRef strref : key_strs) {
        const QString str = strref.trimmed().toString();

        const auto gamepadbtn_it = reverse_gamepadButtonNames.find(str);
        if (gamepadbtn_it != reverse_gamepadButtonNames.cend()) {
            keyseqs.append(gamepadbtn_it->second);
            continue;
        }

        QKeySequence keyseq(str);
        if (!keyseq.isEmpty())
            keyseqs.append(std::move(keyseq));
    }

    for (const QKeySequence& keyseq : qAsConst(keyseqs))
        AppSettings::keys.add_key(key_event, keyseq);
}


SaveContext::SaveContext()
    : STR_TRUE(QStringLiteral("true"))
    , STR_FALSE(QStringLiteral("false"))
    , LINE_TEMPLATE(QStringLiteral("%1.%2: %3\n"))
    , category_names(gen_category_names())
{}

void SaveContext::save() const
{
    QFile config_file(config_path);
    if (!config_file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning().noquote()
            << tr_log("Failed to save program settings to `%1`").arg(config_path);
        return;
    }

    QTextStream stream(&config_file);
    print_general(stream);
    print_providers(stream);
    print_keys(stream);

    qInfo().noquote() << tr_log("Program settings saved");
}

void SaveContext::print_general(QTextStream& stream) const
{
    using GeneralOption = ConfigEntryGeneralOption;
    using GeneralStrMap = HashMap<GeneralOption, QString, EnumHash>;

    GeneralStrMap option_names;
    for (const auto& entry : maps.str_to_general_opt)
        option_names.emplace(entry.second, entry.first);

    GeneralStrMap option_values {
        { GeneralOption::FULLSCREEN, AppSettings::general.fullscreen ? STR_TRUE : STR_FALSE },
        { GeneralOption::LOCALE, AppSettings::general.locale },
        { GeneralOption::THEME, AppSettings::general.theme },
    };

    for (const auto& entry : option_values) {
        stream << LINE_TEMPLATE.arg(
            category_names.at(ConfigEntryCategory::GENERAL),
            option_names.at(entry.first),
            entry.second);
    }
}

void SaveContext::print_providers(QTextStream& stream) const
{
    for (const auto& entry : maps.str_to_extprovider) {
        stream << LINE_TEMPLATE.arg(
            category_names.at(ConfigEntryCategory::PROVIDERS),
            entry.first + QStringLiteral(".enabled"),
            AppSettings::ext_providers[entry.second].enabled ? STR_TRUE : STR_FALSE);
    }
}

void SaveContext::print_keys(QTextStream& stream) const
{
    for (const auto& entry : maps.str_to_key_opt) {
        QStringList key_strs;

        for (const QKeySequence& keyseq : AppSettings::keys.at(entry.second)) {
            const auto btnname_it = AppSettings::gamepadButtonNames.find(keyseq);
            if (btnname_it != AppSettings::gamepadButtonNames.cend()) {
                key_strs << QStringLiteral("Gamepad") + AppSettings::gamepadButtonNames.at(keyseq);
                continue;
            }

            key_strs << keyseq.toString();
        }

        if (key_strs.isEmpty())
            key_strs << QStringLiteral("none");

        stream << LINE_TEMPLATE.arg(
            category_names.at(ConfigEntryCategory::KEYS),
            entry.first,
            key_strs.join(','));
    }
}

HashMap<ConfigEntryCategory, QString, EnumHash> SaveContext::gen_category_names() const {
    HashMap<ConfigEntryCategory, QString, EnumHash> result;

    for (const auto& entry : maps.str_to_category)
        result.emplace(entry.second, entry.first);

    return result;
}

} // namespace appsettings
