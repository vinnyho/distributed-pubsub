
#include <map>
#include <sqlite3.h>
#include <string>

class MessageStore {
private:
  sqlite3 *db;

public:
  MessageStore(int port);
  void save_message_to_db(int &msg_id, const std::string &topic,
                          const std::string &content);
  void restore_messages(
      int &msg_id,
      std::map<std::string, std::vector<std::pair<int, std::string>>>
          &messages);

  ~MessageStore();
};