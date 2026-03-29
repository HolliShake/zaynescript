#include "src/compiler.h"
#include "src/global.h"
#include "src/interpreter.h"
#include "src/lexer.h"
#include "src/parser.h"

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

String ReadInternalFile(String path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    fprintf(stderr, "Error: Could not open file '%s'\n", path);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(size + 1);
  if (!buffer) {
    fprintf(stderr, "Error: Could not allocate memory for file '%s'\n", path);
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, size, file);
  buffer[size] = '\0';

  fclose(file);
  return buffer;
}

#ifdef _WIN32
int RunTestInProcess(const char *testPath, const char *exePath) {
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  // Build command line
  char command[1024];
  snprintf(command, sizeof(command), "\"%s\" --run \"%s\"", exePath, testPath);

  // Create the child process
  if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si,
                      &pi)) {
    fprintf(stderr, "  FAILED: Could not create process (error %lu)\n",
            GetLastError());
    return EXIT_FAILURE;
  }

  // Wait for the process to complete
  WaitForSingleObject(pi.hProcess, INFINITE);

  // Get exit code
  DWORD exitCode;
  GetExitCodeProcess(pi.hProcess, &exitCode);

  // Close process and thread handles
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return (int)exitCode;
}
#else
int RunTestInProcess(const char *testPath, const char *exePath) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    fprintf(stderr, "  FAILED: Could not create pipe\n");
    return EXIT_FAILURE;
  }

  pid_t pid = fork();
  if (pid == -1) {
    fprintf(stderr, "  FAILED: Could not fork process\n");
    close(pipefd[0]);
    close(pipefd[1]);
    return EXIT_FAILURE;
  }

  if (pid == 0) {
    // Child process
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    execl(exePath, exePath, "--run", testPath, NULL);
    exit(EXIT_FAILURE);
  } else {
    // Parent process
    close(pipefd[1]);

    char buffer[256];
    ssize_t bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytesRead] = '\0';
      printf("%s", buffer); // Removed the prepended spaces that randomly
                            // interrupt chunks and break ANSI codes
      fflush(stdout);
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
    return EXIT_FAILURE;
  }
}
#endif

void RunTests() {
  String testsPath = "./tests/";

  DIR *dir = opendir(testsPath);
  if (!dir) {
    // Fall back to installed lib path
    testsPath = "/usr/local/lib/zscript/tests/";
    dir = opendir(testsPath);
  }
  if (!dir) {
    fprintf(stderr, "Error: Could not open tests directory '%s'\n", testsPath);
    return;
  }

  struct dirent *entry;
  int testCount = 0;
  int passedCount = 0;
  int failedCount = 0;

// Track failed tests
#define MAX_FAILED_TESTS 256
  char *failedTests[MAX_FAILED_TESTS];
  int failedTestsCount = 0;

  // Get executable path
  char exePath[512];
#ifdef _WIN32
  GetModuleFileNameA(NULL, exePath, sizeof(exePath));
  // Convert forward slashes to backslashes for Windows
  for (char *p = exePath; *p; p++) {
    if (*p == '/')
      *p = '\\';
  }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) ||    \
    defined(__NetBSD__)
// macOS and BSD systems
#if defined(__APPLE__)
  uint32_t size = sizeof(exePath);
  if (_NSGetExecutablePath(exePath, &size) != 0) {
    strcpy(exePath, "./main");
  }
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  int mib[4];
  size_t len = sizeof(exePath);
  mib[0] = CTL_KERN;
#if defined(__FreeBSD__)
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
#elif defined(__NetBSD__)
  mib[1] = KERN_PROC_ARGS;
  mib[2] = -1;
  mib[3] = KERN_PROC_PATHNAME;
#else // OpenBSD
  mib[1] = KERN_PROC_ARGS;
  mib[2] = getpid();
  mib[3] = KERN_PROC_ARGV;
#endif
  if (sysctl(mib, 4, exePath, &len, NULL, 0) != 0) {
    strcpy(exePath, "./main");
  }
