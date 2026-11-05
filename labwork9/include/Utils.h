#pragma once

#include <memory>

namespace mystd {
    typedef decltype(sizeof(0)) size_t;

    template<typename T> struct remove_reference { using type = T; };
    template<typename T> struct remove_reference<T&> { using type = T; };
    template<typename T> struct remove_reference<T&&> { using type = T; };
    template<typename T> using remove_reference_t = typename remove_reference<T>::type;

    template<typename T>
    constexpr remove_reference_t<T>&& move(T&& t) noexcept {
        return static_cast<remove_reference_t<T>&&>(t);
    }

    template<typename T>
    constexpr T&& forward(remove_reference_t<T>& t) noexcept {
        return static_cast<T&&>(t);
    }
    template<typename T>
    constexpr T&& forward(remove_reference_t<T>&& t) noexcept {
        return static_cast<T&&>(t);
    }

    template<typename T> struct remove_cv { using type = T; };
    template<typename T> struct remove_cv<const T> { using type = T; };
    template<typename T> struct remove_cv<volatile T> { using type = T; };
    template<typename T> struct remove_cv<const volatile T> { using type = T; };

    template<typename T> struct decay_impl {
        using type = typename remove_cv<remove_reference_t<T>>::type;
    };
    template<typename T> struct decay_impl<T[]> { using type = T*; };
    template<typename T, size_t N> struct decay_impl<T[N]> { using type = T*; };
    template<typename R, typename... Args> struct decay_impl<R(Args...)> { using type = R(*)(Args...); };

    template<typename T> struct decay { using type = typename decay_impl<remove_reference_t<T>>::type; };
    template<typename T> using decay_t = typename decay<T>::type;

    struct true_type { static constexpr bool value = true; };
    struct false_type { static constexpr bool value = false; };

    template<typename T, typename U> struct is_same : false_type {};
    template<typename T> struct is_same<T, T> : true_type {};
    template<typename T, typename U> constexpr bool is_same_v = is_same<T, U>::value;

    template<typename T> struct is_void : false_type {};
    template<> struct is_void<void> : true_type {};
    template<typename T> constexpr bool is_void_v = is_void<T>::value;

    template<typename T> struct is_reference : false_type {};
    template<typename T> struct is_reference<T&> : true_type {};
    template<typename T> struct is_reference<T&&> : true_type {};
    template<typename T> constexpr bool is_reference_v = is_reference<T>::value;

    template<typename T> struct is_lvalue_reference : false_type {};
    template<typename T> struct is_lvalue_reference<T&> : true_type {};
    template<typename T> constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

    template<typename T> struct is_pointer_internal : false_type {};
    template<typename T> struct is_pointer_internal<T*> : true_type {};
    template<typename T> struct is_pointer : is_pointer_internal<typename remove_cv<T>::type> {};
    template<typename T> constexpr bool is_pointer_v = is_pointer<T>::value;

    template<size_t... Is> struct index_sequence {};
    template<size_t N, size_t... Is> struct make_index_sequence_impl : make_index_sequence_impl<N-1, N-1, Is...> {};
    template<size_t... Is> struct make_index_sequence_impl<0, Is...> { using type = index_sequence<Is...>; };
    template<size_t N> using make_index_sequence = typename make_index_sequence_impl<N>::type;

    template<size_t Index, typename T>
    struct TupleLeaf {
        T value;
        TupleLeaf() = default;
        template<typename U>
        TupleLeaf(U&& v) : value(mystd::forward<U>(v)) {}
    };

    template<typename Indices, typename... Types>
    struct TupleImpl;

    template<size_t... Indices, typename... Types>
    struct TupleImpl<index_sequence<Indices...>, Types...> : public TupleLeaf<Indices, Types>... {
        TupleImpl() = default;
        template<typename... UTypes>
        TupleImpl(UTypes&&... args) : TupleLeaf<Indices, Types>(mystd::forward<UTypes>(args))... {}
    };

