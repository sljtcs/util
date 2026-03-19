#pragma once
#include <new>

template<typename T>
concept ValidCacheLine = (sizeof(T) <= std::hardware_destructive_interference_size);
template<typename T>
requires ValidCacheLine<T>
struct alignas(std::hardware_destructive_interference_size) CacheLineVar
{
    T value;
    char pad[std::hardware_destructive_interference_size-sizeof(T)];
};