#include <metal.hpp>
#include <boost/pfr.hpp>
#include <iostream>
#include <utility>
#include <tuple>
#include <string>

// Just some quick getting started finger exercises for metal/pfr
// Will remove later on

struct pod_type
{
    // plain pod type with lots of padding internally to test reflection on
    int i1;
    double d1;
    char c1;
    size_t sz1;
    float f1;
    short s1;
};


// metal mapping from type to python struct character code
// this is just a partial mapping for now. Will need to be extended and will have to
// handle array types also which will require some sort of transform
template <typename T>
using map_to_struct_code = metal::at_key<
metal::map<
    metal::pair<int, metal::number<'i'>>,
    metal::pair<double, metal::number<'d'>>,
    metal::pair<char, metal::number<'c'>>,
    metal::pair<size_t, metal::number<'Q'>>,
    metal::pair<float, metal::number<'f'>>,
    metal::pair<short, metal::number<'h'>>
>, T>;


template <typename ... Args>
struct extract_metal_list_impl
{
    using type = metal::list<Args...>;
};


template <template <typename ...> class T, typename ... Args>
auto extract_metal_list(T<Args...> prototype)
{
    return extract_metal_list_impl<Args...>();
}


template <typename T>
char extract_value()
{
    return char(T::value);
}


template <typename ...Args>
auto realize_tuple(metal::list<Args...> metal_list)
{
    return std::make_tuple(extract_value<Args>()...);
}


template<typename T, T...>
struct integer_sequence { };


template<std::size_t N, std::size_t... I>
struct gen_indices : gen_indices<(N - 1), (N - 1), I...> { };
template<std::size_t... I>
struct gen_indices<0, I...> : integer_sequence<std::size_t, I...> { };



std::string& to_string_impl(std::string& s, char h)
{
  using std::to_string;
  s += h;
  return s;
}

template<typename... T>
std::string& to_string_impl(std::string& s, char h, T&&... t)
{
  using std::to_string;
  s += h;
  return to_string_impl(s, std::forward<T>(t)...);
}

template<typename... T, std::size_t... I>
std::string to_string(const std::tuple<T...>& tup, integer_sequence<std::size_t, I...>)
{
  std::string result;
  int ctx[] = { (to_string_impl(result, std::get<I>(tup)...), 0), 0 };
  (void)ctx;
  return result;
}

template<typename... T>
std::string to_string(const std::tuple<T...>& tup)
{
  return to_string(tup, gen_indices<sizeof...(T)>{});
}


int main()
{
    auto s1 = boost::pfr::flat_structure_to_tuple(pod_type());
    using m1 = decltype(extract_metal_list(s1))::type;
    using m2 = metal::transform<metal::lambda<map_to_struct_code>, m1>;

    // Upto here, the elements of interest are encoded in the type signature of
    // a metal::list. This step now makes a real tuple out of that signature
    auto result = realize_tuple(m2());

    // Now the last bit is to turn that tuple into a string
    // https://stackoverflow.com/questions/23436406/converting-tuple-to-string
    std::cout << to_string(result) << std::endl;
    return 0;
}

