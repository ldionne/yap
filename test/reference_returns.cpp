#define BOOST_YAP_CONVERSION_OPERATOR_TEMPLATE 1
#include <boost/yap/expression.hpp>

#include <gtest/gtest.h>


template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

namespace yap = boost::yap;

namespace reference_returning {

    struct number
    {
        double value;
    };

    number a_result{3.0};
    number const the_result{13.0};

    template <typename A, typename B>
    inline number const & eval_plus (A a, B b)
    { return the_result; }

    template <typename A, typename B>
    inline number & eval_minus (A a, B b)
    { return a_result; }

}

TEST(reference_returns, test_reference_returns)
{
    term<reference_returning::number> unity = {{1.0}};
    auto plus_expr = unity + reference_returning::number{1.0};

    {
        reference_returning::number const & n = evaluate(plus_expr);
        EXPECT_EQ(&n, &reference_returning::the_result);
    }

    static_assert(
        std::is_same_v<
            decltype(evaluate(plus_expr)),
            reference_returning::number const &
        >
    );

    auto minus_expr = unity - reference_returning::number{1.0};

    {
        reference_returning::number & n = evaluate(minus_expr);
        EXPECT_EQ(&n, &reference_returning::a_result);
    }

    static_assert(
        std::is_same_v<
            decltype(evaluate(minus_expr)),
            reference_returning::number &
        >
    );

    using namespace yap::literals;

    {
        reference_returning::number & n = evaluate(1_p, reference_returning::a_result);
        EXPECT_EQ(&n, &reference_returning::a_result);
    }

    static_assert(
        std::is_same_v<
            decltype(evaluate(1_p, reference_returning::a_result)),
            reference_returning::number &
        >
    );

    {
        reference_returning::number const & n = evaluate(1_p, reference_returning::the_result);
        EXPECT_EQ(&n, &reference_returning::the_result);
    }

    static_assert(
        std::is_same_v<
            decltype(evaluate(1_p, reference_returning::the_result)),
            reference_returning::number const &
        >
    );
}
