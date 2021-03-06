// Pegasus Frontend
// Copyright (C) 2018  Mátyás Mustoha
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


#include "PowerCommands.h"

#include "LocaleUtils.h"

#include <QDebug>


namespace platform {
namespace power {

void reboot()
{
    qCritical().noquote() << tr_log("[error] Reboot not implemented on this platform yet");
}

void shutdown()
{
    qCritical().noquote() << tr_log("[error] Shutdown not implemented on this platform yet");
}

} // namespace power
} // namespace platform
