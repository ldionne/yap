#include <boost/yap/expression.hpp>
#include <boost/yap/expression_free_operators.hpp>
#include <boost/yap/expression_if_else.hpp>

#include <gtest/gtest.h>


template <typename T>
using term = boost::yap::terminal<boost::yap::expression, T>;

template <typename T>
using ref = boost::yap::expression_ref<boost::yap::expression, T>;

namespace yap = boost::yap;
namespace bh = boost::hana;


namespace user {

    struct number
    {
        double value;

        friend number operator+ (number lhs, number rhs)
        { return number{lhs.value + rhs.value}; }

        friend number operator- (number lhs, number rhs)
        { return number{lhs.value - rhs.value}; }

        friend number operator* (number lhs, number rhs)
        { return number{lhs.value * rhs.value}; }

        friend number operator- (number n)
        { return number{-n.value}; }

        friend bool operator< (number lhs, double rhs)
        { return lhs.value < rhs; }

        friend bool operator< (double lhs, number rhs)
        { return lhs < rhs.value; }
    };

    number naxpy (number a, number x, number y)
    { return number{a.value * x.value + y.value + 10.0}; }

    struct empty_xform {};

    struct eval_xform_tag
    {
        decltype(auto) operator() (yap::terminal_tag, user::number const & n)
        { return n; }
    };

    struct eval_xform_expr
    {
        decltype(auto) operator() (term<user::number> const & expr)
        { return ::boost::yap::value(expr); }
    };

    struct eval_xform_both
    {
        decltype(auto) operator() (yap::terminal_tag, user::number const & n)
        { return n; }

        decltype(auto) operator() (term<user::number> const & expr)
        {
            throw std::logic_error("Oops!  Picked the wrong overload!");
            return ::boost::yap::value(expr);
        }
    };

    struct plus_to_minus_xform_tag
    {
        decltype(auto) operator() (yap::plus_tag, user::number const & lhs, user::number const & rhs)
        {
            return yap::make_expression<yap::expr_kind::minus>(
                term<user::number>{lhs},
                term<user::number>{rhs}
            );
        }
    };

    struct plus_to_minus_xform_expr
    {
        template <typename Expr1, typename Expr2>
        decltype(auto) operator() (yap::expression<yap::expr_kind::plus, bh::tuple<Expr1, Expr2>> const & expr)
        {
            return yap::make_expression<yap::expr_kind::minus>(
                ::boost::yap::left(expr),
                ::boost::yap::right(expr)
            );
        }
    };

    struct plus_to_minus_xform_both
    {
        decltype(auto) operator() (yap::plus_tag, user::number const & lhs, user::number const & rhs)
        {
            return yap::make_expression<yap::expr_kind::minus>(
                term<user::number>{lhs},
                term<user::number>{rhs}
            );
        }

        template <typename Expr1, typename Expr2>
        decltype(auto) operator() (yap::expression<yap::expr_kind::plus, bh::tuple<Expr1, Expr2>> const & expr)
        {
            throw std::logic_error("Oops!  Picked the wrong overload!");
            return yap::make_expression<yap::expr_kind::minus>(
                ::boost::yap::left(expr),
                ::boost::yap::right(expr)
            );
        }
    };

    decltype(auto) naxpy_eager_nontemplate_xform (
        yap::expression<
            yap::expr_kind::plus,
            bh::tuple<
                yap::expression<
                    yap::expr_kind::multiplies,
                    bh::tuple<
                        ref<term<user::number> &>,
                        ref<term<user::number> &>
                    >
                >,
                ref<term<user::number> &>
            >
        > const & expr
    ) {
        auto a = evaluate(expr.left().left());
        auto x = evaluate(expr.left().right());
        auto y = evaluate(expr.right());
        return yap::make_terminal(naxpy(a, x, y));
    }

    decltype(auto) naxpy_lazy_nontemplate_xform (
        yap::expression<
            yap::expr_kind::plus,
            bh::tuple<
                yap::expression<
                    yap::expr_kind::multiplies,
                    bh::tuple<
                        ref<term<user::number> &>,
                        ref<term<user::number> &>
                    >
                >,
                ref<term<user::number> &>
            >
        > const & expr
    ) {
        decltype(auto) a = expr.left().left().value();
        decltype(auto) x = expr.left().right().value();
        decltype(auto) y = expr.right().value();
        return yap::make_terminal(naxpy)(a, x, y);
    }

