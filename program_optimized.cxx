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

// Optional: speed up iostream by decoupling from C stdio
/*
static struct IOSyncOff {
    IOSyncOff() {
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);
    }
} iosyncoff_instance;
*/

static const std::string formatWithCommas(const n_type &value)
{
    std::stringstream ss;

    ss.imbue(std::locale(""));

    ss << value;

    return ss.str();
}

/**
 * Return a string representing local date/time in a default style,
 * e.g. "2025-03-16 07:14:02".
 */
static std::string dateTimeToString(const std::chrono::system_clock::time_point &tp)
{
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);

    std::tm localTm{};

    localtime_r(&t, &localTm);

    std::ostringstream oss;

    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

/**
 * Returns a filename-friendly timestamp in 12-hour format, e.g. "20250316_07_14_02".
 */
static const std::string getFileTimestamp()
{
    const auto now = std::chrono::system_clock::now();

    const std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm localTm{};

    localtime_r(&t, &localTm);

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

    fs::path iterationLogPath = iterationDir / "Iteration.txt";

    // 4) Print banner
    std::cout << std::string(50, '*') << "\n";
    std::cout << "Gautier Iteration Test\n"
              << "Provides an informal assessment of operations per second on a given system\n"
              << "Essentially how fast can C++ code execute today\n"
              << "Helps in building better estimates for capacity planning and design\n"
              << std::string(50, '*') << "\n"
              << "How many times you want the test to run?\n"
              << "Type number then <enter>:  ";

    // 5) Get user input
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
    const auto cycleStartTime = std::chrono::system_clock::now();

    // 6) Loop over cycles
    for(n_type cycle = 1u; cycle <= cycles; ++cycle)
    {
        std::cout << std::string(76, '*') << "\n";
        std::cout << "Running Cycle "
                  << std::setw(2) << std::setfill('0') << cycle
                  << " of "
                  << std::setw(2) << std::setfill('0') << cycles << "\n";
        std::cout << std::string(44, '*') << "\n";

        // Build detail file name
        std::ostringstream fname;

        fname << "T " << getFileTimestamp() << " "
              << std::setw(2) << std::setfill('0') << cycles << " - "
              << std::setw(2) << std::setfill('0') << cycle << ".txt";

        fs::path detailFilePath = logDetailDir / fname.str();

        // We'll collect log lines in a buffer (like StringBuilder in C#)
        std::ostringstream buffer;

        // 7) Pause logic if local second % 8 == 0
        //    (This is also somewhat expensive, so we do it only once at start.)
        {
            auto nowSys   = std::chrono::system_clock::now();
            auto nowTimeT = std::chrono::system_clock::to_time_t(nowSys);

            std::tm localTm{};

            localtime_r(&nowTimeT, &localTm);

            while(localTm.tm_sec % 8 == 0)
            {
                // Sleep
                n_type pauseMs = static_cast<n_type>(localTm.tm_sec) * 100u;
                std::cout << "Pausing for " << pauseMs << "ms\n";
                buffer << "Paused for " << pauseMs << "ms\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));

                // Re-check
                nowSys  = std::chrono::system_clock::now();
                nowTimeT= std::chrono::system_clock::to_time_t(nowSys);
                localtime_r(&nowTimeT, &localTm);
            }
        }

        const auto nowTp  = std::chrono::system_clock::now();
        const auto startTp= std::chrono::steady_clock::now();  // Use steady_clock for iteration
        const auto endTp  = startTp + std::chrono::seconds(1); // We’ll loop for ~1 second

        const std::string nowStr   = dateTimeToString(nowTp);
        const std::string startStr = dateTimeToString(std::chrono::system_clock::now());

        std::cout << "Ready to go ... " << nowStr << "\n";

        // 9) Start measuring iteration
        n_type iterations = 0u;
        n_type i = 0u;

        // Loop until 1 second has elapsed
        while(std::chrono::steady_clock::now() < endTp)
        {
            iterations = i + 1u; 
            ++i;

            // If i is multiple of 100K, record progress
            if((i % 100000u) == 0u)
            {
                auto progressTp = std::chrono::system_clock::now();
                std::string progTimeStr = dateTimeToString(progressTp);
                buffer << "Cycle " << formatWithCommas(cycle) << " of " << formatWithCommas(cycles)
                       << " Iteration " << formatWithCommas(i) << " " << progTimeStr << "\n";
            }
        }

        sumOfIterations += iterations;

        const auto realEndSys = std::chrono::system_clock::now();
        const std::string endStr = dateTimeToString(realEndSys);

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
            itLog << "***\t" << cycle << "\t" << std::string(60, '*') << "\n";
            itLog << startStr << "\n";
            itLog << iterations << "\n";
            itLog << endStr << "\n\n";
        }
    }

    // 11) Final summary
    const auto cycleEndTime = std::chrono::system_clock::now();
    const std::chrono::duration<double> totalDiff = cycleEndTime - cycleStartTime;
    const long long totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalDiff).count();

    const long long days    = totalMs / (1000LL * 60 * 60 * 24);
    long long rem     = totalMs % (1000LL * 60 * 60 * 24);
    const long long hours   = rem / (1000LL * 60 * 60);
    rem              %= (1000LL * 60 * 60);
    const long long minutes = rem / (1000LL * 60);
    rem              %= (1000LL * 60);
    const long long seconds = rem / 1000LL;
    const long long ms      = rem % 1000LL;

    std::cout << "******\tSum: " << formatWithCommas(sumOfIterations)
              << " operations across " << formatWithCommas(cycles) << " cycles *********\n\n";

    const n_type avgOpsPerSec = (cycles > 0) ? (sumOfIterations / cycles) : 0;
    std::cout << "Average: " << formatWithCommas(avgOpsPerSec)
              << " operations per second **********\n\n";

    const auto cycleStartStr = dateTimeToString(cycleStartTime);
    const auto cycleEndStr   = dateTimeToString(cycleEndTime);

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

