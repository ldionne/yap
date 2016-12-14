#define BOOST_YAP_CONVERSION_OPERATOR_TEMPLATE 1
#include <boost/yap/expression.hpp>

#include <gtest/gtest.h>

#include <sstream>


template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

namespace yap = boost::yap;
namespace bh = boost::hana;


namespace user {

    struct number
    {
        double value;

        friend number operator+ (number lhs, number rhs)
        { return number{lhs.value + rhs.value}; }

        friend number operator* (number lhs, number rhs)
        { return number{lhs.value * rhs.value}; }
    };

    number naxpy (number a, number x, number y)
    { return number{a.value * x.value + y.value + 10.0}; }

    decltype(auto) eval_expression_as (
        decltype(term<number>{{0.0}} * number{} + number{}) const & expr,
        boost::hana::basic_type<number>)
    {
        return naxpy(
            expr.left().left().value(),
            expr.left().right().value(),
            expr.right().value()
        );
    }

    decltype(auto) transform_expression (
        decltype(term<number>{{0.0}} * number{} + number{}) const & expr
    ) {
        return naxpy(
            evaluate(expr.left().left()),
            evaluate(expr.left().right()),
            evaluate(expr.right())
        );
    }

}

TEST(user_expression_transform, test_user_expression_transform)
{
    term<user::number> k{{2.0}};

    term<user::number> a{{1.0}};
    term<user::number> x{{42.0}};
    term<user::number> y{{3.0}};

    {
        yap::expression<
            yap::expr_kind::plus,
            bh::tuple<
                yap::expression<
                    yap::expr_kind::multiplies,
                    bh::tuple<
                        yap::expression<
                            yap::expr_kind::multiplies,
                            bh::tuple<
                                term<user::number>,
                                term<user::number>
                            >
                        >,
                        term<user::number>
                    >
                >,
                term<user::number>
            >
        > expr = term<user::number>{{2.0}} * user::number{1.0} * user::number{42.0} + user::number{3.0};

        user::number result = expr;
        EXPECT_EQ(result.value, 87);
    }

    {
        yap::expression<
            yap::expr_kind::plus,
            bh::tuple<
                yap::expression<
                    yap::expr_kind::multiplies,
                    bh::tuple<
                        term<user::number>,
                        term<user::number>
                    >
                >,
                term<user::number>
            >
        > expr = term<user::number>{{1.0}} * user::number{42.0} + user::number{3.0};

        user::number result = expr;
        EXPECT_EQ(result.value, 55);
    }

    {
        yap::expression<
            yap::expr_kind::multiplies,
            bh::tuple<
                term<user::number>,
                yap::expression<
                    yap::expr_kind::plus,
                    bh::tuple<
                        yap::expression<
                            yap::expr_kind::multiplies,
                            bh::tuple<
                                term<user::number>,
                                term<user::number>
                            >
                        >,
                        term<user::number>
                    >
                >
            >
        > expr = term<user::number>{{2.0}} * (term<user::number>{{1.0}} * user::number{42.0} + user::number{3.0});

        user::number result = expr;
        EXPECT_EQ(result.value, 110);
    }

    {
        auto expr =
            (term<user::number>{{1.0}} * user::number{42.0} + user::number{3.0}) *
            (term<user::number>{{1.0}} * user::number{42.0} + user::number{3.0}) +
            (term<user::number>{{1.0}} * user::number{42.0} + user::number{3.0});

        user::number result = expr;

        // Note: +10 not done at the top level naxpy opportunity.
        EXPECT_EQ(result.value, 55 * 55 + 55);
    }
}
