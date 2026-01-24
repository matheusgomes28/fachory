#ifndef DATABASE_DATABASE_H
#define DATABASE_DATABASE_H

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace SQLite {
    class Database;
}

namespace fachory::db {

    using Time = std::chrono::time_point<std::chrono::system_clock>;

    struct Todo {
        std::string id;
        std::string name;
        std::string description;
        Time created_at;
    };

    void to_json(nlohmann::json& j, const Todo& p);
    void from_json(const nlohmann::json& j, Todo& p);

    class DatabaseException : public std::runtime_error {

    public:
        explicit DatabaseException(std::string const& message);
    };

    class Database {
    public:
        Database(std::string const& db_file, std::string const& db_key);
        ~Database();

        [[nodiscard]] std::optional<std::string> add_task(std::string const& name, std::string const& description);

        [[nodiscard]] std::vector<Todo> pending_tasks();
        [[nodiscard]] std::optional<Todo> pending_task(std::string const& uuid);

        [[nodiscard]] bool mark_task_done(std::string const& uuid);

    private:
        std::unique_ptr<SQLite::Database> _db;
    };
}; // namespace fachory::db


#endif // DATABASE_DATABASE_H
