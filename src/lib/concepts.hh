#include <type_traits>

namespace spinscale::nwprog::lib::concepts
{

/// Concept satisfied when value of type U and V are comparable.
template <class U, class V = U>
concept equality_comparable = requires(const U& u, const V& v) {
                                u == v;
                                u != v;
                              };

/// Concept satisfied if class U is an instantiation of template class R (i.e; U = R<Ts...> for some variadic pack Ts).
/// Only type parameters are supported (not value).
/// Default case is false.
template <typename U, template <typename...> class R>
struct IsInstantiationOf : std::false_type
{
};

/// SFINAE overload when concept is satisfied.
template <typename... Ts, template <typename...> class C>
struct IsInstantiationOf<C<Ts...>, C> : std::true_type
{
};

/// Finally concept based of the above.
template <typename U, template <typename...> class R>
concept is_instantiation_of = IsInstantiationOf<U, R>::value;

/// Concept satisfied if all of the given set of classes satisfy the trait.
template <template <typename> class Trait, class... Ts>
concept all_of = std::conjunction_v<Trait<Ts>...>;

/// Concept satisfied if any one of the given set of classes satisfy the trait.
template <template <typename> class Trait, class... Ts>
concept any_of = std::disjunction_v<Trait<Ts>...>;
};  // namespace spinscale::nwprog::lib::concepts
