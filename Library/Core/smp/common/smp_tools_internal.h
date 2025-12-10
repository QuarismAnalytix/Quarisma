// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SMP_TOOLS_INTERNAL_H
#define SMP_TOOLS_INTERNAL_H

#include <iterator>  // For std::advance

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace conductor
{
namespace detail
{
namespace smp
{

template <typename InputIt, typename OutputIt, typename Functor>
class unary_transform_call
{
protected:
    InputIt  m_in;
    OutputIt m_out;
    Functor& m_transform;

public:
    unary_transform_call(InputIt _in, OutputIt _out, Functor& _transform)
        : m_in(_in), m_out(_out), m_transform(_transform)
    {
    }

    void Execute(size_t begin, size_t end)
    {
        InputIt  it_in{m_in};
        OutputIt it_out{m_out};
        std::advance(it_in, begin);
        std::advance(it_out, begin);
        for (size_t it = begin; it < end; it++)
        {
            *it_out = m_transform(*it_in);
            ++it_in;
            ++it_out;
        }
    }
};

template <typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
class binary_transform_call : public unary_transform_call<InputIt1, OutputIt, Functor>
{
    InputIt2 m_in2;

public:
    binary_transform_call(InputIt1 _in1, InputIt2 _in2, OutputIt _out, Functor& _transform)
        : unary_transform_call<InputIt1, OutputIt, Functor>(_in1, _out, _transform), m_in2(_in2)
    {
    }

    void Execute(size_t begin, size_t end)
    {
        InputIt1 it_in1{this->m_in};
        InputIt2 it_in2{m_in2};
        OutputIt it_out{this->m_out};
        std::advance(it_in1, begin);
        std::advance(it_in2, begin);
        std::advance(it_out, begin);
        for (size_t it = begin; it < end; it++)
        {
            *it_out = this->m_transform(*it_in1, *it_in2);
            ++it_in1;
            ++it_in2;
            ++it_out;
        }
    }
};

template <typename T>
struct fill_functor
{
    const T& m_value;

public:
    fill_functor(const T& _value) : m_value(_value) {}

    T operator()(T) { return m_value; }
};

}  // namespace smp
}  // namespace detail
}  // namespace conductor
#endif  // DOXYGEN_SHOULD_SKIP_THIS

#endif
/* VTK-HeaderTest-Exclude: smp_tools_internal.h */
