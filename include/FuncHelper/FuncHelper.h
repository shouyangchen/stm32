#ifndef F103C8_FUNCHELPER_H
#define F103C8_FUNCHELPER_H

#include <type_traits>

// --- 内部辅助模板：检查成员函数指针是否返回 void ---
template<typename T>
struct is_VoidMemberFunc { static constexpr bool value = false; };

// 特化：匹配 const 成员函数 (普通 Lambda 默认是 const 的)
template<typename C, typename... Args>
struct is_VoidMemberFunc<void(C::*)(Args...) const> { static constexpr bool value = true; };

// 特化：匹配非 const 成员函数 (mutable Lambda)
template<typename C, typename... Args>
struct is_VoidMemberFunc<void(C::*)(Args...)> { static constexpr bool value = true; };

// --------------------------------------------------

// 主模板：增加第二个模板参数用于 SFINAE，默认 false
template<typename T, typename = void>
struct is_ExecutableFunction {
    static constexpr bool value = false;
};

// 1. 支持普通函数指针：void(*)(Args...)
// 保持原有的支持
template<typename... Args>
struct is_ExecutableFunction<void(*)(Args...), void> {
    static constexpr bool value = true;
};

// 2. 支持 Lambda 表达式和仿函数 (Functor)
// 条件：T 是类类型，且 T::operator() 返回 void
template<typename T>
struct is_ExecutableFunction<T, typename std::enable_if<std::is_class<T>::value>::type> {
    // 获取 &T::operator() 的类型
    // 注意：如果 Lambda 是泛型的 (auto 参数)，这里会报错，因为无法推导具体的 operator()
    using OperatorType = decltype(&T::operator());

    // 委托给辅助模板判断返回值
    static constexpr bool value = is_VoidMemberFunc<OperatorType>::value;
};

// 3. 支持引用类型 (自动去除引用后递归判断)
template<typename T>
struct is_ExecutableFunction<T&, void> : is_ExecutableFunction<T> {};
template<typename T>
struct is_ExecutableFunction<T&&, void> : is_ExecutableFunction<T> {};
template<typename T>
struct is_ExecutableFunction<const T, void> : is_ExecutableFunction<T> {};


// 宏定义保持不变，现在它可以接受 Lambda 了
#define canAsExecutableFunction(func) static_assert(is_ExecutableFunction<decltype(func)>::value, "The function type is not supported as ExecutableFunction!");

#endif //F103C8_FUNCHELPER_H
