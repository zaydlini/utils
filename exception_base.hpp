#pragma once

#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/system/error_code.hpp>

#include <exception>
#include <ostream>

namespace util {
    struct exception_base
        : virtual std::exception
        , virtual boost::exception
    { };

	namespace error {
		struct info_holder {
			boost::system::error_code _ec;
			info_holder(boost::system::error_code ec) : _ec(std::move(ec)) {}
			
			friend
			std::ostream& operator<<(std::ostream& stm, info_holder const& that) {
				return stm << "error_code(" << that._ec.message()
					<< ", ec=" << that._ec.value()
					<< ", ecat=" << that._ec.category().name()
					<< ")";
			}
		};

		using error_code_info = boost::error_info<struct tag_error_code_info, info_holder>;
	} // namespace error
} // namespace util
