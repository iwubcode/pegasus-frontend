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


#pragma once

#include "Provider.h"
#include "utils/FwdDeclModel.h"

#include <QObject>
#include <QFuture>
#include <memory>

template<typename T> class QQmlObjectListModel;
using ProviderPtr = std::unique_ptr<providers::Provider>;


class ProviderManager : public QObject {
    Q_OBJECT

public:
    explicit ProviderManager(QObject* parent);

    size_t providerCount() const { return m_providers.size(); }

    void startSearch(QQmlObjectListModel<model::Game>&, QQmlObjectListModel<model::Collection>&);
    void onGameLaunched(model::GameFile* const);
    void onGameFinished(model::GameFile* const);
    void onGameFavoriteChanged(const QVector<model::Game*>&);

signals:
    void gameCountChanged(int);
    void singleProviderFinished();

    void firstPhaseComplete(qint64);
    void secondPhaseComplete(qint64);
    void staticDataReady();
    void thirdPhaseComplete(qint64);

private:
    std::vector<ProviderPtr> m_providers;
    QFuture<void> m_init_seq;
};
