#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "subprog.h"
#include "env.h"

Subprogram::Subprogram(const std::string& name, const char **env)
    : m_name(name)
    , m_pid(-1)
{
    size_t len = 0;
    while (env[len])
        len++;

    m_env = (char**) calloc(len + 1, sizeof(char *));

    for (int i = 0; i < len; i++)
    {
        m_env[i] = (char*) malloc(strlen(env[i]) + 1);
        strcpy(m_env[i], env[i]);
    }
}

Subprogram::~Subprogram()
{
    for (int i = 0; m_env[i]; i++)
        free(m_env[i]);

    free(m_env);
}

// プログラムの実行を開始。
bool Subprogram::start()
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        char buf[1024];
        strerror_r(errno, buf, sizeof(buf));
        LOG_ERROR("pipe: %s", buf);
        return false;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // 子プロセス。

        // パイプの読み出し側を閉じる。
        close(pipefd[0]);

        // 書き込み側を標準出力として複製する。
        close(1);
        dup(pipefd[1]);

        close(pipefd[1]);

        int r = execle(m_name.c_str(),
                       m_name.c_str(), NULL,
                       m_env);
        if (r == -1)
        {
            // char buf[1024];
            // strerror_r(errno, buf, sizeof(buf));

            char *buf = strerror(errno);
            LOG_ERROR("execle: %s: %s", m_name.c_str(), buf);
            _exit(1);
        }
        /* NOT REACHED */
    }else
    {
        // 親プロセス。

        m_pid = pid;

        // 書き込み側を閉じる。
        close(pipefd[1]);

        m_inputStream.openReadOnly(pipefd[0]);
    }
    return true;
}

// プログラムにより終了ステータスが返された場合は true を返し、ステー
// タスが *status にセットされる。
bool Subprogram::wait(int* status)
{
    int r;

    waitpid(m_pid, &r, 0);
    if (WIFEXITED(r))
    {
        *status = WEXITSTATUS(r);
        return true;
    }
    else
    {
        return false;
    }
}

// プログラムの出力を読み出すストリーム。
Stream& Subprogram::inputStream()
{
    return m_inputStream;
}

// プロセスID。
int Subprogram::pid()
{
    return m_pid;
}
