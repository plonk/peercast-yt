#ifdef _UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "subprog.h"
#include "env.h"

Subprogram::Subprogram(const std::string& name, Environment& env)
    : m_name(name)
    , m_pid(-1)
    , m_env(env)
#ifdef WIN32
    , m_processHandle(INVALID_HANDLE_VALUE)
#endif
{
}

#ifdef _UNIX
Subprogram::~Subprogram()
{
}
#endif
#ifdef WIN32
Subprogram::~Subprogram()
{
    if (m_processHandle != INVALID_HANDLE_VALUE)
    {
        BOOL success;

        success = CloseHandle(m_processHandle);
        if (success)
            LOG_DEBUG("Process handle closed.");
        else
            LOG_DEBUG("Failed to close process handle.");
    }
}
#endif

// プログラムの実行を開始。
#ifdef WIN32
extern "C" {
    extern DWORD WINAPI GetProcessId(HANDLE Process);
}
bool Subprogram::start()
{
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE stdoutRead;
    HANDLE stdoutWrite;

    CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0);
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION procInfo;
    STARTUPINFO startupInfo;
    bool success;

    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.hStdError = stdoutWrite;
    startupInfo.hStdOutput = stdoutWrite;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    // 標準入力のハンドル指定しなくていいのかな？

    success = CreateProcess(NULL,
                            (char*) ("python\\python \"" + m_name + "\"").c_str(),
                            NULL,
                            NULL,
                            TRUE,
                            0,
                            (PVOID) m_env.windowsEnvironmentBlock().c_str(),
                            NULL,
                            &startupInfo,
                            &procInfo);

    if (!success)
        return false;

    m_processHandle = procInfo.hProcess;
    m_pid = GetProcessId(procInfo.hProcess);

    int fd = _open_osfhandle((intptr_t) stdoutRead, _O_RDONLY);
    m_inputStream.openReadOnly(fd);

    CloseHandle(stdoutWrite);
    CloseHandle(procInfo.hThread);
    return true;
}
#endif
#ifdef _UNIX
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
                       m_env.env());
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
#endif

// プログラムにより終了ステータスが返された場合は true を返し、ステー
// タスが *status にセットされる。
#ifdef WIN32
bool Subprogram::wait(int* status)
{
    DWORD res;
    DWORD exitCode;

    res = WaitForSingleObject(m_processHandle, 10 * 1000);
    if (res == WAIT_TIMEOUT)
        LOG_ERROR("Process wait timeout.");
    else if (res == WAIT_FAILED)
        LOG_ERROR("Process wait failed.");

    if (!GetExitCodeProcess(m_processHandle, &exitCode))
    {
        LOG_ERROR("Failed to get exit code of child process.");
        return false;
    }else
    {
        *status = exitCode;
        return true;
    }
}
#endif
#ifdef _UNIX
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
#endif

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
