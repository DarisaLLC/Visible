#include "permutation_entropy.h"
#include <memory>
#include <vector>

namespace permutation_entropy

{

    long int factorial(int num)
    {
        long int factorial = 1;
        for (long int i=2; i<=num; ++i) {
            factorial = factorial * i;
        }
        return factorial;
    }
    
    long int recursive_permutation_rank(int length, std::vector<int>& vector, std::vector<int>& inverse)
    {
        assert(vector.size() == inverse.size());
        int last_value;
        if (length < 2) {
            return 0;
        }
        last_value = vector[length-1];
        std::swap(vector[length-1], vector[inverse[length-1]]);
        std::swap(inverse[last_value], inverse[length-1]);
        return last_value + length * 
               recursive_permutation_rank(length-1, vector, inverse);
    }
    
    inline long int permutation_rank(std::vector<int>& vector)
    {
        int length = static_cast<int>(vector.size());
        long int permutation_rank;
        std::vector<int> vector_copy(length), inverse(length);

        for (int i = 0; i < length; ++i) {
            vector_copy[i] = vector[i];
            inverse[vector[i]] = i;
        }
        permutation_rank = recursive_permutation_rank(length, vector_copy,
                                                      inverse);
        return permutation_rank;
    }
    
    // evaluates the permutation entropy of a time series
    // with embedding dimension n
    // by allocating the full permutation histogram of 
    // size n!
    double permutation_entropy_array_stats(std::vector<double>& time_series, int n)
    {
        int data_index;
        int data_rank;
        int cmp_data_index;
        int data_rank_offset;
        int insert_flag;
        int histogram_sum;

        long int i;
        long int n_factorial;
        int time_series_len = static_cast<int>(time_series.size());
        
        double permutation_entropy;
        double relative_frequency;

        n_factorial = factorial(n);
        std::vector<int> data_ranks (n);
        std::vector<int> inverse_data_ranks (n);
        std::vector<int> permutations_histogram (n_factorial);

        // find data ranks for the first slice )
        i = 0;
        // each value in data slice gets a rank
        for (data_index=0; data_index<n; data_index++) {
            data_ranks[data_index] = 0;
            for (cmp_data_index=0; cmp_data_index<n; cmp_data_index++) {
                if (cmp_data_index != data_index) {
                    if (time_series[data_index] > time_series[cmp_data_index]) {
                        data_ranks[data_index]++;
                    }
                }
            }
        }
        // calculate the inverse permutation map
        for (data_index=0; data_index<n; data_index++) {
            inverse_data_ranks[data_ranks[data_index]] = data_index;
        }

        // for i > 0 the forward map data_ranks[data_index] won't be used

        // also the first data slice contribures to stats
        permutations_histogram[permutation_rank(inverse_data_ranks)]++;

        // for i > 0 the algorithm recycles old ranks by inserting just
        // one value for successive data slices
        std::vector<int> new_inverse_data_ranks(n);

        for (i=1; i<time_series_len - n + 1; i++) {

            data_rank_offset = 0;
            insert_flag = 1;

            for (data_rank=0; data_rank<n; data_rank++) {
                // expunge the first number of the previous data slice
                if (inverse_data_ranks[data_rank] == 0) {
                    data_rank_offset--;
                    continue;
                }
                // insert the new number in the data_rank list
                if ((time_series[inverse_data_ranks[data_rank]+i-1] > time_series[n+i-1]) && insert_flag) {
                    new_inverse_data_ranks[data_rank + data_rank_offset] = n-1;
                    data_rank_offset++;
                    insert_flag = 0;
                }
                // assign new data_rank with proper offset given by elimination/insertion
                new_inverse_data_ranks[data_rank + data_rank_offset] = inverse_data_ranks[data_rank] - 1;
                // note that new data indices have a +1 offset
            }
            // if last time_series[i+n-1] still not inserted
            if (insert_flag) {
                new_inverse_data_ranks[n-1] = n-1 ;
            }

            // let new data_indexices will be recycled for next i
            for (data_rank=0; data_rank<n; data_rank++) {
                inverse_data_ranks[data_rank] = new_inverse_data_ranks[data_rank];
            }

            permutations_histogram[permutation_rank(inverse_data_ranks)]++;
        }

        // evaluate the permutation entropy
        permutation_entropy = 0.0;

        histogram_sum = 0;
        for (long int k=0; k<n_factorial; k++) {
            histogram_sum += permutations_histogram[k];
        }
        for (long int k=0; k<n_factorial; k++) {
            // NOTE: if relative_frequency == 0  relative_frequency* log(relative_frequency) = 0
            if (permutations_histogram[k] > 0) {
                relative_frequency = (double)permutations_histogram[k] / (double)histogram_sum;
                permutation_entropy -= relative_frequency * log(relative_frequency);
            }
        }
        permutation_entropy = permutation_entropy / log(2.0);

        return permutation_entropy;
    }



