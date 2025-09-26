#include "message_store.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

MessageStore::MessageStore(int port) {
  std::string db_filename = "broker" + std::to_string(port) + ".db";

  int return_code = sqlite3_open(db_filename.c_str(), &db);
  if (return_code != SQLITE_OK) {
    std::cerr << "Failed to open database " << db_filename << ": "
              << sqlite3_errmsg(db) << std::endl;
    throw std::runtime_error("Database initialization failed");
  }

  const char *create_table_sql = "CREATE TABLE IF NOT EXISTS messages("
                                 "id INTEGER PRIMARY KEY,"
                                 "topic TEXT NOT NULL,"
                                 "content TEXT NOT NULL,"
                                 "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                                 ");";

  char *err_msg = nullptr;
  return_code = sqlite3_exec(db, create_table_sql, nullptr, nullptr, &err_msg);
  if (return_code != SQLITE_OK) {
    std::cerr << "Failed to create table: "
              << (err_msg ? err_msg : "Unknown error") << std::endl;
    if (err_msg) {
      sqlite3_free(err_msg);
    }
    sqlite3_close(db);
    throw std::runtime_error("Database table creation failed");
  }

}

void MessageStore::save_message_to_db(int &msg_id, const std::string &topic,
                                      const std::string &content) {
  const char *insert_sql =
      "INSERT INTO messages (id, topic, content) VALUES (?, ?, ?)";
  sqlite3_stmt *stmt = nullptr;

  int return_code = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr);
  if (return_code != SQLITE_OK) {
    std::cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db)
              << std::endl;
    return;
  }

  sqlite3_bind_int(stmt, 1, msg_id);
  sqlite3_bind_text(stmt, 2, topic.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);

  return_code = sqlite3_step(stmt);
  if (return_code != SQLITE_DONE) {
    std::cerr << "Failed to insert message: " << sqlite3_errmsg(db)
              << std::endl;
  }

  sqlite3_finalize(stmt);
}

void MessageStore::restore_messages(
    int &msg_id,
    std::map<std::string, std::vector<std::pair<int, std::string>>> &messages) {
  const char *sql_statement =
      "SELECT id, topic, content FROM messages ORDER BY id";
  sqlite3_stmt *stmt = nullptr;

  int return_code = sqlite3_prepare_v2(db, sql_statement, -1, &stmt, nullptr);
  if (return_code != SQLITE_OK) {
    std::cerr << "Failed to prepare restore statement: " << sqlite3_errmsg(db)
              << std::endl;
    return;
  }

  int highest_id = 0;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt, 0);
    const char *topic_cstr =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    const char *content_cstr =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

    if (topic_cstr && content_cstr) {
      std::string topic(topic_cstr);
      std::string content(content_cstr);
      messages[topic].emplace_back(id, content);
      highest_id = std::max(highest_id, id);
    }
  }

  msg_id = highest_id + 1;
  sqlite3_finalize(stmt);

}

MessageStore::~MessageStore() {
  if (db) {
    int return_code = sqlite3_close(db);
    if (return_code != SQLITE_OK) {
      std::cerr << "Warning: Failed to close database properly: "
                << sqlite3_errmsg(db) << std::endl;
    }
    db = nullptr;
  }
}