    template<typename... Types>
    struct tuple : public TupleImpl<make_index_sequence<sizeof...(Types)>, Types...> {
        tuple() = default;
        template<typename... UTypes>
        tuple(UTypes&&... args) : TupleImpl<make_index_sequence<sizeof...(Types)>, Types...>(mystd::forward<UTypes>(args)...) {}
    };

    template<size_t I, typename TupleType> struct tuple_element;
    template<size_t I, typename Head, typename... Tail>
    struct tuple_element<I, tuple<Head, Tail...>> : tuple_element<I - 1, tuple<Tail...>> {};
    template<typename Head, typename... Tail>
    struct tuple_element<0, tuple<Head, Tail...>> { using type = Head; };
    template<size_t I, typename TupleType>
    using tuple_element_t = typename tuple_element<I, TupleType>::type;

    template<size_t I, typename... Types>
    tuple_element_t<I, tuple<Types...>>& get(tuple<Types...>& t) {
        return static_cast<TupleLeaf<I, tuple_element_t<I, tuple<Types...>>>&>(t).value;
    }

    template<size_t I, typename... Types>
    const tuple_element_t<I, tuple<Types...>>& get(const tuple<Types...>& t) {
        return static_cast<const TupleLeaf<I, tuple_element_t<I, tuple<Types...>>>&>(t).value;
    }

    template<size_t I, typename... Types>
    tuple_element_t<I, tuple<Types...>>&& get(tuple<Types...>&& t) {
        return mystd::move(static_cast<TupleLeaf<I, tuple_element_t<I, tuple<Types...>>>&>(t).value);
    }

    template<typename... Args>
    tuple<decay_t<Args>...> make_tuple(Args&&... args) {
        return tuple<decay_t<Args>...>(mystd::forward<Args>(args)...);
    }

    class runtime_error {
        const char* msg_;
    public:
        explicit runtime_error(const char* msg) : msg_(msg) {}
        const char* what() const { return msg_; }
    };
}

namespace detail {
    template <typename T> struct function_traits;

    template <typename R, typename... Args>
    struct function_traits<R(Args...)> {
        using arg_type = mystd::tuple_element_t<0, mystd::tuple<Args...>>;
    };

    template <typename R, typename... Args>
    struct function_traits<R(*)(Args...)> {
        using arg_type = mystd::tuple_element_t<0, mystd::tuple<Args...>>;
    };

    template <typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...)> {
        using arg_type = mystd::tuple_element_t<0, mystd::tuple<Args...>>;
    };

    template <typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...) const> {
        using arg_type = mystd::tuple_element_t<0, mystd::tuple<Args...>>;
    };

    template <typename T>
    struct function_traits {
        using arg_type = typename function_traits<decltype(&mystd::decay_t<T>::operator())>::arg_type;
    };

    template <typename T>
    struct function_traits<T&> : function_traits<mystd::decay_t<T>> {};

    template <typename T>
    struct function_traits<T&&> : function_traits<mystd::decay_t<T>> {};

    template <typename C, typename Ret, typename Obj, typename... Args>
    decltype(auto) my_invoke(Ret (C::*f)(Args...), Obj&& obj, Args&&... args) {
        if constexpr (mystd::is_pointer_v<mystd::decay_t<Obj>>) {
            return (mystd::forward<Obj>(obj)->*f)(mystd::forward<Args>(args)...);
        } else {
            return (mystd::forward<Obj>(obj).*f)(mystd::forward<Args>(args)...);
        }
    }

    template <typename C, typename Ret, typename Obj, typename... Args>
    decltype(auto) my_invoke(Ret (C::*f)(Args...) const, Obj&& obj, Args&&... args) {
        if constexpr (mystd::is_pointer_v<mystd::decay_t<Obj>>) {
            return (mystd::forward<Obj>(obj)->*f)(mystd::forward<Args>(args)...);
        } else {
            return (mystd::forward<Obj>(obj).*f)(mystd::forward<Args>(args)...);
        }
    }

    template <typename F, typename... Args>
    decltype(auto) my_invoke(F&& f, Args&&... args) {
        return mystd::forward<F>(f)(mystd::forward<Args>(args)...);
    }
}