    // evaluates the permutation entropy of a time series
    // with embedding dimension n
    // by allocating the full permutation histogram of 
    // size n!
    double permutation_entropy_dictionary_stats(std::vector<double>& time_series,
                                                int n)
    {
        int data_index;
        int data_rank;
        int cmp_data_index;
        int data_rank_offset;
        int insert_flag;
        int histogram_sum;
        int time_series_len = static_cast<int>(time_series.size());
        long int i;

        double permutation_entropy;
        double relative_frequency;

        std::vector<int> data_ranks (n);
        std::vector<int> inverse_data_ranks (n);
        std::vector<int> new_inverse_data_ranks(n);

        std::map<long,int> permutations_histogram;

        // find data ranks for the first slice )
        i = 0;
        // each value in data slice gets a rank
        for (data_index=0; data_index<n; data_index++) {
            data_ranks[data_index] = 0;
            for (cmp_data_index=0; cmp_data_index<n; cmp_data_index++) {
                if (cmp_data_index != data_index) {
                    if (time_series[data_index] > time_series[cmp_data_index]) {
                        data_ranks[data_index]++;
                    }
                }
            }
        }
        // calculate the inverse permutation map
        for (data_index=0; data_index<n; data_index++) {
            inverse_data_ranks[data_ranks[data_index]] = data_index;
        }

        // for i > 0 the forward map data_ranks[data_index] won't be used

        // also the first data slice contribures to stats
        permutations_histogram[permutation_rank(inverse_data_ranks)]++;

        // for i > 0 the algorithm recycles old ranks by inserting just
        // one value for successive data slices

        for (i=1; i<time_series_len - n + 1; i++) {

            data_rank_offset = 0;
            insert_flag = 1;

            for (data_rank=0; data_rank<n; data_rank++) {
                // expunge the first number of the previous data slice
                if (inverse_data_ranks[data_rank] == 0) {
                    data_rank_offset--;
                    continue;
                }
                // insert the new number in the data_rank list
                if ((time_series[inverse_data_ranks[data_rank]+i-1] > time_series[n+i-1]) && insert_flag) {
                    new_inverse_data_ranks[data_rank + data_rank_offset] = n-1;
                    data_rank_offset++;
                    insert_flag = 0;
                }
                // assign new data_rank with proper offset given by elimination/insertion
                new_inverse_data_ranks[data_rank + data_rank_offset] = inverse_data_ranks[data_rank] - 1;
                // note that new data indices have a +1 offset
            }
            // if last time_series[i+n-1] still not inserted
            if (insert_flag) {
                new_inverse_data_ranks[n-1] = n-1 ;
            }

            // let new data_indexices will be recycled for next i
            for (data_rank=0; data_rank<n; data_rank++) {
                inverse_data_ranks[data_rank] = new_inverse_data_ranks[data_rank];
            }

            permutations_histogram[permutation_rank(inverse_data_ranks)]++;
        }

        // evaluate the permutation entropy
        permutation_entropy = 0.0;

        histogram_sum = 0;
        for (const auto &dict_pair : permutations_histogram) {
            histogram_sum += dict_pair.second;
        }
        for (const auto &dict_pair : permutations_histogram) {
            // add only when the relative freqency of a permutation is > 0
            // in order to avoid nan issues
            if (dict_pair.second > 0) {
                relative_frequency = (double)dict_pair.second / (double)histogram_sum;
                permutation_entropy -= relative_frequency * log(relative_frequency);
            }
        }
        permutation_entropy = permutation_entropy / log(2.0);

        return permutation_entropy;
    }

}