    struct naxpy_xform
    {
        template <typename Expr1, typename Expr2, typename Expr3>
        decltype(auto) operator() (
            yap::expression<
                yap::expr_kind::plus,
                bh::tuple<
                    yap::expression<
                        yap::expr_kind::multiplies,
                        bh::tuple<
                            Expr1,
                            Expr2
                        >
                    >,
                    Expr3
                >
            > const & expr
        ) {
            return yap::make_terminal(naxpy)(
                transform(expr.left().left(), naxpy_xform{}),
                transform(expr.left().right(), naxpy_xform{}),
                transform(expr.right(), naxpy_xform{})
            );
        }
    };


    // unary transforms

    struct disable_negate_xform_tag
    {
        decltype(auto) operator() (yap::negate_tag, user::number const & value)
        { return yap::make_terminal(value); }

        template <typename Expr>
        decltype(auto) operator() (yap::negate_tag, Expr const & expr)
        { return expr; }
    };

    struct disable_negate_xform_expr
    {
        template <typename Expr>
        decltype(auto) operator() (yap::expression<yap::expr_kind::negate, bh::tuple<Expr>> const & expr)
        { return ::boost::yap::value(expr); }
    };

    struct disable_negate_xform_both
    {
        decltype(auto) operator() (yap::negate_tag, user::number const & value)
        { return yap::make_terminal(value); }

        template <typename Expr>
        decltype(auto) operator() (yap::negate_tag, Expr const & expr)
        { return expr; }

        template <typename Expr>
        decltype(auto) operator() (yap::expression<yap::expr_kind::negate, bh::tuple<Expr>> const & expr)
        {
            throw std::logic_error("Oops!  Picked the wrong overload!");
            return ::boost::yap::value(expr);
        }
    };


    // ternary transforms

//[ tag_xform
    struct ternary_to_else_xform_tag
    {
        template <typename Expr>
        decltype(auto) operator() (
            boost::yap::if_else_tag,
            Expr const & cond,
            user::number const & then,
            user::number const & else_
        ) { return boost::yap::make_terminal(else_); }
    };
//]

//[ expr_xform
    struct ternary_to_else_xform_expr
    {
        template <typename Cond, typename Then, typename Else>
        decltype(auto) operator() (
            boost::yap::expression<
                boost::yap::expr_kind::if_else,
                boost::hana::tuple<Cond, Then, Else>
            > const & expr
        ) { return ::boost::yap::else_(expr); }
    };
//]

    struct ternary_to_else_xform_both
    {
        template <typename Expr>
        decltype(auto) operator() (yap::if_else_tag, Expr const & cond, user::number const & then, user::number const & else_)
        { return yap::make_terminal(else_); }

        template <typename Cond, typename Then, typename Else>
        decltype(auto) operator() (yap::expression<yap::expr_kind::if_else, bh::tuple<Cond, Then, Else>> const & expr)
        {
            throw std::logic_error("Oops!  Picked the wrong overload!");
            return ::boost::yap::else_(expr);
        }
    };

}

