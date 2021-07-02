static_assert( __cplusplus > 201700, "C++17 Required" );

#pragma once

#include <Alepha/Alepha.h>

#include <tuple>
#include <string>

#include <Alepha/Meta/is_vector.h>
#include <Alepha/Utility/evaluation.h>

namespace Alepha::Hydrogen::Testing
{
	inline namespace exports { inline namespace table_test {} }

	namespace detail::table_test
	{
		inline namespace exports
		{
			template< auto > struct TableTest;
		}

		using namespace Utility::exports::evaluation;

		template< typename RetVal, typename ... Args, RetVal (*function)( Args... ) >
		struct exports::TableTest< function >
		{
			using args_type= std::tuple< Args... >;

			struct Cases
			{
				using TestDescription= std::tuple< std::string, args_type, RetVal >;

				std::vector< TestDescription > tests;

				explicit
				Cases( std::initializer_list< TestDescription > initList )
					: tests( initList ) {}

				int
				operator() () const
				{
					int failureCount= 0;
					for( const auto &[ comment, params, expected ]: tests )
					{
						if( std::apply( function, params ) != expected )
						{
							std::cout << "  FAILURE: " << comment << std::endl;
							++failureCount;
						}
						else std::cout << "  SUCCESS: " << comment << std::endl;
					}

					return failureCount;
				}
			};

			struct VectorCases;
		};

		template< typename RetVal, typename ... Args, RetVal (*function)( Args... ) >
		struct TableTest< function >::VectorCases
		{
			static_assert( sizeof...( Args ) == 1 );
			static_assert( Meta::is_vector_v< RetVal > );
			static_assert( Meta::is_vector_v< std::tuple_element_t< 0, std::tuple< Args... > > > );

			using TestDescription= std::tuple< std::string,
					std::vector< std::pair< typename std::tuple_element_t< 0, std::tuple< Args... > >::value_type, typename RetVal::value_type > > >;

			std::vector< TestDescription > tests;
			
			explicit
			VectorCases( std::initializer_list< TestDescription > initList )
				: tests( initList ) {}

			int
			operator() () const
			{
				int failureCount= 0;
				for( const auto &[ comment, productions ]: tests )
				{
					const auto expected= evaluate <=[&]
					{
						std::vector< RetVal > rv;
						std::transform( begin( productions ), end( productions ), back_inserter( rv ),
							[]( const auto &prod ) { return prod.second; } );
						return rv;
					};

					const auto params= evaluate <=[&]
					{
						std::vector< RetVal > rv;
						std::transform( begin( productions ), end( productions ), back_inserter( rv ),
							[]( const auto &prod ) { return prod.first; } );
						return rv;
					};

					if( std::apply( function, std::tuple{ params } ) != expected )
					{
						std::cout << "  FAILURE: " << comment << std::endl;
						++failureCount;
					}
					else std::cout << "  SUCCESS: " << comment << std::endl;
				}

				return failureCount;
			}
		};
	}

	namespace exports::table_test
	{
		using namespace detail::table_test::exports;
	}
}
