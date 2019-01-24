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


#include "GogMetadata.h"

#include "GogCommon.h"
#include "LocaleUtils.h"
#include "providers/JsonCacheUtils.h"
#include "modeldata/gaming/GameData.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>


namespace {
static constexpr auto MSG_PREFIX = "GOG:";
static constexpr auto JSON_CACHE_DIR = "gog";

struct GogEntry {
    QString gogid;
    modeldata::Game* game;

    GogEntry(QString gogid, modeldata::Game* const game)
        : gogid(std::move(gogid))
        , game(game)
    {
        Q_ASSERT(!this->gogid.isEmpty());
        Q_ASSERT(this->game);
    }

    MOVE_ONLY(GogEntry)
};

bool read_api_json(GogEntry& entry, const QJsonDocument& json)
{
    if (json.isNull())
        return false;

    const auto json_root = json.object();
    if (json_root.isEmpty())
        return false;


    modeldata::Game& game = *entry.game;

    const auto desc = json_root[QLatin1String("description")].toObject();
    if (!desc.isEmpty()) {
        game.summary = desc[QLatin1String("lead")].toString().replace('\n', ' ');
        game.description = desc[QLatin1String("full")].toString().replace('\n', ' ');
    }

    const auto date_raw = json_root[QLatin1String("release_date")].toString();
    game.release_date = QDate::fromString(date_raw, Qt::ISODate);


    modeldata::GameAssets& assets = game.assets;

    const auto images = json_root[QLatin1String("images")].toObject();
    if (!images.isEmpty()) {
        const QString prefix(QStringLiteral("https:"));

        const QString box_front = images[QLatin1String("logo2x")].toString();
        const QString background = images[QLatin1String("background")].toString();
        const QString logo = images[QLatin1String("icon")].toString();

        if (!box_front.isEmpty())
            assets.setSingle(AssetType::BOX_FRONT, prefix + box_front);
        if (!background.isEmpty())
            assets.setSingle(AssetType::BACKGROUND, prefix + background);
        if (!logo.isEmpty())
            assets.setSingle(AssetType::LOGO, prefix + logo);
    }

    const auto screenshots = json_root[QLatin1String("screenshots")].toArray();
    for (const auto& array_entry : screenshots) {
        const auto screenshot = array_entry.toObject();
        const auto url = screenshot[QLatin1String("formatter_template_url")].toString()
            .replace(QStringLiteral("{formatter}"), QStringLiteral("ggvgm_2x"));

        if (!url.isEmpty())
            assets.appendMulti(AssetType::SCREENSHOTS, url);
    }

    return true;
}

bool read_embed_json(GogEntry& entry, const QJsonDocument& json)
{
    if (json.isNull())
        return false;

    const auto json_root = json.object();
    if (json_root.isEmpty())
        return false;


    const auto products = json_root[QLatin1String("products")].toArray();
    for (const auto& products_entry : products) {
        const auto product = products_entry.toObject();

        const auto id = product[QLatin1String("id")].toInt();
        if (id == 0 || QString::number(id) != entry.gogid)
            continue;

        modeldata::Game& game = *entry.game;

        game.developers.append(product[QLatin1String("developer")].toString());
        game.publishers.append(product[QLatin1String("publisher")].toString());

        const auto genres = product[QLatin1String("genres")].toArray();
        for (const auto& genres_entry : genres)
            game.genres.append(genres_entry.toString());
    }

    return true;
}

bool fill_from_cache(GogEntry& entry)
{
    const QString message_prefix = QLatin1String(MSG_PREFIX);
    const QString cache_dir = QLatin1String(JSON_CACHE_DIR);
    const QString entry_api = entry.gogid + providers::gog::json_api_suffix();
    const QString entry_embed = entry.gogid + providers::gog::json_embed_suffix();

    const auto json_api = providers::read_json_from_cache(message_prefix, cache_dir, entry_api);
    const bool json_api_success = read_api_json(entry, json_api);
    if (!json_api_success)
        providers::delete_cached_json(message_prefix, cache_dir, entry_api);

    const auto json_embed = providers::read_json_from_cache(message_prefix, cache_dir, entry_embed);
    const bool json_embed_success = read_embed_json(entry, json_embed);
    if (!json_embed_success)
        providers::delete_cached_json(message_prefix, cache_dir, entry_embed);

    return json_api_success && json_embed_success;
}

void download_metadata(std::vector<GogEntry>& entries, QNetworkAccessManager& netman)
{
    const int TIMEOUT_MS(5000);
    const auto API_URL(QStringLiteral("https://api.gog.com/products/%1?expand=description,screenshots,videos"));
    const auto EMBED_URL(QStringLiteral("https://embed.gog.com/games/ajax/filtered?mediaType=game&search=%1"));

    const QString message_prefix = QLatin1String(MSG_PREFIX);
    const QString cache_dir = QLatin1String(JSON_CACHE_DIR);


    QVector<QNetworkReply*> listeners_api;
    QVector<QNetworkReply*> listeners_embed;
    int completed_transfers_api = 0;
    int completed_transfers_embed = 0;

    QTimer loop_timeout;
    loop_timeout.setSingleShot(true);
    loop_timeout.start(TIMEOUT_MS);

    QEventLoop loop;
    QObject::connect(&loop_timeout, &QTimer::timeout,
                     &loop, &QEventLoop::quit);

    // FIXME: Remove this huge duplication -- or rather, rethink the process

    for (size_t i = 0; i < entries.size(); i++) {
        const QString gog_id = entries[i].gogid;
        modeldata::Game* const game = entries[i].game;

        const QUrl url(API_URL.arg(gog_id));
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        QNetworkReply* reply = netman.get(request);

        QObject::connect(reply, &QNetworkReply::finished,
            [&, game, i, reply](){
                completed_transfers_api++;
                if (completed_transfers_api == listeners_api.count() && completed_transfers_embed == listeners_embed.count())
                    loop.quit();

                if (reply->error()) {
                    qWarning().noquote() << MSG_PREFIX
                        << tr_log("downloading metadata for `%1` failed (%2)")
                           .arg(game->title, reply->errorString());
                }
                else {
                    const auto raw_data = reply->readAll();
                    const bool json_success = read_api_json(entries[i], QJsonDocument::fromJson(raw_data));
                    if (json_success) {
                        const QString json_name = gog_id + providers::gog::json_api_suffix();
                        providers::cache_json(message_prefix, cache_dir, json_name, raw_data);
                    }
                    else {
                        qWarning().noquote() << MSG_PREFIX
                            << tr_log("failed to parse the response of the server "
                                      "for game `%1` - perhaps the GOG API changed?")
                                      .arg(game->title);
                    }
                }

                loop_timeout.start(TIMEOUT_MS); // restart the timer
            });

        listeners_api << reply;
    }

    for (size_t i = 0; i < entries.size(); i++) {
        const QString gog_id = entries[i].gogid;
        modeldata::Game* const game = entries[i].game;

        const QUrl url(EMBED_URL.arg(game->title)); // TODO: this seems to work, but shouldn't it be escaped?
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        QNetworkReply* reply = netman.get(request);

        QObject::connect(reply, &QNetworkReply::finished,
            [&, i, reply](){
                completed_transfers_embed++;
                if (completed_transfers_api == listeners_api.count() && completed_transfers_embed == listeners_embed.count())
                    loop.quit();

                if (reply->error()) {
                    qWarning().noquote() << MSG_PREFIX
                        << tr_log("downloading secondary metadata for `%1` failed (%2)")
                           .arg(game->title, reply->errorString());
                }
                else {
                    const auto raw_data = reply->readAll();
                    const bool json_success = read_embed_json(entries[i], QJsonDocument::fromJson(raw_data));
                    if (json_success) {
                        const QString json_name = gog_id + providers::gog::json_embed_suffix();
                        providers::cache_json(message_prefix, cache_dir, json_name, raw_data);
                    }
                    else {
                        qWarning().noquote() << MSG_PREFIX
                            << tr_log("failed to parse the response of the server "
                                      "for game `%1` - perhaps the GOG API changed?")
                                      .arg(game->title);
                    }
                }

                loop_timeout.start(TIMEOUT_MS); // restart the timer
            });

        listeners_embed << reply;
    }

    loop.exec();


    for (auto& listener : listeners_api) {
        if (!listener->isFinished())
            listener->abort();

        listener->deleteLater();
        listener = nullptr;
    }
    for (auto& listener : listeners_embed) {
        if (!listener->isFinished())
            listener->abort();

        listener->deleteLater();
        listener = nullptr;
    }
}
} // namespace


