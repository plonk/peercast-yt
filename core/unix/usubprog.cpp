#include <stdexcept>

#include "subprog.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // kill(2)

Subprogram::Subprogram(const std::string& name, bool receiveData, bool feedData)
    : m_name(name)
    , m_pid(-1)
    , m_receiveData(receiveData)
    , m_feedData(feedData)
{
}

Subprogram::~Subprogram()
{
}

// プログラムの実行を開始。
bool Subprogram::start(std::initializer_list<std::string> arguments, Environment& env)
{
    int stdoutPipe[2];
    int stdinPipe[2];

    if (pipe(stdoutPipe) == -1)
    {
        char buf[1024];
        strerror_r(errno, buf, sizeof(buf));
        LOG_ERROR("pipe: %s", buf);
        return false;
    }

    if (pipe(stdinPipe) == -1)
    {
        char buf[1024];
        strerror_r(errno, buf, sizeof(buf));
        LOG_ERROR("pipe: %s", buf);

        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return false;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // 子プロセス。

        // パイプの読み出し側を閉じる。
        close(stdoutPipe[0]);
        close(stdinPipe[1]);

        // 書き込み側を標準出力として複製する。
        if (m_receiveData)
        {
            close(1);
            dup(stdoutPipe[1]);
        }
        if (m_feedData)
        {
            close(0);
            dup(stdinPipe[0]);
        }

        close(stdoutPipe[1]);
        close(stdinPipe[0]);

        // exec するか exit するかなので解放しなくていいかも。
        const char* *argv = new const char* [arguments.size() + 1];
        argv[0] = m_name.c_str();
        int i = 1;
        for (auto it = arguments.begin(); it != arguments.end(); ++it)
        {
            argv[i++] = it->c_str();
        }
        argv[i] = nullptr;

        int r = execvpe(m_name.c_str(), (char* const*) argv, (char* const*) env.env());
        if (r == -1)
        {
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
        close(stdoutPipe[1]);
        close(stdinPipe[0]);

        if (m_receiveData)
            m_inputStream.openReadOnly(stdoutPipe[0]);
        else
            close(stdoutPipe[0]);

        if (m_feedData)
            m_outputStream.openWriteReplace(stdinPipe[1]);
        else
            close(stdinPipe[1]);
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

bool Subprogram::isAlive()
{
    int r;

    r = waitpid(m_pid, NULL, WNOHANG);

    if (r == 0)
        return true;
    else if (r == -1)
    {
        LOG_ERROR("Failed in checking the status of %d: %s", (int) m_pid, strerror(errno));
        return true;
    }
    else
    {
        return false; // the child died
    }

    // ここで waitpid したあと this->wait() すると深刻な問題がありそう。
}

void Subprogram::terminate()
{
    int r;

    r = kill(m_pid, 9); // send SIGKILL

    if (r == -1)
        LOG_ERROR("Failed in killing %d.", (int) m_pid);

    waitpid(m_pid, NULL, 0);
}
