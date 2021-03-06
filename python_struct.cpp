#include <metal.hpp> // https://github.com/brunocodutra/metal
#include <boost/pfr.hpp> // https://github.com/apolukhin/magic_get
#include <iostream>
#include <utility>
#include <string>
#include <type_traits>
#include <cstdint>


// In this example, the type signature of a pod struct type in c++ is converted via metal and pfr metaprogramming
// into the corresponding type signature which can be used to process the memory layout of this structure in Python.
// Without metaprogramming, it would be difficult if not impossible to obtain the corresponding struct unpack string
// for use in Python directly.


struct trivially_copyable
{
    // plain pod type to test reflection on
    int i1;
    double d1;
    char c1;
    size_t sz1[64];
    void * p1;
    float f1;
    short s1;
    int * ip2[24];
    unsigned short foo;
};


// https://docs.python.org/3.8/library/struct.html
template <typename T>
using struct_code_map = metal::at_key<
metal::map<
    metal::pair<char, metal::number<'c'>>,
    metal::pair<std::uint8_t, metal::number<'B'>>,
    metal::pair<bool,
                std::conditional< sizeof(bool) == 4, metal::number<'I'>,
                std::conditional< sizeof(bool) == 2, metal::number<'H'>, metal::number<'B'>>::type>::type>,
    metal::pair<short, metal::number<'h'>>,
    metal::pair<std::uint16_t, metal::number<'H'>>,
    metal::pair<int, metal::number<'i'>>,
    metal::pair<unsigned int, metal::number<'I'>>,
    metal::pair<size_t, metal::number<'Q'>>,
    metal::pair<ssize_t, metal::number<'q'>>,
    metal::pair<float, metal::number<'f'>>,
    metal::pair<double, metal::number<'d'>>
>, T>;


template <typename T>
using size_map = metal::at_key<
metal::map<
        metal::pair<metal::number<'c'>, metal::number<1>>,
        metal::pair<metal::number<'B'>, metal::number<1>>,
        metal::pair<metal::number<'h'>, metal::number<2>>,
        metal::pair<metal::number<'H'>, metal::number<2>>,
        metal::pair<metal::number<'i'>, metal::number<4>>,
        metal::pair<metal::number<'I'>, metal::number<4>>,
        metal::pair<metal::number<'q'>, metal::number<8>>,
        metal::pair<metal::number<'Q'>, metal::number<8>>,
        metal::pair<metal::number<'f'>, metal::number<4>>,
        metal::pair<metal::number<'d'>, metal::number<8>>,
        metal::pair<metal::number<'P'>, metal::number<8>>>, T>;


template <typename T, size_t count_in, size_t max_size_in>
struct py_struct_element
{
    using type = T;
    using value = struct_code_map<T>;
    static constexpr size_t count = count_in;
    static constexpr size_t value_size = size_map<value>::value;
    static constexpr size_t max_size = std::max(max_size_in, value_size);
};


template <typename T, size_t count_in, size_t max_size_in>
struct py_struct_element<T*, count_in, max_size_in>
{
    using type = T*;
    using value = metal::number<'P'>;
    static constexpr size_t count = count_in;
    static constexpr size_t max_size = 8;
};


template <typename Args>
struct extract_py_struct_element_impl;


template <typename T, typename N, bool = std::is_same<T, typename N::type>::value>
struct pystruct_combine_type;


template <typename T, typename ...Args>
struct extract_py_struct_element_impl<metal::list<T, Args...>> :public extract_py_struct_element_impl<metal::list<Args...>>
{
    using list_t = typename extract_py_struct_element_impl<metal::list<Args...>>::result;
    using front = metal::front<list_t>;
    using rest = metal::range<list_t, metal::number<1>, metal::size<list_t>>;
    using combined_elem = typename pystruct_combine_type<T, front>::result;
    using result = metal::join<combined_elem, rest>;
};


template <typename T>
struct extract_py_struct_element_impl<metal::list<T>>
{
    using result = metal::list<py_struct_element<T, 1, 0>>;
};


template <>
struct extract_py_struct_element_impl< metal::list<>>
{
    using result = metal::list<>;
};


template <typename T, typename N>
struct pystruct_combine_type<T, N, true>
{
    using result = metal::list<py_struct_element<T, N::count + 1, 0>>;
};


template <typename T, typename N>
struct pystruct_combine_type<T, N, false>
{
    using result = metal::list<py_struct_element<T, 1, N::max_size>, N>;
};


template <template <typename ...> class T, typename ... Args>
auto extract_py_struct_elements(T<Args...> prototype)
{
    return typename extract_py_struct_element_impl<metal::list<Args...>>::result();
}


template <typename T>
std::string extract_value()
{
    size_t n = T::count;
    std::string s;
    if (T::count == 1)
    {
        s += T::value::value;
    }
    else
    {
        s += std::to_string(T::count);
        s += T::value::value;
    }
    return s;
}


template <typename ...Args>
std::string make_pystruct_code(metal::list<Args...> metal_list)
{
    std::string result;
    (result += ... += extract_value<Args>());
    if (result.size())
    {
        size_t alignment = metal::front<metal::list<Args...>>::max_size;
        if (alignment > 1)
        {
            result += (alignment == 2 ? "0h" : (alignment == 4 ? "0i" : "0P"));
        }
    }
    return result;
}


template <typename StructType>
std::string get_py_struct_code()
{
    static_assert(std::is_trivially_copyable<StructType>::value, "StructType must be trivially copyable");

    // Use pfr to obtain a tuple containing the types of all the elements in the pod struct
    auto reflection_type_info = boost::pfr::flat_structure_to_tuple(trivially_copyable());
    using python_type_info = decltype(extract_py_struct_elements(reflection_type_info));
    return make_pystruct_code(python_type_info());
}


int main()
{
    std::cout << get_py_struct_code<trivially_copyable>() << std::endl;
    return 0;
}