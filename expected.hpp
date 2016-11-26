/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of azmq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an implementation of Andrei Alexandrescu's Expected<T> type
    from the "Systematic Error Handling in C++" talk given at C++ And
    Beyond 2012
*/
#ifndef EXPECTED_HPP_
#define EXPECTED_HPP_

#include <boost/assert.hpp>
#include <exception>
#include <utility>
#include <typeinfo>
#include <type_traits>
#include <stdexcept>
#include <cassert>

namespace util {
// define AZMQ_LOG_UNCHECKED *BEFORE* including expected.hpp to forward declare the following
// function to be called any time an exception is present and unchecked in an expected<T>
// when it's destructor is called
#ifdef LOG_UNCHECKED
    void log_expected_unchecked(std::exception_ptr err);
#endif

template<class T>
class expected {
	using val_t = std::aligned_union_t<sizeof(std::exception_ptr), T, std::exception_ptr>;
	val_t val_;
    bool valid_;
    mutable bool unchecked_;

    expected() { }

	T const* p_T() const { return reinterpret_cast<T const*>(&val_); }
	T * p_T() { return reinterpret_cast<T*>(&val_); }

	std::exception_ptr const* p_Ex() const { return reinterpret_cast<std::exception_ptr const*>(&val_); }
	std::exception_ptr * p_Ex() { return reinterpret_cast<std::exception_ptr*>(&val_); }

	void reset() {
		if (valid_)
			p_T()->~T();
		else
			p_Ex()->~exception_ptr();
	}

public:

    expected(const T & rhs) : valid_(true), unchecked_(false) { 
		new (&val_) T(rhs);
	}

    expected(T && rhs) : valid_(true), unchecked_(false) { 
		new (&val_) T(std::forward<T>(rhs));
	}
 
	expected(const expected & rhs)
		: valid_(rhs.valid_)
		, unchecked_(rhs.unchecked_) {
        if (rhs.valid_)
            new(&val_) T(*rhs.p_T());
        else
            new(&val_) std::exception_ptr(*rhs.p_Ex());
    }

	expected& operator=(const expected & rhs) {
		reset();
		if (rhs.valid_)
			new(&val_) T(*rhs.p_T());
		else
			new(&val_) std::exception_ptr(*rhs.p_Ex());
		return *this;
	}

    expected(expected && rhs) 
		: valid_(rhs.valid_)
        , unchecked_(rhs.unchecked_) {
            if (rhs.valid_)
                new(&val_) T(std::move(*rhs.p_T()));
            else
                new(&val_) std::exception_ptr(std::move(*rhs.p_Ex()));
            rhs.unchecked_ = false;
    }

	expected& operator=(expected && rhs) {
		reset();
		if (rhs.valid_)
			new(&val_) T(std::move(*rhs.p_T()));
		else
			new(&val_) std::exception_ptr(std::move(*rhs.p_Ex()));
		return *this;
	}

    ~expected() {
#ifdef LOG_UNCHECKED
        if (unchecked_)
            log_expected_unchecked(*p_Ex());
#else
        BOOST_ASSERT_MSG(!unchecked_, "error result not checked");
#endif
		reset();
    }

    void swap(expected& rhs) {
        if (valid_) {
            if (rhs.valid_) {
                std::swap(*p_T(), *rhs.p_T());
            } else {
                auto t = std::move(*rhs.p_Ex());
                new(&rhs.val_) T(std::move(*p_T()));
                new(&val_) std::exception_ptr(t);
                std::swap(valid_, rhs.valid_);
                std::swap(unchecked_, rhs.unchecked_);
            }
        } else {
            if (rhs.valid_) {
                rhs.swap(*this);
            } else {
                std::swap(*p_Ex()(), *rhs.p_Ex());
                std::swap(valid_, rhs.valid_);
                std::swap(unchecked_, rhs.unchecked_);
            }
        }
    }

    template<class E>
    static expected<T> from_exception(const E & exception) {
        if (typeid(exception) != typeid(E))
            throw std::invalid_argument("slicing detected");
        return from_exception(std::make_exception_ptr(exception));
    }

    static expected<T> from_exception(std::exception_ptr p) {
        expected<T> res;
        res.valid_ = false;
        res.unchecked_ = true;
        new(&res.val_) std::exception_ptr(std::move(p));
        return res;
    }

    static expected<T> from_exception() {
        return from_exception(std::current_exception());
    }

    bool valid() const {
        unchecked_ = false;
        return valid_;
    }

    const T & get() const {
        unchecked_ = false;
        if (!valid_) std::rethrow_exception(*p_Ex());
        return *p_T();
    }

    T & get() {
        unchecked_ = false;
        if (!valid_) std::rethrow_exception(*p_Ex());
        return *p_T();
    }

    template<class E>
    bool has_exception() const {
        unchecked_ = false;
        try {
            if (!valid_) std::rethrow_exception(*p_Ex());
        } catch (const E &) {
            return true;
        } catch (...) { }
        return false;
    }
};
} // namespace util
#endif // EXPECTED_HPP_