TEST(user_expression_transform_3, test_user_expression_transform_3)
{
    term<user::number> a{{1.0}};
    term<user::number> x{{42.0}};
    term<user::number> y{{3.0}};

    {
        auto expr = a;
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 1);
        }

        {
            auto transformed_expr = transform(expr, user::empty_xform{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 1);
        }

        {
            auto transformed_expr = transform(expr, user::eval_xform_tag{});
            EXPECT_EQ(transformed_expr.value, 1);
        }

        {
            auto transformed_expr = transform(expr, user::eval_xform_expr{});
            EXPECT_EQ(transformed_expr.value, 1);
        }

        {
            auto transformed_expr = transform(expr, user::eval_xform_both{});
            EXPECT_EQ(transformed_expr.value, 1);
        }
    }

    {
        auto expr = x + y;
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 45);
        }

        {
            auto transformed_expr = transform(expr, user::plus_to_minus_xform_tag{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 39);
        }

        {
            auto transformed_expr = transform(expr, user::plus_to_minus_xform_expr{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 39);
        }

        {
            auto transformed_expr = transform(expr, user::plus_to_minus_xform_both{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 39);
        }
    }

    {
        auto expr = a * x + y;
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 45);
        }

        auto transformed_expr = transform(expr, user::naxpy_eager_nontemplate_xform);
        {
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 55);
        }
    }

    {
        auto expr = a + (a * x + y);
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 46);
        }

        auto transformed_expr = transform(expr, user::naxpy_eager_nontemplate_xform);
        {
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 56);
        }
    }

    {
        auto expr = a * x + y;
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 45);
        }

        auto transformed_expr = transform(expr, user::naxpy_lazy_nontemplate_xform);
        {
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 55);
        }
    }

    {
        auto expr = a + (a * x + y);
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 46);
        }

        auto transformed_expr = transform(expr, user::naxpy_lazy_nontemplate_xform);
        {
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 56);
        }
    }

    {
        auto expr = (a * x + y) * (a * x + y) + (a * x + y);
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 45 * 45 + 45);
        }

        auto transformed_expr = transform(expr, user::naxpy_xform{});
        {
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 55 * 55 + 55 + 10);
        }
    }
}

auto double_to_float (term<double> expr)
{ return term<float>{(float)expr.value()}; }

auto check_unique_ptrs_equal_7 (term<std::unique_ptr<int>> && expr)
{
    using namespace boost::hana::literals;
    EXPECT_EQ(*expr.elements[0_c], 7);
    return std::move(expr);
}

TEST(move_only, test_user_expression_transform_3)
{
    term<double> unity{1.0};
    term<std::unique_ptr<int>> i{new int{7}};
    yap::expression<
        yap::expr_kind::plus,
        bh::tuple<
            ref<term<double> &>,
            term<std::unique_ptr<int>>
        >
    > expr_1 = unity + std::move(i);

    yap::expression<
        yap::expr_kind::plus,
        bh::tuple<
            ref<term<double> &>,
            yap::expression<
                yap::expr_kind::plus,
                bh::tuple<
                    ref<term<double> &>,
                    term<std::unique_ptr<int>>
                >
            >
        >
    > expr_2 = unity + std::move(expr_1);

    auto transformed_expr = transform(std::move(expr_2), double_to_float);

    transform(std::move(transformed_expr), check_unique_ptrs_equal_7);
}

TEST(unary_transforms, test_user_expression_transform_3)
{
    term<user::number> a{{1.0}};
    term<user::number> x{{42.0}};
    term<user::number> y{{3.0}};

    {
        auto expr = -x;
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, -42);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_tag{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 42);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_expr{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 42);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_both{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 42);
        }
    }

    {
        auto expr = a * -x + y;
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, -39);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_tag{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 45);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_expr{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 45);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_both{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 45);
        }
    }

    {
        auto expr = -(x + y);
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, -45);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_tag{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 45);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_expr{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 45);
        }

        {
            auto transformed_expr = transform(expr, user::disable_negate_xform_both{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 45);
        }
    }
}

TEST(ternary_transforms, test_user_expression_transform_3)
{
    term<user::number> a{{1.0}};
    term<user::number> x{{42.0}};
    term<user::number> y{{3.0}};

    {
        auto expr = if_else(0 < a, x, y);
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 42);
        }

        {
            auto transformed_expr = transform(expr, user::ternary_to_else_xform_tag{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 3);
        }

        {
            auto transformed_expr = transform(expr, user::ternary_to_else_xform_expr{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 3);
        }

        {
            auto transformed_expr = transform(expr, user::ternary_to_else_xform_both{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 3);
        }
    }

    {
        auto expr = y * if_else(0 < a, x, y) + user::number{0.0};
        {
            user::number result = evaluate(expr);
            EXPECT_EQ(result.value, 126);
        }

        {
            auto transformed_expr = transform(expr, user::ternary_to_else_xform_tag{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 9);
        }

        {
            auto transformed_expr = transform(expr, user::ternary_to_else_xform_expr{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 9);
        }

        {
            auto transformed_expr = transform(expr, user::ternary_to_else_xform_both{});
            user::number result = evaluate(transformed_expr);
            EXPECT_EQ(result.value, 9);
        }
    }
}
