#include <chrono>
#include <fstream>
#include <string>
#include <thread>

int main (int argc, char* argv[])
{
    std::string pluginPath;
    std::string outputPath;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument (argv[index]);
        if (argument == "--scan-vst3" && index + 1 < argc)
            pluginPath = argv[++index];
        else if (argument == "--output" && index + 1 < argc)
            outputPath = argv[++index];
    }

    if (pluginPath.find ("Timeout") != std::string::npos)
    {
        std::this_thread::sleep_for (std::chrono::seconds (10));
        return 0;
    }
    if (pluginPath.find ("Malformed") != std::string::npos)
    {
        std::ofstream output (outputPath, std::ios::binary);
        output << "not a scanner result";
        return output.good() ? 0 : 92;
    }
    if (pluginPath.find ("Missing Output") != std::string::npos)
        return 0;

    return 91;
}
