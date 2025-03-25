#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <locale>

// For convenience
namespace fs = std::filesystem;
using n_type = uint_fast32_t;

static std::string formatWithCommas(const n_type value)
{
    std::stringstream ss;
    // Imbue the stream with the user’s default locale, 
    // which often uses commas for thousands separators in en_US.
    ss.imbue(std::locale(""));
    ss << value;  // The locale should insert thousands separators
    return ss.str();
}

/**
 * Get the current system local time broken into seconds.
 */
static n_type getCurrentSecond()
{
    auto now     = std::chrono::system_clock::now();
    std::time_t t= std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
    localtime_r(&t, &localTm);
    return localTm.tm_sec; 
}

/**
 * Return a string representing local date/time in a default style,
 * e.g. "2025-03-16 07:14:02".
 */
static std::string dateTimeToString(const std::chrono::system_clock::time_point &tp)
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm localTm{};
    localtime_r(&t, &localTm);

    // Example: "2025-03-16 07:14:02"
    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/**
 * Returns a filename-friendly timestamp in 12-hour format, e.g. "20250316_07_14_02".
 * The C# code uses "yyyyMMdd_hh_mm_ss" with a 12-hour 'hh' (leading zero if needed).
 */
static std::string getFileTimestamp()
{
    auto now     = std::chrono::system_clock::now();
    std::time_t t= std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
    localtime_r(&t, &localTm);

    // 12-hr format => %I, minutes => %M, seconds => %S
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%I_%M_%S", &localTm);
    return std::string(buf);
}

