
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <execution>
#include "number_masker.h"

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

std::vector<double_t> get_histogram(uint64_t max_size, int iterations, uint64_t filesize, std::ifstream& file, double looked_up_percentile) {
    uint64_t vectorsize = filesize < max_size ? (uint64_t)ceil(filesize / 8.0) : (uint64_t)max_size / 8;
    uint64_t numbersCount = 0;
    auto phase_vectorsize = vector_size();
    std::vector<uint64_t> frequencies(vector_size());
    std::vector<double> fileData(vectorsize);
    auto low = get_lowest_possible_number();
    auto high = get_highest_possible_number();
    auto ff = file.tellg();
    auto ienqkwn = 0;
    if (file.fail()) {
        file.clear();
    }
    file.seekg(0);

    for (int i = 0; i < iterations; i++) {
        uint64_t actual_position = file.tellg();
        uint64_t to_read = ((i + 1 * max_size) > filesize) ? (filesize - (i * max_size)) : max_size;
        file.read((char*)&fileData[0], to_read);
        uint32_t size = (uint32_t)(to_read + to_read % 8) / 8;

        for (int j = 0; j < size; j++) {
            if ((std::fpclassify(fileData[i]) == FP_NORMAL || std::fpclassify(fileData[i]) == FP_ZERO)) {
                double_t doublicek = fileData[i];
                int64_t number = *(int64_t*)&doublicek;
                uint32_t index = return_index(number);
                doublicek = *(double*)&number;
                frequencies[index]++;
                numbersCount++;
            }
        }
    }

    //vyhodnoceni
    auto total_of_numbers = get_max_count() == 0 ? numbersCount : get_max_count();
    uint64_t numbers_before = get_numbers_before();
    uint64_t counter = numbers_before;
    double last_percentile = ((double)numbers_before / total_of_numbers) * 100;
    uint32_t index = 0;
    auto phase = get_phase();


    for (int i = 0; i < frequencies.size(); i++) {
        counter += frequencies[i];

        double percentile = ((double)(counter) / total_of_numbers) * 100;
        if (percentile > looked_up_percentile && last_percentile < looked_up_percentile) {
            index = i;
            break;
        }
        last_percentile = percentile;
        numbers_before += frequencies[i];
    }

    
    // vejde se to tam ?
    if (frequencies[index] > max_size && phase < 2) {
        //NE ?prvni pruchod -> furt je to moc velke !
        increment_phase(index, numbersCount, numbers_before);
        return std::vector<double_t>(0);
    }
    else {
        //vejde se to tam - potØebujem najít ten prvek a jeho pozice
        uint64_t number_of_elements = counter - numbers_before;
        auto number_of_elems = frequencies[index];
        auto phase_vectorsize = vector_size();
        auto phase = get_phase();
        std::vector<int64_t>  numbers(0);
        std::vector<uint64_t> first_encounter(phase_vectorsize);
        std::vector<uint64_t> last_encounter(phase_vectorsize);
        numbersCount = 0;
        file.seekg(0);
        if (file.fail()) {
            file.clear();
        }
        int64_t lowest_number, highest_number;
        lowest_number = get_lowest_possible_number(index);
        double loww = *(int64_t*)&lowest_number;
        highest_number = get_highest_possible_number(index);
        double high = *(int64_t*)&highest_number;
        for (int i = 0; i < iterations; i++) {
            uint64_t to_read = ((i + 1 * max_size) > filesize) ? (filesize - (i * max_size)) : max_size;
            file.read((char*)&fileData[0], to_read);

            for (int i = 0; i < fileData.size(); i++) {
                double_t var = fileData[i];
                int64_t number = *(int64_t*)&var;
                auto iindex = return_index(fileData[i]);
                if ((std::fpclassify(fileData[i]) == FP_NORMAL || std::fpclassify(fileData[i]) == FP_ZERO) && (number >= lowest_number && number < highest_number)) {
                    double_t doublicek = fileData[i];
                    int64_t number = *(int64_t*)&doublicek;
                    uint32_t index = return_index(number);
                    numbers.push_back(number);
                    uint64_t position = file.tellg();
                    if (first_encounter[index] == 0) {
                        first_encounter[index] = position;
                    }
                    last_encounter[index] = position;
                    numbersCount++;
                }
            }
        }
        std::sort(numbers.begin(), numbers.end());
        counter = numbers_before;
        auto numbereara = frequencies[index];
        auto aaaaa = frequencies[index - 1];
        for (int i = 0; i < numbers.size(); i++) {
            counter++;
            double percentile = ((double)(counter) / total_of_numbers) * 100;
            if (percentile >= looked_up_percentile && last_percentile < looked_up_percentile) {
                index = i;
                break;
            }
            last_percentile = percentile;
        }
        int64_t number_finder = numbers[index];
        auto encounter_index = return_index(index);
        uint64_t first_enc = first_encounter[encounter_index];
        uint64_t last_enc = last_encounter[encounter_index];
        double aaaaaaaa = *(double*)&number_finder;
        printf("Hledaný percentil:%f \n", aaaaaaaa);
        std::vector<double_t> result(0);
        result.push_back(aaaaaaaa);
        return result;
    }
}

void find (std::string file_name, std::double_t looked_up_percentile, uint8_t execution_mode) {
    uint64_t max_size = 104857600;
    // open the file:
    std::ifstream file(file_name, std::ios::binary);
    // get its size:
    file.seekg(0, std::ios::end);
    uint64_t filesize = file.tellg();
    uint32_t iterations = (uint32_t)ceil((double)filesize / max_size);
    std::vector<double_t> frequencies;
    do
    {
        file.seekg(0, std::ios::beg);
        frequencies = get_histogram(max_size, iterations, filesize, file, looked_up_percentile);
    } while (frequencies.size() == 0);
    printf("Hledaný percentil:%f \n", frequencies[0]);
    file.close();
}

int main(int argc, char* argv[])
{
    int64_t a = INT64_MAX-4;
    double aaa = *(double*)&a;
    a = INT64_MIN;
    double bbb = *(double*)&a;
    a = 0;
    bbb = *(double*)&a;
    a = 0x8000000000000000;
    bbb = *(double*)&a;
    std::string file_name;
    std::double_t looked_up_percentile;
    uint8_t execution_mode;

    if (parse_arguments(&file_name, &looked_up_percentile, &execution_mode, argc, argv)) {
        return -1;
    };
    int errno;
    initialize();
    find(file_name, looked_up_percentile, execution_mode);

    // open the file:
    std::ifstream file(file_name, std::ios::binary);
    // get its size:
    uint64_t beginingsize = file.tellg();
    file.seekg(0, std::ios::end);
    uint64_t filesize = file.tellg();
    file.seekg(std::ios::beg);
    std::vector<double> fileData((uint64_t)(ceil(filesize / 8)) + 1);
    file.read((char*)&fileData[0], filesize);

    for (int i = 0; i < fileData.size(); i++)
    {
        if (std::fpclassify(fileData[i]) == FP_NORMAL || std::fpclassify(fileData[i]) == FP_ZERO) {
            break;
        }
        else {
            fileData.erase(fileData.begin() + 1);
        }
    }

    std::sort(std::execution::par_unseq, fileData.begin(), fileData.end());
    uint64_t index = (uint64_t)(looked_up_percentile / 100 * fileData.size()) -1;
    double result = fileData[index];
    printf("Hledaný percentil:%f \n", result);
    do
    {
        std::cout << '\n' << "Press a key to continue...";
    } while (std::cin.get() != '\n');

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
