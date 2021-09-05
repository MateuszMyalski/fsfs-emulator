#ifndef COMMON_TYPES_HPP
#define COMMON_TYPES_HPP
#include <stdint.h>
namespace FSFS {
using address = int32_t;
using fsize = int32_t;
using data = uint8_t;

template <typename T>
data* cast_to_data(T container) {
    return reinterpret_cast<data*>(container);
}

}

#endif