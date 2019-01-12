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


#include "GameData.h"


namespace {
QString pretty_filename(const QFileInfo& fi)
{
    return fi.completeBaseName()
        .replace(QLatin1Char('_'), QLatin1Char(' '))
        .replace(QLatin1Char('.'), QLatin1Char(' '));
}
} // namespace


namespace modeldata {

GameFile::GameFile() = default;
GameFile::GameFile(const QFileInfo& fi)
    : name(pretty_filename(fi))
{}

Game::Game(const QFileInfo& fi)
    : Game(pretty_filename(fi))
{
    // TODO: one call to the prettifier could be optimized out here
    files.emplace(fi.absoluteFilePath(), GameFile(fi));
}

Game::Game(QString title)
    : title(title)
    , player_count(1)
    , is_favorite(false)
    , rating(0.f)
    , playcount(0)
    , playtime(0)
{}

} // namespace modeldata
