// percentile_finder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <algorithm>
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

uint16_t STARTING_MASKING_BITS = 8192;

int parse_arguments(std::string* file_name, std::double_t* percentile, uint8_t* execution_mode, int argc, char* argv[]) {
    std::string exec_mode;
    if (argc != 4) {
        std::cout << "Not enough arguments - run with /h for more info";
        return -1;
    }
    else {
        *file_name = argv[1];
        *percentile = atof(argv[2]);
        exec_mode = argv[3];
    }

    if (exec_mode == "single") {
        *execution_mode = 0;
    } else if (exec_mode == "SMP") {
        *execution_mode = 1;
    }
    else if (execution_mode != NULL) {
        *execution_mode = 2;
    } else {
        return -1;
    }

    std::cout << "File_name: " + *file_name + "\n";
    std::cout << "Percentile:" << percentile << "\n";
    std::cout << "Exec_mode: " + exec_mode + "\n";
    std::cout << "Hello World!\n";
    return 0;
}

int main(int argc, char* argv[])
{
    std::string file_name;
    std::double_t looked_up_percentile;
    uint8_t execution_mode;
    uint64_t max_size = 104857600;

    if (parse_arguments(&file_name, &looked_up_percentile, &execution_mode, argc, argv)) {
        return -1;
    };
    int errno;


    // open the file:
    std::ifstream file(file_name, std::ios::binary);

    // get its size:
    file.seekg(0, std::ios::end);
    uint64_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);
    int iterations = ceil(filesize / max_size);
    uint64_t vectorsize = max_size / 4;



    std::vector<uint64_t> frequencies(STARTING_MASKING_BITS);

    for (int i = 0; i < iterations; i++) {
        uint64_t to_read = ((i + 1 * max_size) > filesize) ? (filesize - (i * max_size)) : max_size;
        std::vector<double> fileData(vectorsize);
        file.read((char*)&fileData[0], max_size);

        for (int i = 0; i < fileData.size(); i++) {
            if (std::fpclassify(fileData[i]) == FP_NORMAL || std::fpclassify(fileData[i]) == FP_ZERO) {
                uint64_t number = (uint64_t)fileData[i];
                uint16_t index = number >> 51;
                frequencies[index]++;
            }
        }
    }

    double lastPercentile = 0;
    uint64_t numbersCount = 0;
    for (int i = 0; i < frequencies.size(); i++) {
        numbersCount += frequencies[i];
    }

    std::vector<double> percentilesForFrequencies(STARTING_MASKING_BITS);
    uint16_t index = 0;
    for (int i = 0; i < frequencies.size(); i++) {
        double percentile = lastPercentile + ((frequencies[i] / (double)numbersCount) * 100);
        percentilesForFrequencies[i] = lastPercentile;
        lastPercentile = percentile;
        if (percentile > looked_up_percentile) {
            index = i;
            break;
        }
    }

    uint64_t lowest_number = 0, highest_number = 0;
    if (frequencies[index] > max_size) {
        //prvni pruchod -> furt je to moc velke !
        lowest_number = ((uint64_t)index << 51)  + 0x0000000000000000;
        highest_number = ((uint64_t)index << 51) + 0x0007FFFFFFFFFFFF;
    }

    frequencies = std::vector<uint64_t>(8388608);
    file.seekg(0, std::ios::beg);
    for (int i = 0; i < iterations; i++) {
        uint64_t to_read = ((i + 1 * max_size) > filesize) ? (filesize - (i * max_size)) : max_size;
        std::vector<double> fileData(vectorsize);
        file.read((char*)&fileData[0], max_size);

        for (int i = 0; i < fileData.size(); i++) {
            if ((std::fpclassify(fileData[i]) == FP_NORMAL || std::fpclassify(fileData[i]) == FP_ZERO) && (fileData[i] >= lowest_number && fileData[i] <= highest_number)) {
                uint64_t number = (uint64_t)fileData[i];
                uint32_t index = number << 11;
                index = index >> 30;
                frequencies[index]++;
            }
        }
    }
    do
    {
        std::cout << '\n' << "Press a key to continue...";
    } while (std::cin.get() != '\n');

    



    // read the data:
    
    
    /*
    for (int i = 0; i < fileData.size(); i++)
    {
        if (std::fpclassify(fileData[i]) == FP_NORMAL || std::fpclassify(fileData[i]) == FP_ZERO) {
            break;
        }
        else {
            fileData.erase(fileData.begin() + 1);
        }
    }
    std::sort(fileData.begin(), fileData.end());
    uint64_t index = percentile / 100 * fileData.size();
    */
    /*
    for (int i = 0; i < fileData.size(); i++) {
        uint64_t number = (uint64_t)fileData[i];
        uint16_t index = number >> 53;
        frequencies[index]++;
    }

    double lastPercentile = 0;
    uint64_t numbersCount = 0;
    for (int i = 0; i < frequencies.size(); i++) {
        numbersCount += frequencies[i];
    }
    std::vector<double> percentilesForFrequencies(2048);
    for (int i = 0; i < frequencies.size(); i++) {
        double percentile = lastPercentile + ((frequencies[i] / (double)numbersCount) * 100);
        percentilesForFrequencies[i] = lastPercentile;
        lastPercentile = percentile;
    }
    printf("Hledaný percentil:%f \n", fileData[index]);
    do
    {
        std::cout << '\n' << "Press a key to continue...";
    } while (std::cin.get() != '\n');*/

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
