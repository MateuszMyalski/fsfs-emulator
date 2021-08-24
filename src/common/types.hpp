#ifndef COMMON_TYPES_HPP
#define COMMON_TYPES_HPP
#include <stdint.h>
namespace FSFS {
typedef int32_t address;
typedef int32_t fsize;
typedef uint8_t data;

template <typename T>
data* cast_to_data(T container) {
    return reinterpret_cast<data*>(container);
}

}

#endif