int main()
{
    // 1) Build directory paths
    fs::path currentDir = fs::current_path();
    fs::path logDetailDir = currentDir / "CycleLogDetail";
    fs::path iterationDir = currentDir / "CycleLog";

    // 2) Create directories if needed
    fs::create_directories(logDetailDir);
    fs::create_directories(iterationDir);

    // 3) Verify existence
    if(!fs::exists(logDetailDir) || !fs::exists(iterationDir))
    {
        std::cerr << "Directories do not exist\nMissing:\n";
        if(!fs::exists(logDetailDir))  std::cerr << logDetailDir << "\n";
        if(!fs::exists(iterationDir))  std::cerr << iterationDir << "\n";
        return 1;
    }

    // Construct path to "Iteration.txt"
    fs::path iterationLogPath = iterationDir / "Iteration.txt";

    // 4) Print banner
    std::cout << std::string(50, '*') << "\n";
    std::cout << "Gautier Iteration Test\n";
    std::cout << "Provides an informal assessment of operations per second on a given system\n";
    std::cout << "Essentially how fast can C++ code execute today\n";
    std::cout << "Helps in building better estimates for capacity planning and design\n";
    std::cout << std::string(50, '*') << "\n";
    std::cout << "How many times you want the test to run?\n";
    std::cout << "Type number then <enter>:  ";

    // 5) Get user input: number of cycles
    n_type cycles = 1u;
    std::cin >> cycles;
    if(!std::cin.good() || cycles < 1)
    {
        std::cerr << "Invalid input; defaulting to 1.\n";
        cycles = 1u;
    }

    std::cout << "Running " << cycles << " test runs\n";

    // Append initial info to iteration log
    {
        std::ofstream itLog(iterationLogPath, std::ios::app);
        itLog << std::string(33, '*') << "\n";
        itLog << "Cycles: " << cycles << "\t" 
              << dateTimeToString(std::chrono::system_clock::now()) << "\n";
        itLog << std::string(28, '*') << "\n";
    }

    // For final summary
    n_type sumOfIterations = 0;
    auto cycleStartTime = std::chrono::system_clock::now();

    // 6) Loop over cycles
    for(n_type cycle = 1u; cycle <= cycles; ++cycle)
    {
        std::cout << std::string(76, '*') << "\n";
        std::cout << "Running Cycle " 
                  << std::setw(2) << std::setfill('0') << cycle 
                  << " of " 
                  << std::setw(2) << std::setfill('0') << cycles << "\n";
        std::cout << std::string(44, '*') << "\n";

        // Build detail file name: "T yyyyMMdd_hh_mm_ss XX - YY.txt"
        // Example: "T 20250316_07_14_02 03 - 01.txt"
        std::ostringstream fname;
        fname << "T " << getFileTimestamp() << " "
              << std::setw(2) << std::setfill('0') << cycles << " - "
              << std::setw(2) << std::setfill('0') << cycle << ".txt";
        fs::path detailFilePath = logDetailDir / fname.str();

        // We'll collect log lines in a buffer (similar to StringBuilder in C#)
        std::ostringstream buffer;

        // 7) Pause logic if current second % 8 == 0
        n_type pauseMs = getCurrentSecond() * 100u;

        while(true)
        {
            int currentSec = getCurrentSecond();
            if(currentSec % 8u != 0u) {
                break;
            }
            std::cout << "Pausing for " << pauseMs << "ms\n";
            buffer << "Paused for " << pauseMs << "ms\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));

            // Recalc pauseMs after each sleep
            pauseMs = getCurrentSecond() * 100u;
        }

        // 8) Print "Ready to go ..."
        auto nowTp = std::chrono::system_clock::now();
        std::string nowStr = dateTimeToString(nowTp);
        std::cout << "Ready to go ... " << nowStr << "\n";

        // 9) Start measuring iteration in that same-second loop
        auto startTp    = std::chrono::system_clock::now();
        std::string startStr = dateTimeToString(startTp);
        int startSecond = getCurrentSecond();

        n_type iterations = 0u;
        n_type i = 0u;
        n_type currentSecond = startSecond;
        
        // Loop while currentSecond == startSecond
        while(currentSecond == startSecond)
        {
            iterations = i + 1u;  // Matches "Iterations = 1 + i" from C#
            ++i;

            // If i is multiple of 100K, update currentSecond, log progress
            if((i % 100000u) == 0u)
            {
                currentSecond = getCurrentSecond();
                auto progressTp = std::chrono::system_clock::now();
                std::string progTimeStr = dateTimeToString(progressTp);
                buffer << "Cycle " << formatWithCommas(cycle) << " of " << formatWithCommas(cycles) 
                          << " Iteration " << formatWithCommas(i) << " " << progTimeStr << "\n";
            }
            // Re-check second in case it changes mid-iteration
            currentSecond = getCurrentSecond();
        }

        // Add to global sum
        sumOfIterations += iterations;

        auto endTp = std::chrono::system_clock::now();
        std::string endStr = dateTimeToString(endTp);

        // 10) Print iteration results to console
        buffer << "Iterations " << formatWithCommas(iterations)
               << " Start " << startStr
               << " ... End " << endStr << "\n";

        // Show path to detail file
        std::cout << detailFilePath.string() << "\n";

        // Write buffer to detail file
        {
            std::ofstream ofs(detailFilePath);
            if(ofs.is_open()) {
                ofs << buffer.str();
            }
        }

	std::cout << buffer.str();

        // Append info to iteration log
        {
            std::ofstream itLog(iterationLogPath, std::ios::app);
            itLog << "***\t" << cycle << "\t"
                  << std::string(60, '*') << "\n";
            itLog << startStr << "\n";
            itLog << iterations << "\n";
            itLog << endStr << "\n\n";
        }
    }

    // 11) Final summary
    auto cycleEndTime = std::chrono::system_clock::now();
    std::chrono::duration<double> totalDiff = cycleEndTime - cycleStartTime;
    long long totalMs = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(totalDiff).count());

    // Break down into days/hours/min/sec/ms
    long long days    = totalMs / (1000LL * 60 * 60 * 24);
    long long rem     = totalMs % (1000LL * 60 * 60 * 24);
    long long hours   = rem / (1000LL * 60 * 60);
    rem              %= (1000LL * 60 * 60);
    long long minutes = rem / (1000LL * 60);
    rem              %= (1000LL * 60);
    long long seconds = rem / 1000LL;
    long long ms      = rem % 1000LL;

    std::cout << "******\tSum: " << formatWithCommas(sumOfIterations) 
              << " operations across " << formatWithCommas(cycles) << " cycles *********\n\n";

    n_type avgOpsPerSec = (cycles > 0) ? (sumOfIterations / cycles) : 0;
    std::cout << "Average: " << formatWithCommas(avgOpsPerSec) 
              << " operations per second **********\n\n";

    auto cycleStartStr = dateTimeToString(cycleStartTime);
    auto cycleEndStr   = dateTimeToString(cycleEndTime);

    std::cout << "Cycle started: " << cycleStartStr 
              << " ... Cycle ended: " << cycleEndStr
              << " **********\n";
    std::cout << "Time: " << days << " days " << hours << " hrs "
              << minutes << " min " << seconds << " sec " 
              << ms << " ms\n";

    // Write final summary to iteration log
    {
        std::ofstream itLog(iterationLogPath, std::ios::app);
        itLog << "******\tSum: " << sumOfIterations 
              << " operations across " << cycles << " cycles *********\n";
        itLog << "Cycle started: " << cycleStartStr 
              << " ... Cycle ended: " << cycleEndStr << " **********\n";
        itLog << "Average: " << avgOpsPerSec 
              << " operations per second **********\n";
        itLog << "Time: " << days << " days " << hours << " hrs "
              << minutes << " min " << seconds << " sec "
              << ms << " ms\n";
        itLog << std::string(33, '_') << "\n\n";
    }

    return 0;
}

