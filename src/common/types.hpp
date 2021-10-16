#ifndef COMMON_TYPES_HPP
#define COMMON_TYPES_HPP
#include <stdint.h>
namespace FSFS {

template <typename T>
uint8_t* cast_to_data(T container) {
    return reinterpret_cast<uint8_t*>(container);
}

}

#endif