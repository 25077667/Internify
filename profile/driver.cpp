#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <internify.hpp>

// Function declarations
bool parseInputLine(const std::string &line, std::string &base_string, char &times_operator, int &multiplier);
void processInternifyOperations(scc::Internify<std::string> &internify, const std::string &base_string, int multiplier);
void handleFileProcessing(const std::string &filename, scc::Internify<std::string> &internify);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_data_file>" << std::endl;
        return 1;
    }

    // Instantiate the Internify library
    scc::Internify<std::string> internify;

    // Process the input file
    handleFileProcessing(argv[1], internify);

    // Optionally, print the size of the interned map
    std::cout << "Final size of interned map: " << internify.size() << std::endl;

    return 0;
}

// Function to parse a line of input
bool parseInputLine(const std::string &line, std::string &base_string, char &times_operator, int &multiplier)
{
    std::istringstream iss(line);
    if (!(iss >> std::quoted(base_string) >> times_operator >> multiplier))
    {
        std::cerr << "Error: Invalid input format: " << line << std::endl;
        return false;
    }

    if (times_operator != '*')
    {
        std::cerr << "Error: Expected '*' operator in input: " << line << std::endl;
        return false;
    }

    return true;
}

// Function to process intern/release operations based on the multiplier
void processInternifyOperations(scc::Internify<std::string> &internify, const std::string &base_string, int multiplier)
{
    if (multiplier > 0)
    {
        for (int i = 0; i < multiplier; ++i)
        {
            internify.internify(base_string);
        }
    }
    else if (multiplier < 0)
    {
        for (int i = 0; i < -multiplier; ++i)
        {
            internify.release(base_string);
        }
    }
}

// Function to handle processing of the input file
void handleFileProcessing(const std::string &filename, scc::Internify<std::string> &internify)
{
    std::ifstream infile(filename);
    if (!infile)
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(infile, line))
    {
        std::string base_string;
        char times_operator;
        int multiplier;

        if (parseInputLine(line, base_string, times_operator, multiplier))
        {
            processInternifyOperations(internify, base_string, multiplier);
        }
    }
}