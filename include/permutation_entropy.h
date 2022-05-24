#include <cmath>
#include <algorithm>
#include <map>

namespace permutation_entropy

{
    long int factorial(int num);

    long int recursive_permutation_rank(int length, std::vector<int>& vector, std::vector<int>& inverse);

    inline long int permutation_rank(std::vector<int>& vector);

    double permutation_entropy_array_stats(std::vector<double>&, int);

    double permutation_entropy_dictionary_stats(std::vector<double>&, int);
}
