#define BOOST_YAP_CONVERSION_OPERATOR_TEMPLATE 1
#include <boost/yap/expression.hpp>

#include <gtest/gtest.h>

#include <sstream>


template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

template <long long I>
using place_term = boost::yap::terminal<boost::yap::expression, boost::yap::placeholder<I>>;

template <typename T>
using ref = boost::yap::expression_ref<boost::yap::expression, T>;

namespace yap = boost::yap;
namespace bh = boost::hana;


namespace user {

    struct number
    {
        explicit operator double () const { return value; }

        double value;
    };

    number naxpy (number a, number x, number y)
    { return number{a.value * x.value + y.value + 10.0}; }

    inline auto max (int a, int b)
    { return a < b ? b : a; };

    inline number tag_function (double a, double b)
    { return number{a + b}; }

    struct tag_type {};

    template <typename ...T>
    inline auto eval_call (tag_type, T && ...t)
    {
        if constexpr (sizeof...(T) == 2u) {
            return tag_function((double)t...);
        } else {
            assert(!"Unhandled case in eval_call()");
            return;
        }
    }

}


TEST(call_expr, test_call_expr)
{
    using namespace boost::yap::literals;

    {
        yap::expression<
            yap::expr_kind::call,
            bh::tuple<place_term<1>, place_term<2>, place_term<3>>
        > expr = 1_p(2_p, 3_p);

        {
            auto min = [] (int a, int b) { return a < b ? a : b; };
            int result = evaluate(expr, min, 3, 7);
            EXPECT_EQ(result, 3);
        }

        {
            int result = evaluate(expr, &user::max, 3, 7);
            EXPECT_EQ(result, 7);
        }

        {
            int result = evaluate(expr, user::max, 3, 7);
            EXPECT_EQ(result, 7);
        }
    }

    {
        auto min_lambda = [] (int a, int b) { return a < b ? a : b; };

        {
            auto min = yap::make_terminal(min_lambda);
            auto expr = min(1_p, 2_p);

            {
                int result = evaluate(expr, 3, 7);
                EXPECT_EQ(result, 3);
            }
        }

        {
            term<decltype(min_lambda)> min = {{min_lambda}};
            auto expr = min(1_p, 2_p);

            {
                int result = evaluate(expr, 3, 7);
                EXPECT_EQ(result, 3);
            }
        }
    }

    {
        struct min_function_object_t
        {
            auto operator() (int a, int b) const { return a < b ? a : b; }
        };

        min_function_object_t min_function_object;

        {
            term<min_function_object_t &> min = yap::make_terminal(min_function_object);
            auto expr = min(1_p, 2_p);

            {
                using namespace boost::hana::literals;
                int result = evaluate(expr, 3, 7);
                EXPECT_EQ(result, 3);
            }
        }

        {
            term<min_function_object_t> min = {{min_function_object}};
            auto expr = min(1_p, 2_p);

            {
                int result = evaluate(expr, 3, 7);
                EXPECT_EQ(result, 3);
            }
        }

        {
            auto min = yap::make_terminal(min_function_object_t{});
            auto expr = min(1_p, 2_p);

            {
                int result = evaluate(expr, 3, 7);
                EXPECT_EQ(result, 3);
            }
        }

        {
            term<min_function_object_t> min = {{min_function_object_t{}}};
            auto expr = min(1_p, 2_p);

            {
                int result = evaluate(expr, 3, 7);
                EXPECT_EQ(result, 3);
            }
        }

    }

    {
        auto min_lambda = [] (int a, int b) { return a < b ? a : b; };

        {
            auto min = yap::make_terminal(min_lambda);
            auto expr = min(0, 1);

            {
                int result = evaluate(expr);
                EXPECT_EQ(result, 0);
            }
        }

        {
            term<decltype(min_lambda)> min = {{min_lambda}};
            yap::expression<
                yap::expr_kind::call,
                bh::tuple<
                    ref<term<decltype(min_lambda)>& >,
                    term<int>,
                    term<int>
                >
            > expr = min(0, 1);

            {
                int result = evaluate(expr);
                EXPECT_EQ(result, 0);
            }
        }
    }

    {
        {
            auto plus = yap::make_terminal(user::tag_type{});
            auto expr = plus(user::number{13}, 1);

            {
                user::number result = expr;
                EXPECT_EQ(result.value, 14);
            }
        }

        {
            term<user::tag_type> plus = {{user::tag_type{}}};
            yap::expression<
                yap::expr_kind::call,
                bh::tuple<
                    ref<term<user::tag_type>& >,
                    term<int>,
                    term<int>
                >
            > expr = plus(0, 1);

            {
                user::number result = expr;
                EXPECT_EQ(result.value, 1);
            }
        }

    }

    {
        term<user::number> a{{1.0}};
        term<user::number> x{{42.0}};
        term<user::number> y{{3.0}};
        auto n = yap::make_terminal(user::naxpy);

        auto expr = n(a, x, y);
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 55);
        }
    }
}