#endif
#else
  // Linux and other Unix-like systems
  ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
  if (len != -1) {
    exePath[len] = '\0';
  } else {
    strcpy(exePath, "./main");
  }
#endif

  printf("=== Running Tests ===\n\n");

  while ((entry = readdir(dir)) != NULL) {
    // Skip . and .. directories
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Check if file has .zs extension
    size_t nameLen = strlen(entry->d_name);
    if (nameLen < 3 || strcmp(entry->d_name + nameLen - 3, ".zs") != 0) {
      continue;
    }

    // Exclude interactive/long-running visualization test scripts
    if (strcmp(entry->d_name, "test_doughnut.zs") == 0 ||
        strcmp(entry->d_name, "test_cube.zs") == 0 ||
        strcmp(entry->d_name, "test_star.zs") == 0 ||
        strcmp(entry->d_name, "test_diamond.zs") == 0) {
      continue;
    }

    // Build full path with proper directory separator
    char fullPath[512];
#ifdef _WIN32
    snprintf(fullPath, sizeof(fullPath), "%s%s", testsPath, entry->d_name);
#else
    snprintf(fullPath, sizeof(fullPath), "%s%s", testsPath, entry->d_name);
#endif

    printf("Running test: %s\n", fullPath);
    testCount++;

    int result = RunTestInProcess(fullPath, exePath);

    if (result == EXIT_SUCCESS) {
      printf("  PASSED\n\n");
      passedCount++;
    } else {
      printf("  FAILED (exit code: %d)\n\n", result);
      failedCount++;

      // Store failed test name
      if (failedTestsCount < MAX_FAILED_TESTS) {
        failedTests[failedTestsCount] = strdup(fullPath);
        failedTestsCount++;
      }
    }
  }

  closedir(dir);

  printf("=== Test Results ===\n");
  printf("Total: %d, Passed: %d, Failed: %d\n", testCount, passedCount,
         failedCount);

  // Print list of failed tests
  if (failedTestsCount > 0) {
    printf("\n=== Failed Tests ===\n");
    for (int i = 0; i < failedTestsCount; i++) {
      printf("  - %s\n", failedTests[i]);
      free(failedTests[i]);
    }
  }
}

void PrintHelp() {
#ifdef _WIN32
  // Set console to UTF-8 on Windows
  SetConsoleOutputCP(CP_UTF8);
#endif

  printf("╔════════════════════════════════════════════════════════════════════"
         "══════════════════════╗\n");
  printf("║                                                                    "
         "                      ║\n");
  printf("║  ███████╗ █████╗ ██╗   ██╗███╗   ██╗███████╗███████╗ "
         "██████╗██████╗ ██╗██████╗ ████████╗ ║\n");
  printf("║  ╚══███╔╝██╔══██╗╚██╗ ██╔╝████╗  "
         "██║██╔════╝██╔════╝██╔════╝██╔══██╗██║██╔══██╗╚══██╔══╝ ║\n");
  printf("║    ███╔╝ ███████║ ╚████╔╝ ██╔██╗ ██║█████╗  ███████╗██║     "
         "██████╔╝██║██████╔╝   ██║    ║\n");
  printf("║   ███╔╝  ██╔══██║  ╚██╔╝  ██║╚██╗██║██╔══╝  ╚════██║██║     "
         "██╔══██╗██║██╔═══╝    ██║    ║\n");
  printf("║  ███████╗██║  ██║   ██║   ██║ ╚████║███████╗███████║╚██████╗██║  "
         "██║██║██║        ██║    ║\n");
  printf("║  ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═══╝╚══════╝╚══════╝ ╚═════╝╚═╝  "
         "╚═╝╚═╝╚═╝        ╚═╝    ║\n");
  printf("║                                                                    "
         "                      ║\n");
  printf("║                                A Custom Programming Language       "
         "                      ║\n");
  printf("║                                      Implemented in C              "
         "                      ║\n");
  printf("║                                                                    "
         "                      ║\n");
  printf("║  Features: Dynamic Typing • Functions • Arrays • Objects • Classes "
         "                      ║\n");
  printf("║  License:  MIT License                                             "
         "                      ║\n");
  printf("║  Author:   Philipp Andrew Redondo                                  "
         "                      ║\n");
  printf("║                                                                    "
         "                      ║\n");
  printf("║  usage: zscript [--run <file.zs> | --tests | --help]               "
         "                      ║\n");
  printf("╚════════════════════════════════════════════════════════════════════"
         "══════════════════════╝\n");
  printf("\n");
  printf("USAGE:\n");
  printf("  %s [OPTIONS] [FILE]\n", "zscript");
  printf("\n");
  printf("OPTIONS:\n");
  printf("  --tests              Run all test files in ./tests/ directory\n");
  printf("  --run <file>         Run a specific .zs file\n");
  printf("  --help, -h           Display this help message\n");
  printf("\n");
  printf("EXAMPLES:\n");
  printf("  %s --tests           # Run all tests\n", "zscript");
  printf("  %s --run script.zs   # Run script.zs\n", "zscript");
  printf("\n");
}

