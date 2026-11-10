// Lanceur des tests Qt Test sous ctest, sous Windows seulement (actcraft_add_test).
// Usage : actcraft_test_launcher <journal> <exécutable> [arguments...]
//
// ctest crée ses processus sans console. Qt Test envoie alors sa sortie standard à OutputDebugString
// (QPlainTestLogger::outputMessage) : un test rouge ne laisse rien dans les journaux de ctest. Le lanceur
// demande donc à Qt la sortie standard (QT_FORCE_STDERR_LOGGING), fait écrire au test <journal>, puis y
// ajoute son code de sortie. Un journal sans ligne « code de sortie » : le lanceur n'a pas fini (délai de
// ctest, job annulé). Sans Qt : un test qui ne démarre pas (DLL absente) laisse quand même son code.

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <windows.h>

namespace {

// Saute un argument de la ligne de commande brute : la suite est transmise au test sans la réécrire.
const wchar_t* skipArgument(const wchar_t* cursor)
{
    while (*cursor == L' ' || *cursor == L'\t')
        ++cursor;
    bool quoted = false;
    for (; *cursor != L'\0'; ++cursor) {
        if (*cursor == L'"')
            quoted = !quoted;
        else if (!quoted && (*cursor == L' ' || *cursor == L'\t'))
            break;
    }
    return cursor;
}

void appendToJournal(const wchar_t* journal, const char* line)
{
    FILE* file = nullptr;
    if (_wfopen_s(&file, journal, L"ab") != 0 || file == nullptr)
        return;
    std::fputs(line, file);
    std::fclose(file);
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (argc < 3) {
        std::fputs("usage : actcraft_test_launcher <journal> <executable> [arguments...]\n", stderr);
        return 2;
    }
    const wchar_t* journal = argv[1];
    std::error_code ignored;
    std::filesystem::create_directories(std::filesystem::path(journal).parent_path(), ignored);

    SetEnvironmentVariableW(L"QT_FORCE_STDERR_LOGGING", L"1");

    // Deux sorties : le fichier du test, et la sortie standard que ctest capte. Après les arguments du test :
    // le premier peut être une clé (tst_ui_lot7 <clé>).
    const wchar_t* rest = skipArgument(skipArgument(skipArgument(GetCommandLineW())));
    std::wstring command = L"\"";
    command += argv[2];
    command += L"\"";
    command += rest;
    command += L" -o \"";
    command += journal;
    command += L",txt\" -o -,txt";

    // ctest ne tue que son fils direct quand le délai est dépassé : le travail emporte le test avec le
    // lanceur.
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (job != nullptr)
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof limits);

    STARTUPINFOW startup{};
    startup.cb = sizeof startup;
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_SUSPENDED, nullptr, nullptr,
                        &startup, &process)) {
        char line[80];
        std::snprintf(line, sizeof line, "\nlancement impossible : erreur Windows %lu\n", GetLastError());
        appendToJournal(journal, line);
        std::fputs(line, stderr);
        return 2;
    }
    if (job != nullptr)
        AssignProcessToJobObject(job, process.hProcess);
    ResumeThread(process.hThread);
    CloseHandle(process.hThread);

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hProcess);

    char line[80];
    std::snprintf(line, sizeof line, "\ncode de sortie : %lu (0x%08lX)\n", exitCode, exitCode);
    appendToJournal(journal, line);
    return static_cast<int>(exitCode);
}
