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

#include <stdexcept>


Subprogram::Subprogram(const std::string& name, bool receiveData, bool feedData)
    : m_name(name)
    , m_pid(-1)
    , m_receiveData(receiveData)
    , m_feedData(feedData)
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
bool Subprogram::start(std::initializer_list<std::string> arguments, Environment& env)
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
    if (!m_receiveData)
        throw std::runtime_error("no input stream");
    return m_inputStream;
}

Stream& Subprogram::outputStream()
{
    if (!m_feedData)
        throw std::runtime_error("no output stream");
    return m_outputStream;
}

// プロセスID。
int Subprogram::pid()
{
    return m_pid;
}
