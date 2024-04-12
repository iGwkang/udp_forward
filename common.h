#pragma once

#if _WIN32
#include <cstdlib>

# define BYTE_SWAP_2(x) _byteswap_ushort(x)
# define BYTE_SWAP_4(x) _byteswap_ulong(x)
# define BYTE_SWAP_8(x) _byteswap_uint64(x)

#elif __linux__
#include <endian.h>
#include <byteswap.h>
#include <sys/time.h>

#if !defined(BYTE_ORDER) && !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
#define BYTE_ORDER		__BYTE_ORDER__
#define LITTLE_ENDIAN	__ORDER_LITTLE_ENDIAN__
#define BIG_ENDIAN		__ORDER_BIG_ENDIAN__
#endif

# define BYTE_SWAP_2(x) bswap_16(x)
# define BYTE_SWAP_4(x) bswap_32(x)
# define BYTE_SWAP_8(x) bswap_64(x)

#elif __APPLE__
#include <machine/endian.h>
#include <sys/time.h>

# define BYTE_SWAP_2(x) __builtin_bswap16(x)
# define BYTE_SWAP_4(x) __builtin_bswap32(x)
# define BYTE_SWAP_8(x) __builtin_bswap64(x)

#endif

#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <atomic>
#include <memory>
#include <functional>
#include <array>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <vector>
#include <deque>
#include <random>
#include <algorithm>

#if __cplusplus >= 201703L
#include <optional>
#include <filesystem>
#endif

#include "log.h"

#define UNCOPY(ClassName) \
	ClassName(const ClassName &) = delete; \
	ClassName &operator=(const ClassName &) = delete

#define SAFE_THREAD_JOIN(thrd) if (thrd.joinable()) thrd.join()


#define STRINGCAT_HELPER(x, y)  x ## y
#define STRINGCAT(x, y)  STRINGCAT_HELPER(x, y)

#define LAMDBA_REF(...) ((void)((void) __VA_ARGS__))

// 不使用，稻草人标识符
enum { __defer_stack__ };

class DeferStack
{
	std::vector<std::function<void()>> defer_func_stack;
public:
	~DeferStack()
	{
		// while (!defer_func_stack.empty())
		// {
		// 	defer_func_stack.front()();
		// 	defer_func_stack.pop_front();
		// }
		for (auto & f : defer_func_stack)
			f();
	}

	template<typename Function, typename ... Args>
	inline void add_defer(Function && f, Args && ... args)
	{
		defer_func_stack.emplace_back(std::bind(f, std::forward<Args>(args)...));
	}

	template<typename Function>
	inline void operator+(Function && f)
	{
		defer_func_stack.emplace_back(f);
	}

	static constexpr DeferStack &defer_helper(DeferStack &, DeferStack &local_defer)
	{
		return local_defer;
	}

	static constexpr DeferStack &defer_helper(DeferStack &new_defer, const decltype(::__defer_stack__) &)
	{
		return new_defer;
	}
};

// 定义局部 DeferStack 对象
#define init_defer_func_stack() DeferStack __defer_stack__; 

#define defer_var_name STRINGCAT(__defer__, __LINE__)
#define defer_helper_name STRINGCAT(__helper__, __LINE__)

#define  defer_define_helper(var_name) \
    DeferStack var_name; \
    DeferStack::defer_helper(var_name, __defer_stack__)

// 表达式（如果没使用init_defer_func_stack，则引用捕获，若使用了init_defer_func_stack，则值捕获）
#define defer_expr(x) \
    DeferStack defer_var_name; \
    DeferStack &defer_helper_name = DeferStack::defer_helper(defer_var_name, __defer_stack__); \
    if (&defer_helper_name == &defer_var_name) defer_helper_name+[&]{x;}; \
    else defer_helper_name+[=]()mutable{x;};

// 表达式（引用捕获）
#define defer_expr_ref(x) defer_define_helper(defer_var_name)+[&]{x;};

// 表达式（值捕获）
#define defer_expr_val(x) defer_define_helper(defer_var_name)+[=]()mutable{x;};

// 函数
#define defer_func(func, ...) defer_define_helper(defer_var_name).add_defer(func, ##__VA_ARGS__);

// lambda
#define defer_lambda defer_define_helper(defer_var_name)+


inline int64_t get_system_ns()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

// #if _WIN32
//     static bool s_got_clock_frequency = false;
//     static LARGE_INTEGER s_clock_frequency;

//     if (!s_got_clock_frequency)
//     {
//         QueryPerformanceFrequency(&s_clock_frequency);
//         s_got_clock_frequency = true;
//     }

//     LARGE_INTEGER clock_count;
//     QueryPerformanceCounter(&clock_count);

//     double current_time = static_cast<double>(clock_count.QuadPart);
//     current_time *= 1000000000.0;
//     current_time /= static_cast<double>(s_clock_frequency.QuadPart);

