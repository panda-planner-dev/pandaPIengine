#ifndef SEQUENCESETCOMMON_H
#define SEQUENCESETCOMMON_H

#include <cstdint>

inline uint64_t bitmask_ignore_first(uint16_t bits){
	return ~((uint64_t(1) << bits)-1);
}

inline uint64_t bitmask_ignore_last(uint16_t bits){
	if (bits == 0) return ~uint64_t(0);
	return (uint64_t(1) << (uint16_t(64)-bits))-1;
}

inline uint64_t bitmask_bit(int bit){
	return uint64_t(1) << bit;
}

#endif
