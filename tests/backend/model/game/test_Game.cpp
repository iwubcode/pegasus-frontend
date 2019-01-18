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


#include <QtTest/QtTest>

#include "model/gaming/Game.h"


class test_Game : public QObject {
    Q_OBJECT

private slots:
    void developers();
    void publishers();
    void genres();
    void release();

    void launchSingle();
    void launchMulti();
};

void testStrAndList(std::function<void(modeldata::Game&, const QString&)> fn_add,
                    const char* str_name,
                    const char* list_name)
{
    modeldata::Game modeldata("test");
    fn_add(modeldata, "test1");
    fn_add(modeldata, "test2");
    fn_add(modeldata, "test3");

    model::Game game(std::move(modeldata));

    QCOMPARE(game.property(str_name).toString(), QStringLiteral("test1, test2, test3"));
    QCOMPARE(game.property(list_name).toStringList(), QStringList({"test1", "test2", "test3"}));
}

void test_Game::developers()
{
    auto fn = [](modeldata::Game& game, const QString& val){ game.developers.append(val); };
    testStrAndList(fn, "developer", "developerList");
}

void test_Game::publishers()
{
    auto fn = [](modeldata::Game& game, const QString& val){ game.publishers.append(val); };
    testStrAndList(fn, "publisher", "publisherList");
}

void test_Game::genres()
{
    auto fn = [](modeldata::Game& game, const QString& val){ game.genres.append(val); };
    testStrAndList(fn, "genre", "genreList");
}

void test_Game::release()
{
    modeldata::Game modeldata("test");
    modeldata.release_date = QDate(1999,1,2);

    model::Game game(std::move(modeldata));
    QCOMPARE(game.property("releaseYear").toInt(), 1999);
    QCOMPARE(game.property("releaseMonth").toInt(), 1);
    QCOMPARE(game.property("releaseDay").toInt(), 2);
}

void test_Game::launchSingle()
{
    modeldata::Game gamedata("test");
    gamedata.files.emplace("a", modeldata::GameFile(QFileInfo("test")));
    model::Game game(std::move(gamedata));

    QSignalSpy spy_launch(game.files()->first(), &model::GameFile::launchRequested);
    QVERIFY(spy_launch.isValid());

    QMetaObject::invokeMethod(&game, "launch");
    QVERIFY(spy_launch.count() == 1 || spy_launch.wait());
}

void test_Game::launchMulti()
{
    modeldata::Game gamedata("test");
    gamedata.files.emplace("a", modeldata::GameFile(QFileInfo("test1")));
    gamedata.files.emplace("b", modeldata::GameFile(QFileInfo("test2")));
    model::Game game(std::move(gamedata));

    QSignalSpy spy_launch(&game, &model::Game::launchFileSelectorRequested);
    QVERIFY(spy_launch.isValid());

    QMetaObject::invokeMethod(&game, "launch");
    QVERIFY(spy_launch.count() == 1 || spy_launch.wait());
}


QTEST_MAIN(test_Game)
#include "test_Game.moc"
