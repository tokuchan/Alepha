static_assert( __cplusplus > 2020'00 );

#pragma once

#include <Alepha/Alepha.h>

#include <cassert>

#include <iostream>
#include <functional>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <map>

#include <boost/lexical_cast.hpp>

#include <Alepha/Concepts.h>

namespace Alepha::inline Cavorite  ::detail::  string_algorithms
{
	inline namespace exports {}

	using VarMap= std::map< std::string, std::function< std::string () > >;

	inline namespace exports
	{
		using VariableMap= VarMap;

		/*!
		 * Returns a new string with text-replacement variables expanded.
		 *
		 * @param text The source text to expand variables within.
		 * @param vars A map of variable names to values to expand.
		 * @param sigil A character which encloses the variable name.  (If the character is `'%'` for example,
		 * then `"%variable%"` is a variable name.)
		 */
		std::string expandVariables( const std::string &text, const VarMap &vars, const char sigil );

		struct StartSubstitutions
		{
			const char sigil;
			VarMap substitutions;

			explicit
			StartSubstitutions( const char sigil, VarMap substitutions )
					: sigil( sigil ), substitutions( std::move( substitutions ) )
			{}
		};

		// Note that these function as a stack -- so `EndSubstitutions` will
		// terminate the top of that stack.
		constexpr struct EndSubstitutions_t {} EndSubstitutions;

		inline namespace impl
		{
			std::ostream &operator << ( std::ostream &, StartSubstitutions && );

			std::ostream &operator << ( std::ostream &, EndSubstitutions_t );
		}

		/*!
		 * Returns a vector of strings parsed from a comma separated string.
		 *
		 * @note This function is almost a split function, but it supports explicit
		 * backslashes for various special characters and the `','` character.
		 *
		 * @param text The text to parse.
		 * @return A vector of the substrings found in the string.
		 */
		std::vector< std::string > parseCommas( const std::string &text );

		std::vector< std::string > split( const std::string &s, char token );

		/*!
		 * Parses an integral range description into a vector of values.
		 */
		template< Integral T >
		std::vector< T >
		parseRange( const std::string &s )
		{
			auto tokens= split( s, "-" );
			if( tokens.empty() or tokens.size() > 2 )
			{
				throw std::runtime_error( "Expected an integer or a range." );
			}
			// If there's no range, or we had a negative number, just emit that.
			if( tokens.size() == 1 or tokens.at( 0 ).empty() ) return { boost::lexical_cast< T >( s ) };

			const auto low= boost::lexical_cast< T >( tokens.at( 0 ) );
			const auto high= boost::lexical_cast< T >( tokens.at( 0 ) );

			std::vector< T > rv{ high - low + 1 };
			std::iota( begin( rv ), end( rv ), low );

			return rv;
		}
	}
}

namespace Alepha::Cavorite::inline exports::inline string_algorithms
{
	using namespace detail::string_algorithms::exports;
}
