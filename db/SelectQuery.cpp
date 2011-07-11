/*
 * Illarionserver - server for the game Illarion
 * Copyright 2011 Illarion e.V.
 *
 * This file is part of Illarionserver.
 *
 * Illarionserver  is  free  software:  you can redistribute it and/or modify it
 * under the terms of the  GNU  General  Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * Illarionserver is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY;  without  even  the  implied  warranty  of  MERCHANTABILITY  or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU  General Public License along with
 * Illarionserver. If not, see <http://www.gnu.org/licenses/>.
 */

#include "db/SelectQuery.hpp"

#include <sstream>

#include "db/ConnectionManager.hpp"

using namespace Database;

SelectQuery::SelectQuery(const SelectQuery &org) {
    Query(org);
    WhereQuery(org);
    columns = org.columns;
    tables = org.tables;
}

SelectQuery::SelectQuery() {
    SelectQuery(ConnectionManager::getInstance().getConnection());
}

SelectQuery::SelectQuery(const PConnection connection) {
    Query(connection);
}

SelectQuery::~SelectQuery() {
    ~WhereQuery();
}

void SelectQuery::addColumn(const std::string &column) {
    appendToStringList(columns, escapeKey(column));
}

void SelectQuery::addColumn(const std::string &table, const std::string &column) {
    appendToStringList(columns, escapeAndChainKeys(table, column));
}

void SelectQuery::addServerTable(const std::string &table) {
    appendToStringList(tables, escapeAndChainKeys(Database::SchemaHelper::getServerSchema(), table));
}

void SelectQuery::addAccountTable(const std::string &table) {
    appendToStringList(tables, escapeAndChainKeys(Database::SchemaHelper::getAccountSchema(), table));
}

Result SelectQuery::execute() {
    std::stringstream ss;
    ss << "SELECT ";
    ss << columns;
    ss << " FROM ";
    ss << tables;
    ss << QueryWhere::buildQuerySegment()

    ss << ";";

    setQuery(ss.str());
    Query::execute();
}