namespace providers {
namespace gog {

Metadata::Metadata(QObject* parent)
    : QObject(parent)
{}

void Metadata::enhance(providers::SearchContext& sctx, HashMap<size_t, QString>& gogid_map)
{
    const QString GOG_TAG(QStringLiteral("GOG"));
    if (!sctx.collection_childs.count(GOG_TAG))
        return;

    std::vector<GogEntry> entries;

    const std::vector<size_t>& childs = sctx.collection_childs.at(GOG_TAG);
    for (const size_t game_idx : childs) {
        if (Q_LIKELY(gogid_map.count(game_idx)))
            entries.emplace_back(gogid_map.at(game_idx), &sctx.games.at(game_idx));
    }

    // try to fill using cached jsons

    const auto del_from = std::remove_if(entries.begin(), entries.end(),
        [](GogEntry& entry){ return fill_from_cache(entry); });// ie. remove if cached
    entries.erase(del_from, entries.end());

    if (entries.empty())
        return;

    // try to fill from network

    QNetworkAccessManager netman; // TODO: move NAM to global
    if (netman.networkAccessible() != QNetworkAccessManager::Accessible) {
        qWarning().noquote()
            << MSG_PREFIX
            << tr_log("no internet connection - most game data may be missing");
        return;
    }
    download_metadata(entries, netman);
}

} // namespace gog
} // namespace providers
