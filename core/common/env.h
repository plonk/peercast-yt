#ifndef _ENV_H
#define _ENV_H

#include <vector>
#include <string>

// 環境変数のビルダークラス。
class Environment
{
public:
    Environment();
    Environment(char *src[]);
    ~Environment();

    void set(const std::string&, const std::string&);
    void unset(const std::string& key);
    std::string get(const std::string&) const;
    bool hasKey(const std::string&) const;
    std::vector<std::string> keys() const;
    size_t size() const { return m_vars.size(); }
    void copyFromCurrentProcess();

    std::vector<std::string> m_vars;

    char const ** m_env;
    // 環境はオブジェクト内部に作成され、ポインターが返される。オブジェ
    // クトに変更が加えられるとデータは無効化される。
    char const ** env();

    std::string windowsEnvironmentBlock();
};

#endif
