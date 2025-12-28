#include <database/database.hpp>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <span>
#include <sstream>

namespace {

    static auto constexpr MIGRATION_TABLE_CREATION_STATEMENT =
        R"(CREATE TABLE migrations (id INTEGER AUTO INCREMENT PRIMARY KEY, uuid TEXT, applied_at DATETIME);)";

    // Migrations are in the form (uuidv4, migration statement)
    static std::array<std::pair<const char*, const char*>, 2> constexpr MIGRATIONS{{
     {"7b87b3ab-6153-4904-9270-73b61efe637c", R"(CREATE TABLE pending (id INTEGER AUTO INCREMENT PRIMARY KEY);)"},
     {"98739ef0-69eb-4196-a884-b5b18b0e93e7",
      R"(CREATE TABLE completed (id INTEGER AUTO INCREMENT PRIMARY KEY, uuid TEXT, name TEXT, description TEXT, comments TEXT, date DATETIME, completed_at DATETIME);)"},
    }};

    fachory::db::Time str_to_time(std::string const& date) {
        std::tm tm = {};
        std::stringstream ss("Jan 9 2014 12:35:34");
        ss >> std::get_time(&tm, "%b %d %Y %H:%M:%S");

        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    bool check_db_connection(SQLite::Database& db) {
        try {
            SQLite::Statement query(db, "SELECT 1");
            return query.executeStep();
        } catch (const SQLite::Exception& e) {
            return false;
        }
    }

    bool migrate_db(SQLite::Database& db, std::span<std::pair<const char*, const char*> const> migrations) {
        // Check if the metadata table exists
        if (!db.tableExists("migrations")) {
            auto const ret = db.exec(MIGRATION_TABLE_CREATION_STATEMENT);
            if (ret != SQLite::OK) {
                return false;
            }
        }

        SQLite::Statement check_statement{db, "SELECT (id) FROM migrations WHERE uuid = ?;"};
        for (auto const& [uuid, statement] : migrations) {

            // Check the migrations exists
            check_statement.bind(1, uuid);
            auto const check_res = check_statement.executeStep();
            check_statement.reset();

            if (check_res) {
                spdlog::warn("migration {} has already been applied, skipping", uuid);
                continue;
            }

            // apply the migration
            spdlog::info("appying migration {}", uuid);
            try {
                SQLite::Statement migration_statement{db, statement};
                migration_statement.executeStep();

                SQLite::Statement migration_uuid_statement{
                 db, "INSERT INTO migrations(uuid, applied_at) values(?, DATE('now'))"};
                migration_uuid_statement.executeStep();
            } catch (SQLite::Exception const& e) {
                spdlog::error("error applying migration {}: {}", uuid, e.what());
                return false;
            }
        }

        return true;
    }
} // namespace

namespace fachory::db {

    DatabaseException::DatabaseException(std::string const& message)
        : std::runtime_error(message) {}


    Database::Database(std::string const& db_file, std::string const& db_key)
        : _db{std::make_unique<SQLite::Database>(db_file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)} {
        _db->key(db_key);

        if (!check_db_connection(*_db)) {
            throw new DatabaseException{fmt::format("could not create the database from file {}", db_file)};
        }

        if (!migrate_db(*_db, MIGRATIONS)) {
            throw new DatabaseException{"could not apply database migrations"};
        }
    }

    Database::~Database() {}

    std::vector<Todo> Database::pending_tasks() {

        SQLite::Statement query(*_db, "SELECT (id, uuid, name, description, date) FROM pending");

        std::vector<Todo> all_tasks;

        while (query.executeStep()) {

            int const id                  = query.getColumn(0);
            std::string const uuid        = query.getColumn(1);
            std::string const name        = query.getColumn(2);
            std::string const description = query.getColumn(3);
            std::string const date        = query.getColumn(4);

            spdlog::info("new task ({}, {}, {}, {})", uuid, name, description, date);
            all_tasks.push_back(
                Todo{.id = uuid, .name = name, .description = description, .created_at = str_to_time(date)});
        }

        return all_tasks;
    }

    bool Database::mark_task_done(std::string const& uuid) {

        SQLite::Statement query(*_db, "DELETE FROM pending WHERE uuid = ?");

        int affected = query.exec();
        if (affected <= 0) {
            spdlog::error("task {} was not deleted", uuid);
            return false;
        }

        // TODO : We probably want to add this task to the other table that
        // TODO : tracks the done tasks. It should have "created_at" and "done_at"

        return true;
    }

} // namespace fachory::db
