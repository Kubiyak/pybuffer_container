#include <metal.hpp>
#include <boost/pfr.hpp>
#include <iostream>
#include <utility>
#include <tuple>
#include <string>
#include <type_traits>
#include <cstdint>


struct trivially_copyable
{
    // plain pod type with lots of padding internally to test reflection on
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
struct py_struct_element
{
    using value = struct_code_map<T>;
};


template <typename T>
struct py_struct_element<T*>
{
    using value = metal::number<'P'>;
};


template <typename ...Args>
struct extract_py_struct_element_impl
{
    using type = metal::transform<metal::lambda<py_struct_element>, metal::list<Args...>>;
};


template <template <typename ...> class T, typename ... Args>
auto extract_py_struct_elements(T<Args...> prototype)
{
    return extract_py_struct_element_impl<Args...>();
}


template <typename T>
std::string extract_value()
{
    std::string s;
    s += T::value::value;
    return s;
}


template <typename ...Args>
std::string make_pystruct_code(metal::list<Args...> metal_list)
{
    std::string result;
    return  (result += ... += extract_value<Args>());
}


template <typename StructType>
std::string get_py_struct_code()
{
    static_assert(std::is_trivially_copyable<StructType>::value, "StructType must be trivially copyable");
    auto s1 = boost::pfr::flat_structure_to_tuple(trivially_copyable());
    using m1 = decltype(extract_py_struct_elements(s1))::type;
    return make_pystruct_code(m1());
}


int main()
{
    std::cout << get_py_struct_code<trivially_copyable>() << std::endl;
    return 0;
}