#include <database/database.hpp>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <uuid.h>
#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <format>
#include <optional>
#include <span>
#include <sstream>

namespace {


    static auto constexpr MIGRATION_TABLE_CREATION_STATEMENT =
        R"(CREATE TABLE migrations (id INTEGER AUTO INCREMENT PRIMARY KEY, uuid TEXT, applied_at DATETIME);)";

    // Migrations are in the form (uuidv4, migration statement)
    // clang-format off
    static std::array<std::pair<const char*, const char*>, 3> constexpr MIGRATIONS{{
     {"7b87b3ab-6153-4904-9270-73b61efe637c", R"(CREATE TABLE pending (id INTEGER AUTO INCREMENT PRIMARY KEY, uuid TEXT, name TEXT, description TEXT, created_at DATETIME);)"},
     {"98739ef0-69eb-4196-a884-b5b18b0e93e7", R"(CREATE TABLE completed (id INTEGER AUTO INCREMENT PRIMARY KEY, uuid TEXT, name TEXT, description TEXT, comments TEXT, date DATETIME, completed_at DATETIME);)"},

     // A bunch of insertions into pending
     {"a6354064-65b5-4d68-a1c4-ee1311b6456c", R"(INSERT INTO pending(uuid, name, description, created_at) values ('d10a98ab-316f-44e8-bd9b-0df2afd8f977', 'example task', 'example desc', DATETIME('now')); )"},
    }};
    // clang-format on

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
        SQLite::Statement migration_uuid_statement{
         db, "INSERT INTO migrations(uuid, applied_at) values (?, DATETIME('now'));"};

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
                // TODO : This should be a transaction for sure: migrations + uuid insertion
                SQLite::Statement migration_statement{db, statement};
                migration_statement.exec();
                migration_statement.reset();

                migration_uuid_statement.bind(1, uuid);
                migration_uuid_statement.exec();
                migration_uuid_statement.reset();
            } catch (SQLite::Exception const& e) {
                spdlog::error("error applying migration {}: {}", uuid, e.what());
                return false;
            }
        }

        return true;
    }

    fachory::db::Time str_to_time(std::string const& date) {
        static auto constexpr DB_DATE_FORMAT = "%Y-%m-%d %H:%M:%S";

        std::tm tm = {};
        std::stringstream ss{date};
        ss >> std::get_time(&tm, DB_DATE_FORMAT);

        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    std::string time_point_to_sqlite(const std::chrono::system_clock::time_point& tp) {
        static auto constexpr DB_DATE_FORMAT = "{:%Y-%m-%d %H:%M:%S}";
        return std::format(DB_DATE_FORMAT, tp);
    }

    std::string generate_uuid() {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size>{};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};
        uuids::uuid const uuid = gen();
        return uuids::to_string(uuid);
    }
} // namespace

namespace fachory::db {

    void to_json(nlohmann::json& j, const Todo& t) {
        std::string const date_str = time_point_to_sqlite(t.created_at);

        // clang-format off
        j = nlohmann::json{
          {"name", t.name},
          {"id", t.id},
          {"description", t.description},
          {"created_at", date_str}
        };
        // clang-format on
    }

    void from_json(const nlohmann::json& j, Todo& t) {

        j.at("name").get_to(t.name);
        j.at("id").get_to(t.id);
        j.at("description").get_to(t.description);

        // convert from string to time point
        std::string date_str;
        j.at("created_at").get_to(date_str);
        t.created_at = str_to_time(date_str);
    }

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

    std::optional<std::string> Database::add_task(std::string const& name, std::string const& description) {
        SQLite::Statement query(
            *_db, R"(INSERT INTO pending(uuid, name, description, created_at) VALUES (?, ?, ?, ?);")");


        auto const uuid = generate_uuid();
        query.bind(1, uuid);
        query.bind(2, name);
        query.bind(3, description);
        query.bind(4, "DATETIME('now')");

        if (query.exec() != 1) {
            spdlog::error("could not insert the task {}", name);
            return std::nullopt;
        }
        return std::make_optional<std::string>(uuid);
    }

    std::vector<Todo> Database::pending_tasks() {

        SQLite::Statement query(*_db, "SELECT id, uuid, name, description, created_at FROM pending;");

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

    std::optional<Todo> Database::pending_task(std::string const& uuid) {

        SQLite::Statement query(*_db, "SELECT id, name, description, created_at FROM pending WHERE uuid = ?;");
        query.bind(1, uuid);


        if (!query.executeStep()) {
            spdlog::error("could not get taks '{}'", uuid);
            return std::nullopt;
        }

        int const id                  = query.getColumn(0);
        std::string const name        = query.getColumn(1);
        std::string const description = query.getColumn(2);
        std::string const date        = query.getColumn(3);

        return Todo{.id = uuid, .name = name, .description = description, .created_at = str_to_time(date)};
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

        // std::string id;
        // std::string name;
        // std::string description;
        // Time created_at;

        return true;
    }

} // namespace fachory::db
