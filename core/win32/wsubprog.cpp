#include <stdexcept>

#include "subprog.h"
#include "str.h"

// Windows headers
#include <io.h>
#include <fcntl.h>

Subprogram::Subprogram(const std::string& name, bool receiveData, bool feedData)
    : m_name(name)
    , m_pid(-1)
    , m_receiveData(receiveData)
    , m_feedData(feedData)
    , m_processHandle(INVALID_HANDLE_VALUE)
{
}

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

static
std::string createCommandLine(std::string prog, std::vector<std::string> args)
{
    bool isCGI = str::has_suffix(str::upcase(prog), ".CGI");

    std::vector<std::string> words;
    if (isCGI)
        words.push_back("python\\python");

    words.push_back(prog);
    for (auto s : args)
        words.push_back(s);

    for (int i = 0; i < words.size(); ++i)
    {
        auto s = words[i];
        // Replace unsafe characters with a space.  The percent mark
        // '%' is intentionally allowed.
        for (int j = 0; j < s.size(); j++)
            if (s[j] == '!' || s[j] == '\"')
                s[j] = ' ';
        words[i] = "\"" + s + "\"";
    }
    return str::join(" ", words);
}

// プログラムの実行を開始。
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

    if (m_receiveData)
    {
        CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0);
        SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    }

    PROCESS_INFORMATION procInfo;
    STARTUPINFOA startupInfo;
    bool success;

    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    if (m_receiveData)
    {
        startupInfo.hStdError = stdoutWrite;
        startupInfo.hStdOutput = stdoutWrite;
    }
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    // 標準入力のハンドル指定しなくていいのかな？

    std::string cmdline = createCommandLine(m_name, arguments);
    LOG_DEBUG("cmdline: %s", str::inspect(cmdline).c_str());
    success = CreateProcessA(NULL,
                            const_cast<char*>( cmdline.c_str() ),
                            NULL,
                            NULL,
                            TRUE,
                            CREATE_NO_WINDOW,
                            (PVOID) env.windowsEnvironmentBlock().c_str(),
                            NULL,
                            &startupInfo,
                            &procInfo);

    if (!success)
        return false;

    m_processHandle = procInfo.hProcess;
    m_pid = GetProcessId(procInfo.hProcess);

    if (m_receiveData)
    {
        int fd = _open_osfhandle((intptr_t) stdoutRead, _O_RDONLY);
        m_inputStream.openReadOnly(fd);

        CloseHandle(stdoutWrite);
    }
    CloseHandle(procInfo.hThread);
    return true;
}

// プログラムにより終了ステータスが返された場合は true を返し、ステー
// タスが *status にセットされる。
bool Subprogram::wait(int* status)
{
    DWORD res;
    DWORD exitCode;

    res = WaitForSingleObject(m_processHandle, INFINITE);
    if (res != WAIT_OBJECT_0)
    {
        LOG_ERROR("Process wait failed.");
        abort();
    }

    if (GetExitCodeProcess(m_processHandle, &exitCode) == 0)
    {
        LOG_ERROR("Failed to get exit code of child process.");
        abort();
    }else
    {
        if (exitCode == STILL_ACTIVE)
            abort(); // something is very wrong

        *status = exitCode;
        CloseHandle(m_processHandle);
        m_processHandle = INVALID_HANDLE_VALUE;
        m_pid = -1;
        return true;
    }
}

bool Subprogram::isAlive()
{
    if (m_processHandle == INVALID_HANDLE_VALUE)
        return false;

    DWORD exitCode;
    if (GetExitCodeProcess(m_processHandle, &exitCode) == 0)
    {
        LOG_ERROR("GetExitCodeProcess: error code = %d\n", (int) GetLastError());
        abort();
    }
    if (exitCode != STILL_ACTIVE)
    {
        LOG_DEBUG("Child process exited with error code %d\n", (int) exitCode);
        CloseHandle(m_processHandle);
        m_processHandle = INVALID_HANDLE_VALUE;
        return false;
    }else
        return true;
}

void Subprogram::terminate()
{
    if (m_processHandle == INVALID_HANDLE_VALUE)
        return;

    if (TerminateProcess(m_processHandle, 1) == 0)
    {
        LOG_ERROR("TerminateProcess: error code = %d\n", (int) GetLastError());
        abort();
    }
    CloseHandle(m_processHandle);
    m_processHandle = INVALID_HANDLE_VALUE;
    m_pid = -1;
}