// Simple example using the allocator system
int main(int argc, char **argv) {
#ifdef _WIN32
  // Enable ANSI escape sequence processing on Windows
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE) {
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
      // ENABLE_VIRTUAL_TERMINAL_PROCESSING is 0x0004
      SetConsoleMode(hOut, dwMode | 0x0004);
    }
  }
#endif

  // Check if help flag is provided
  if (argc > 1 &&
      (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    PrintHelp();
    return EXIT_SUCCESS;
  }

  // Check if --tests flag is provided
  if (argc > 1 && strcmp(argv[1], "--tests") == 0) {
    RunTests();
    return EXIT_SUCCESS;
  }

  // Exe path
  char execBuf[512];
  String execPath = NULL;
#ifdef _WIN32
  GetModuleFileNameA(NULL, execBuf, sizeof(execBuf));
  for (char *p = execBuf; *p; p++) {
    if (*p == '/')
      *p = '\\';
  }
#else
  ssize_t execLen = readlink("/proc/self/exe", execBuf, sizeof(execBuf) - 1);
  if (execLen != -1) {
    execBuf[execLen] = '\0';
  } else {
    strcpy(execBuf, argv[0]);
  }
#endif
  // Extract directory from executable path
  {
    char *lastSep = strrchr(execBuf, '/');
#ifdef _WIN32
    char *lastBackSep = strrchr(execBuf, '\\');
    if (lastBackSep > lastSep)
      lastSep = lastBackSep;
#endif
    if (lastSep) {
      *(lastSep + 1) = '\0';
    }
    execPath = AllocateString(execBuf);
  }

  // Check if --run flag is provided with a file path
  String path = NULL;
  if (argc > 2 && strcmp(argv[1], "--run") == 0) {
    path = AllocateString(argv[2]);
  } else {
    // No arguments provided or invalid arguments - show help
    PrintHelp();
    return EXIT_SUCCESS;
  }

  Interpreter *interpreter = CreateInterpreter(execPath);

  // NOTE: memory leak (ReadFile allocates a buffer, StringToRunes reads it, but
  // the buffer is never freed)
  String fileContent = ReadInternalFile(path);
  if (!fileContent) {
    free(execPath);
    free(path);
    ForceGarbageCollect(interpreter);
    FreeInterpreter(interpreter);
    return EXIT_FAILURE;
  }
  Rune *data = StringToRunes(fileContent);
  free(fileContent);

  Lexer *lexer = CreateLexer(path, data);
  Parser *parser = CreateParser(lexer);

  Compiler *compiler = CreateCompiler(interpreter, parser);
  Value *compiled = Compile(compiler);

  Interpret(interpreter, compiled);

  FreeLexer(lexer);
  FreeParser(parser);
  FreeCompiler(compiler);
  FreeInterpreter(interpreter);
  free(execPath);
  free(path);
  free(data);
  printf("Program Finished!\n");
  return EXIT_SUCCESS;
}