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
    * lowest value for next iterations
    */
    double low = -std::numeric_limits<double>::max();
    /**
    * highest value for next iterations
    */
    double high = std::numeric_limits<double>::max();
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
    uint32_t get_masked_vector_size() const;
    /**
    * returns border values for next stage of algorithm
    * @param index
    */
    Border get_border_values(uint32_t index);
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

    /**
     * Returns bucket_index from double from first stage - default is 20 (how many bits is defined in @see{@link default_config.h}
     * @param number double number that will be masked
     * @return uint32_t bucket_index of the number (bit mask)
     */
    [[nodiscard]] static uint32_t return_index_from_double_first_stage(double number) ;
    /**
     * Returns bucket_index from double from second stage - default is 21(how many bits is defined in @see{@link default_config.h}
     * @param number double number that will be masked
     * @return uint32_t bucket_index of the number (bit mask)
     */
    [[nodiscard]] uint32_t return_index_from_double_second_stage(double number) const;
    /**
     * Returns bucket_index from double from last stage - default is 23 (how many bits is defined in @see{@link default_config.h}
     * @param number double number that will be masked
     * @return uint32_t bucket_index of the number (bit mask)
     */
    [[nodiscard]] uint32_t return_index_from_double_last_stage(double number) const;
};
}