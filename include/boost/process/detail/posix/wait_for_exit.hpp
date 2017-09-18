// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_POSIX_WAIT_FOR_EXIT_HPP
#define BOOST_PROCESS_DETAIL_POSIX_WAIT_FOR_EXIT_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/posix/child_handle.hpp>
#include <system_error>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace boost { namespace process { namespace detail { namespace posix {

inline bool no_error_and_no_process_has_changed_state(
        int wait_result) noexcept
{
    if ( -1 == wait_result )
        return (EINTR == errno);
    if (0 == wait_result)
        return true;
    return false;
}

inline void wait(
        const child_handle &p,
        int & exit_code,
        std::error_code &ec) noexcept
{
    pid_t ret;
    int status;

    do
    {
        ret = ::waitpid(p.pid, &status, 0);
    }
    while (no_error_and_no_process_has_changed_state(ret));

    if (ret == -1)
    {
        ec = boost::process::detail::get_last_error();
        return;
    }

    ec.clear();
    exit_code = status;
}

inline void wait(const child_handle &p, int & exit_code)
{
    std::error_code ec;
    wait(p, exit_code, ec);
    if (ec)
        boost::process::detail::throw_last_error("waitpid(2) failed");
}

template< class Clock, class Duration >
inline bool wait_until(
        const child_handle &p,
        int & exit_code,
        const std::chrono::time_point<Clock, Duration>& time_out,
        std::error_code & ec) noexcept
{

    pid_t ret;
    int status;

    bool timed_out;

    do
    {
        ret = ::waitpid(p.pid, &status, WNOHANG);
        timed_out = std::chrono::system_clock::now() >= time_out;
    }
    while (no_error_and_no_process_has_changed_state(ret) && !timed_out);

    if (-1 == ret)
    {
        ec = boost::process::detail::get_last_error();
        return !timed_out;
    }

    ec.clear();
    if (!timed_out)
        exit_code = status;
    return true;
}

template< class Clock, class Duration >
inline bool wait_until(
        const child_handle &p,
        int & exit_code,
        const std::chrono::time_point<Clock, Duration>& time_out)
{
    std::error_code ec;
    bool rvalue = wait_until(p, exit_code, time_out, ec);
    if (ec)
        boost::process::detail::throw_last_error("waitpid(2) failed");
    return rvalue;
}

template< class Rep, class Period >
inline bool wait_for(
        const child_handle &p,
        int & exit_code,
        const std::chrono::duration<Rep, Period>& rel_time,
        std::error_code & ec) noexcept
{
    pid_t ret;
    int status;

    auto start = std::chrono::system_clock::now();
    auto time_out = start + rel_time;

    return wait_until(p, exit_code, time_out, ec);
}

template< class Rep, class Period >
inline bool wait_for(
        const child_handle &p,
        int & exit_code,
        const std::chrono::duration<Rep, Period>& rel_time)
{
    std::error_code ec;
    bool rvalue = wait_for(p, exit_code, rel_time, ec);
    if ( ec )
        boost::process::detail::throw_last_error("waitpid(2) failed");
    return rvalue;
}

}}}}

#endif
