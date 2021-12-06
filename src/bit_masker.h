//
// Created by koup on 27.11.2021.
//

#pragma once
namespace percentile_finder {
/**
* structure for saving border values
* for next iteration of algorithm (finds only low > number < high
*/
struct Border {
    double low;
    double high;
};


/**
* Stage of algorithm
*/
enum class Stage {
    ZERO,
    FIRST,
    SECOND,
    LAST
};

/**
* Class
* Number masker returns uint32_t bucket_index from double value
* indexing depends on stage of algorithm
*/
class NumberMasker {
public:
    /**
    * stage contains iteration of algorithm
    * First iteration
    * Second and last iteration
    * masking bits from the number
    */
    Stage stage = Stage::FIRST;
    /**
    * bucket_index from first phase of algorithm
    */
    uint32_t zero_phase_index = 0;
    /**
    * returns bucket_index from double value by masking it as int by bits
    * @param number double number
    */
    [[nodiscard]] uint32_t return_index_from_double(double number) const;
    /**
    * increment stage of algorithm and sets border values (low, high)
    * @param index - bucket_index from last iteration
    */
    void increment_stage(uint32_t index);
    /**
    * returns size of vector to use when masking numbers
    */
    [[nodiscard]] uint32_t get_masked_vector_size() const;
    /**
    * returns border values for next stage of algorithm
    * @param index
    */
    Border get_border_values(uint32_t index);

    /**
     * Returns bit shift for actual algorithm stage (used specially on OpenCL)
     * @return bit shift
     */
    uint32_t get_bit_shift();
    /**
    * Returns offset for actual algorithm stage (used specially on OpenCL)
    * @return offset
    */
    uint32_t get_offset();
    /**
     * Returns bit mask for actual algorithm stage (used specially on OpenCL)
     * @return bit mask
     */
    uint32_t get_mask();

    /**
     * returns lower limit of number masker
     * @return lower border limit
     */
    double get_min();

    /**
     * returns high limit of number masker
     * @return high border limit
     */
    double get_max();
    /**
    * constructor - sets stage = 0
    */
    NumberMasker();
private:
    /**
     * Return border values for second stage of algorithm
     * @param index bucket_index of first stage of algorithm
     * @return struct Border @see{Border} with border values
     */
    static Border get_border_values_second_stage(uint32_t index);

    /**
     * Return border values for last stage of algorithm
     * @param index bucket_index of second stage of algorithm
     * @return struct Border @see{Border} with border values
     */
    Border get_border_values_last_stage(uint32_t index) const;

    uint32_t bit_shift;
    uint32_t offset;
    uint32_t mask;

    /**
    * lowest value for next iterations
    */
    double low = -std::numeric_limits<double>::max();
    /**
    * highest value for next iterations
    */
    double high = std::numeric_limits<double>::max();

    /**
     * Check if low >= number < high
     * @param number double number
     * @return if the number is between borders
     */
    bool in_span(double number) const;

    [[nodiscard]] bool is_number_valid(double number) const;
};
}