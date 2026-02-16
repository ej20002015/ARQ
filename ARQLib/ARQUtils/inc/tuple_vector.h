#include <vector>
#include <tuple>
#include <concepts>
#include <type_traits>

/**
 * @brief A generic container holding a vector for each type in the parameter pack.
 * @tparam Types The list of types to be managed.
 */
template <typename... Types>
class TupleVector
{
public:
    using Storage = std::tuple<std::vector<Types>...>;

private:
    Storage m_data;

    template <typename T>
    static constexpr bool IsSupported = ( std::same_as<T, Types> || ... );

public:
    constexpr TupleVector() = default;

    /**
     * @brief Clears all vectors in the batch.
     */
    constexpr void clear() noexcept
    {
        std::apply( [] ( auto&... vectors )
        {
            ( vectors.clear(), ... );
        }, m_data );
    }

    /**
     * @brief Checks if all vectors are empty.
     * @return true if ALL vectors in the batch are empty, false otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return std::apply( [] ( const auto&... vectors )
        {
            return ( vectors.empty() && ... );
        }, m_data );
    }

    /**
     * @brief Calculates the total number of elements across all vectors.
     * @return The sum of the sizes of all contained vectors.
     */
    [[nodiscard]] constexpr size_t size() const noexcept
    {
        return std::apply( [] ( const auto&... vectors )
        {
            return ( vectors.size() + ... );
        }, m_data );
    }

    /**
     * @brief Retrieves the mutable vector for the specified type T.
     * @tparam T The entity type to retrieve (must be in Types).
     * @return A reference to std::vector<T>.
     */
    template <typename T>
        requires IsSupported<T>
    [[nodiscard]] constexpr std::vector<T>& get() noexcept
    {
        return std::get<std::vector<T>>( m_data );
    }

    /**
     * @brief Retrieves the read-only vector for the specified type T.
     * @tparam T The entity type to retrieve.
     * @return A const reference to std::vector<T>.
     */
    template <typename T>
        requires IsSupported<T>
    [[nodiscard]] constexpr const std::vector<T>& get() const noexcept
    {
        return std::get<std::vector<T>>( m_data );
    }

    /**
     * @brief Applies a visitor function to every vector.
     * @tparam Self The deduced type of *this* (const or mutable).
     * @tparam Func The type of the visitor function.
     * @param self The instance of the *TupleVector* (passed automatically).
     * @param func A callable that accepts std::vector<T>& (or const version).
     */
    template <typename Self, typename Func>
    constexpr void visitVectors( this Self&& self, Func&& func )
    {
        // std::forward_like preserves the const-ness of 'self' when accessing m_data
        std::apply( [&func] ( auto&&... vectors )
        {
            // Fold expression calls func(vector) for each vector in the tuple
            ( func( std::forward<decltype( vectors )>( vectors ) ), ... );
        }, std::forward<Self>( self ).m_data );
    }
};