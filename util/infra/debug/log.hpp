#pragma once

#ifndef UNABLE_DEBUG_LOG
    #include <sstream>
    #include <iostream>
    #include <chrono>
    #include <iomanip>
#endif

#ifdef UNABLE_DEBUG_LOG
    #define LOG(...)
    #define LOG_ERR(...)
    #define LOG_VAR(...)
    #define LOG_BIN(...)
    #define LOG_ARR(...)
    #define LOG_TIME(...)
#else
    namespace debug_util
    {
        #define LOG(...) log_internal::log_impl(std::cout, __VA_ARGS__)
        #define LOG_ERR(...) log_internal::log_impl(std::cerr, __VA_ARGS__)
        #define LOG_VAR(var) std::cout << #var ": " << var << std::endl;

        namespace log_internal
        {
            template<typename Stream, typename T>
            void log_impl(Stream& os, T&& arg){
                os << arg << '\n';
            }

            template<typename Stream, typename T, typename... Args>
            void log_impl(Stream& os, T&& first, Args&&... rest){
                std::ostringstream oss;
                oss << std::forward<T>(first);
                ((oss << std::forward<Args>(rest)), ...);
                os << oss.str() << '\n';
            }
        }

        template<typename T>
        void LOG_BIN(const T* buffer, std::size_t length, std::size_t bytePerLine = 0)
        {
            static_assert(std::is_integral_v<T>, "LOG_BIN only support integral type");

            for(std::size_t i=0; i<length; ++i)
            {
                std::printf("%02x ", static_cast<uint8_t>(buffer[i]));
                if(bytePerLine && (i + 1) % bytePerLine == 0)
                    std::printf("\n");
            }
            std::printf("\n");
        }
    }

    namespace debug_util
    {
        template <typename T>
        struct is_std_array : std::false_type {};
        template <typename T, size_t N>
        struct is_std_array<std::array<T, N>> : std::true_type {};

        template<typename T>
        std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>
        LOG_ARR(const T& val){ std::cout << val << ' ';}

        template<typename T, size_t N>
        void LOG_ARR(const T (&arr)[N])
        {
            for(const auto& elem : arr)
            {
                LOG_ARR(elem);
                if constexpr (!(std::is_arithmetic_v<T> || std::is_enum_v<T>))
                    std::cout << '\n';
            }
            if(std::is_arithmetic_v<T> || std::is_enum_v<T>)
                std::cout << '\n';
        }

        template<typename T, size_t N>
        void LOG_ARR(const std::array<T, N>& arr)
        {
            for(const auto& elem : arr)
                LOG_ARR(elem);
            if(std::is_arithmetic_v<T> || std::is_enum_v<T>)
                std::cout << '\n';
        }
    }

    namespace debug_util
    {
        inline void LOG_TIME(const std::chrono::system_clock::time_point& tp, int8_t zone = 0)
        {
            std::time_t t = std::chrono::system_clock::to_time_t(tp + std::chrono::hours(zone));
            std::tm tm_buf{};
        #ifdef __linux__
            gmtime_r(&t, &tm_buf);
        #else 
            gmtime_s(&tm_buf, &t);
        #endif
            std::cout << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S UTC") << std::endl;
        }
    }
    using namespace debug_util;
#endif