//     return (static_cast<int64_t>(current_time));
// #else
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     return tv.tv_sec * 1000000000ll + tv.tv_usec * 1000ll;

// #endif
}

inline int64_t get_steady_clock_ms()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

template<typename T, std::size_t size> struct type_size { typedef T type; };
template<typename T> struct type_size<T, sizeof(uint16_t)> { typedef uint16_t type; };
template<typename T> struct type_size<T, sizeof(uint32_t)> { typedef uint32_t type; };
template<typename T> struct type_size<T, sizeof(uint64_t)> { typedef uint64_t type; };

template<typename T> inline void hton_impl(T &v) { }

inline void hton_impl(uint16_t &v) { v = BYTE_SWAP_2(v); }

inline void hton_impl(uint32_t &v) { v = BYTE_SWAP_4(v); }

inline void hton_impl(uint64_t &v) { v = BYTE_SWAP_8(v); }

#if __cplusplus >= 201703L

template<class T, std::size_t... N>
constexpr T bswap_impl(T i, std::index_sequence<N...>) {
	return (((i >> N * CHAR_BIT & std::uint8_t(-1)) << (sizeof(T) - 1 - N)*CHAR_BIT) | ...);
}

template<class T, std::enable_if_t<(sizeof(T)>1), int> = 0>
constexpr T bswap(T i) {
	return (T)bswap_impl<std::make_unsigned_t<T>>(i, std::make_index_sequence<sizeof(T)>{});
}

#else

template<class T>
constexpr typename std::enable_if<std::is_pod<T>::value, T>::type
bswap(T i, T j = 0u, std::size_t n = 0u) {
    return n == sizeof(T) ? j :
        bswap<T>(i >> CHAR_BIT, (j << CHAR_BIT) | (i & (T)(unsigned char)(-1)), n + 1);
}

#endif

template<typename T, typename std::enable_if<std::is_standard_layout<T>::value, int>::type = 0>
constexpr void hton(T &obj)
{
#if _WIN32 || (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN))
	typedef typename type_size<T, sizeof(T)>::type i_type;
	hton_impl(reinterpret_cast<i_type &>(obj));
	//obj = bswap(obj);
#endif
}

template<typename T>
constexpr void ntoh(T &obj)
{
	hton(obj);
}

// C++14
template <typename T, int N>
constexpr int sizeof_array(T(&)[N])
{
	return N;
}

// C++14
template<typename Func, typename Tuple, std::size_t ... I, typename ...Args, std::enable_if_t<!std::is_member_function_pointer<Func>::value, int> = 0>
constexpr
decltype(auto) bind_front_impl(Func&& f, Tuple&& fargs, std::index_sequence<I...>, Args && ... args)
{
	return (std::forward<Func>(f))(std::get<I>(fargs)..., std::forward<decltype(args)>(args)...);
}

template<typename Func, typename Tuple, std::size_t ... I, typename ...Args, std::enable_if_t<std::is_member_function_pointer<Func>::value, int> = 0>
constexpr
decltype(auto) bind_front_impl(Func&& f, Tuple&& fargs, std::index_sequence<I...>, Args && ... args)
{
	return std::mem_fn(std::forward<Func>(f))(std::get<I>(fargs)..., std::forward<decltype(args)>(args)...);
}

template <typename Func, typename ...Args>
constexpr
decltype(auto) bind_front(Func&& f, Args && ... args)
{
	// std::forward_as_tuple 会将 args 的引用保存在 tuple中，不安全，因此用std::make_tuple将参数拷贝一份
	// 如果要保持传入参数的引用，参考 std::bind，使用 std::ref 包装
	// 不加 mutable 会调用 operator() const，加了mutable之后，调用 operator()，但lambda无法赋值给const变量
	return[f, fargs = std::make_tuple(std::forward<Args>(args)...)](auto &&...args) -> decltype(auto)
	{
		// 此时 fargs 相当于闭包类型的成员变量，调用 f 时直接传入，不能转发进f（如果转发进f，则lambda调用一次之后, fargs中参数的生命周期结束）
		// 不用 tuple_cat 将 fargs、args 打包到一起，是因为 fargs 中可能存在不允许copy的对象
		// 如果用tuple_cat打包到一起，意味着 fargs 要 forward 到新的tuple中，那么 fargs 中的参数生命周期结束
		// const_cast<Func&&>(f) 为了正确处理仿函数，与入参类型保持一致
		// 直接写 args... 会导致无法转发不能拷贝的对象，无法正确转发参数
		return bind_front_impl(const_cast<Func&&>(f), const_cast<decltype(fargs)&>(fargs), std::index_sequence_for<Args...>(), std::forward<decltype(args)>(args)...);
	};
}
