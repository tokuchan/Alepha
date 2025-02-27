static_assert( __cplusplus > 2020'00 );

#pragma once

#include <Alepha/Alepha.h>

#include <cstdlib>

#include <string>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <typeindex>

namespace Alepha::Hydrogen
{
	namespace detail::exceptions::inline exports
	{
		/*!
		 * @file
		 *
		 * Exception grades -- Exceptions have different grades of severity.
		 *
		 * In Alepha, there are several "grades" of exception types which can be thrown.  The grade of
		 * an exception is "branded" into its name as the last word in camel case.
		 *
		 *  * `Exception`: All exceptions inherit from this interface.  Catching this will
		 *    catch anything which is part of this system.  Normally you just ignore this grade.
		 *    Code that needs to catch *everything* should not catch `Exception`, but `catch( ... )`
		 *    instead.  `Exception` is the mechanism by which various situations are reported.
		 *    For comparison, Java calls this class `Throwable`.
		 *
		 *  * `Condition`: An out-of-band message from functions which notify their callers of 
		 *    new information that affects the ability to fulfill the common case expected
		 *    result.  Note that this kind of `Exception` is not considered a contract `Violation`.
		 *    This exception can be thrown as part of normal program operation -- it is a control
		 *    flow device.
		 *
		 *  * `Notification`: When a thread is interrupted as a notification, all `Exception`s thrown
		 *    to do so are derived from this type.  Alepha threads are setup to catch and discard
		 *    `Exception`s of this grade in the thread start function.  It is legal to catch `Exception`s
		 *    of this type type and silence it; however, `Notification` typically means that the
		 *    target thread needs to change its behaviour.  (Note that `ThreadInterrupt` is entirely
		 *    independent from the Alepha exception hierarchy -- this helps ensure that the thread
		 *    termination mechanism works correctly.)
		 *
		 *  * `Error`: This is the exception grade you would typically want to recover from.
		 *    Catch this grade, in most circumstances.  They should contain sufficient information in their
		 *    data pack to facilitate a proper programmatic recovery.  All exceptions of this grade
		 *    will also have `std::exception` as one of its bases, thus code which is unaware of
		 *    Alepha exceptions, but handles basic standard exceptions cleanly, will work just fine.
		 *    When an `Error` models something that the standard library models, such as
		 *    `std::bad_alloc`, then that `Error` will be eligible for catch by that base as well.
		 *
		 *  * `CriticalError`: This exception grade represents a form of moderately-unrecoverable condition.
		 *    The `CriticalError` grade typically indicates a condition that prevents the current thread or
		 *    program state from being easily transitioned to a recovered state.  However, a very
		 *    large state transform, such as an unwind to the top-of-thread-callstack handler, may
		 *    be able to recover.  For example, no more available operating system file handles.
		 *    Tearing down several client handlers in a multi-client server might alleviate this condition.
		 *
		 *  * `Violation`: This exception grade represents an impossible to recover from condition.
		 *    Think of the handler for `Violation` as-if it were a `[[noreturn]]` function.  Catch
		 *    handlers for this grade should only clean up resources which might cause persistent
		 *    state corruption.  A program probably SHOULD ignore these and allow a core dump to
		 *    be emitted.  An example of this might be: Runtime detection of dereference of a
		 *    raw pointer set to `nullptr`.  This would typically indicate some kind of program
		 *    bug.  `Violation::~Violation` is setup to call `abort()` when called.  In some sense
		 *    this type is what `std::logic_error` is often used to represent.
		 */

		template< typename ... Bases >
		struct bases : virtual public Bases... {};

		template< typename unique, typename GradeType, typename Bases >
		class synthetic_any_tagged_type
		{
		};

		template< typename unique_handle, typename GradeType, typename ... Bases >
		class synthetic_exception
			: virtual public bases< GradeType, Bases... >
		{
			public:
				using grade_type= GradeType;

				class any_tagged_type
					: virtual public bases< synthetic_exception, typename GradeType::any_tagged_type, typename Bases::any_tagged_type... >
				{
					public:
						using grade_type= GradeType;
				};

				template< typename tag >
				class tagged_type
					: virtual public bases< synthetic_exception, any_tagged_type, typename GradeType::template tagged_type< tag >, typename Bases::template tagged_type< tag >... >
				{
					public:
						using grade_type= GradeType;
				};
		};

		template< typename unique_handle, typename GradeType, typename ... Bases >
		using create_exception= synthetic_exception< unique_handle, GradeType, Bases ... >;

		template< typename Exc >
		using AnyTagged= typename Exc::any_tagged_type;

		template< typename Exc, typename tag >
		using Tagged= typename Exc::template tagged_type< tag >;

		class Exception
		{
			public:
				using grade_type= Exception;

				class any_tagged_type;
				template< typename Tag > class tagged_type;

				virtual ~Exception()= default;
				virtual const char *message() const noexcept= 0;

				template< typename Target >
				const Target &
				as() const
				{
					if( not is_a< const Target * >() )
					{
						// TODO: Structured exception recovery here...
					}

					return dynamic_cast< const Target & >( *this );
				}

				template< typename Target >
				bool
				is_a() const noexcept
				{
					return dynamic_cast< const Target * >( this );
				}
		};
		class Exception::any_tagged_type
			: virtual public grade_type
		{
			public:
				virtual std::type_index tag() const noexcept= 0;
		};
		template< typename Tag >
		class Exception::tagged_type
			: virtual public grade_type, virtual public grade_type::any_tagged_type
		{
			public:
				std::type_index
				tag() const noexcept final
				{
					return typeid( std::type_identity< Tag > );
				}
		};
		using AnyTaggedException= AnyTagged< Exception >;
		template< typename tag >
		using TaggedException= Tagged< Exception, tag >;

		// `Condition`s are "events" that indicate a need
		// for special control flow changes.
		// 
		// They must be handled, but they do not represent
		// an error -- just a "time to change your focus".
		class Condition
			: public virtual Exception
		{
			public:
				using grade_type= Condition;

				class any_tagged_type;
				template< typename tag >
				class tagged_type;
		};
		class Condition::any_tagged_type
			: public virtual bases< grade_type, Exception::any_tagged_type > {};
		template< typename tag >
		class Condition::tagged_type
			: public virtual bases< grade_type::any_tagged_type, Exception::tagged_type< tag > > {};
		using AnyTaggedCondition= Condition::any_tagged_type;
		template< typename tag >
		using TaggedCondition= Condition::tagged_type< tag >;


		// `Notification`s are "events" or "interrupts" that
		// if ignored will gracefully terminate the current
		// thread, but not halt the program.
		class Notification
			: public virtual Exception
		{
			public:
				using grade_type= Notification;

				class any_tagged_type;
				template< typename tag >
				class tagged_type;
		};
		class Notification::any_tagged_type
			: public virtual bases< grade_type, Exception::any_tagged_type > {};
		template< typename tag >
		class Notification::tagged_type
			: public virtual bases< grade_type::any_tagged_type, Exception::tagged_type< tag > > {};
		using AnyTaggedNotification= Notification::any_tagged_type;
		template< typename tag >
		using TaggedNotification= Notification::tagged_type< tag >;

		// `Error`s are recoverable at any point.
		class ErrorBridgeInterface
		{
			public:
				virtual const char *what() const noexcept= 0;
		};
		class Error
			: virtual public bases< Exception >, virtual private ErrorBridgeInterface
		{
			public:
				using grade_type= Error;
				using ErrorBridgeInterface::what;

				class any_tagged_type;
				template< typename tag >
				class tagged_type;
		};
		class Error::any_tagged_type : virtual public bases< grade_type, Exception::any_tagged_type > {};
		template< typename tag >
		class Error::tagged_type
			: virtual public bases< grade_type::any_tagged_type, Exception::tagged_type< tag > > {};
		using AnyTaggedError= Error::any_tagged_type;
		template< typename tag >
		using TaggedError= Error::tagged_type< tag >;

		// `CriticalError`s are only really recoverable by terminating
		// the major procedure underway.  Like terminating the
		// entire thread or similar.  Essentially, arbitrarily
		// localized recovery is impossible.
		class CriticalError
			: virtual public bases< Exception >
		{
			public:
				using grade_type= CriticalError;

				class any_tagged_type;
				template< typename tag >
				class tagged_type;
		};
		class CriticalError::any_tagged_type
			: virtual public bases< grade_type, Exception::any_tagged_type > {};
		template< typename tag >
		class CriticalError::tagged_type
			: public virtual bases< grade_type::any_tagged_type, Exception::tagged_type< tag > > {};
		using AnyTaggedCriticalError= CriticalError::any_tagged_type;
		template< typename tag >
		using TaggedCriticalError= CriticalError::tagged_type< tag >;

		// `Violation`s are unrecoverable events which happen to
		// the process.  They are impossible to recover from.
		// Handlers for this should be treated almost like
		// `[[noreturn]]` functions, such as `std::terminate_handler`.
		// Mostly one would catch this class if they intended to
		// perform a bit of local persistent state sanitization
		// which is somewhat different to the normal dtors in scope
		// and then continue the unwind process
		class Violation
			: virtual public bases< Exception >
		{
			private:
				bool active= true;

			public:
				using grade_type= Violation;
				class any_tagged_type;
				template< typename tag >
				class tagged_type;

				~Violation() override { if( not active ) abort(); }

				Violation( const Violation &copy )= delete;
				Violation( Violation &copy ) : active( copy.active ) { copy.active= false; }
		};
		class Violation::any_tagged_type : virtual public bases< grade_type, Exception::any_tagged_type > {};
		template< typename tag >
		class Violation::tagged_type : virtual public bases< grade_type::any_tagged_type, Exception::tagged_type< tag > > {};
		using AnyTaggedViolation= Violation::any_tagged_type;
		template< typename tag >
		using TaggedViolation= Violation::tagged_type< tag >;

		template< typename T >
		concept DerivedFromError= std::is_base_of_v< Error, T >;

		class NamedResourceStorage;
		class NamedResourceInterface
		{
			public:
				using storage_type= NamedResourceStorage;

				virtual ~NamedResourceInterface()= default;
				virtual std::string_view resourceName() const noexcept= 0;
		};
		class NamedResourceStorage
			: virtual public NamedResourceInterface
		{
			private:
				std::string storage;

			public:
				std::string_view resourceName() const noexcept final { return storage; }
		};
		class NamedResourceException : public virtual synthetic_exception< struct named_resource_throwable, Exception >, virtual public NamedResourceInterface {};
		using AnyTaggedNamedResourceException= NamedResourceException::any_tagged_type;
		template< typename tag >
		using TaggedNamedResourceException= NamedResourceException::tagged_type< tag >;

		using NamedResourceNotification= synthetic_exception< struct named_resource_notification, Notification, NamedResourceException >;
		using AnyTaggedNamedResourceNotification= NamedResourceNotification::any_tagged_type;
		template< typename tag >
		using TaggedNamedResourceNotification= NamedResourceNotification::tagged_type< tag >;

		using NamedResourceError= synthetic_exception< struct named_resource_exception, Error, NamedResourceException >;
		using AnyTaggedNamedResourceError= NamedResourceError::any_tagged_type;
		template< typename tag >
		using TaggedNamedResourceError= NamedResourceError::tagged_type< tag >;

		using NamedResourceCriticalError= synthetic_exception< struct named_resource_error, CriticalError, NamedResourceException >;
		using AnyTaggedNamedResourceCriticalError= NamedResourceCriticalError::any_tagged_type;
		template< typename tag >
		using TaggedNamedResourceCriticalError= NamedResourceCriticalError::tagged_type< tag >;

		using NamedResourceViolation= synthetic_exception< struct named_resource_violation, Violation, NamedResourceException >;
		using AnyTaggedNamedResourceViolation= NamedResourceViolation::any_tagged_type;
		template< typename tag >
		using TaggedNamedResourceViolation= NamedResourceViolation::tagged_type< tag >;

		class OutOfRangeException
			: virtual public synthetic_exception< struct out_of_range_throwable, Exception > {};
		using AnyTaggedOutOfRangeException= OutOfRangeException::any_tagged_type;
		template< typename tag >
		using TaggedOutOfRangeException= OutOfRangeException::tagged_type< tag >;

		using OutOfRangeError= synthetic_exception< struct out_of_range_throwable, Error, OutOfRangeException >;
		using AnyTaggedOutOfRangeError= OutOfRangeError::any_tagged_type;
		template< typename tag >
		using TaggedOutOfRangeError= OutOfRangeError::tagged_type< tag >;

		using OutOfRangeCriticalError= synthetic_exception< struct out_of_range_throwable, CriticalError, OutOfRangeException >;
		using AnyTaggedOutOfRangeCriticalError= OutOfRangeCriticalError::any_tagged_type;
		template< typename tag >
		using TaggedOutOfRangeCriticalError= OutOfRangeCriticalError::tagged_type< tag >;

		using OutOfRangeViolation= synthetic_exception< struct out_of_range_throwable, Violation, OutOfRangeException >;
		using AnyTaggedOutOfRangeViolation= OutOfRangeViolation::any_tagged_type;
		template< typename tag >
		using TaggedOutOfRangeViolation= OutOfRangeViolation::tagged_type< tag >;

		class IndexedRangeInformationStorage;
		class IndexedRangeInformationInterface
		{
			public:
				using storage_type= IndexedRangeInformationStorage;
				virtual ~IndexedRangeInformationInterface()= default;
				virtual std::size_t requested() const noexcept= 0;
				virtual std::size_t lowerBound() const noexcept= 0;
				virtual std::size_t upperBound() const noexcept= 0;
		};
		class IndexedRangeInformationStorage
			: virtual public IndexedRangeInformationInterface
		{
			private:
				std::size_t bottom;
				std::size_t top;
				std::size_t request;

			public:
				std::size_t lowerBound() const noexcept override { return bottom; }
				std::size_t upperBound() const noexcept override { return top; }
				std::size_t requested() const noexcept override { return request; }
		};
		class IndexOutOfRangeException
			: virtual public synthetic_exception< struct index_out_of_range_throwable, Exception, OutOfRangeException > {};
		using AnyTaggedIndexOutOfRangeException= IndexOutOfRangeException::any_tagged_type;
		template< typename tag >
		using TaggedIndexOutOfRangeException= IndexOutOfRangeException::tagged_type< tag >;

		class IndexOutOfRangeError
			: virtual public synthetic_exception< struct index_out_of_range_throwable, OutOfRangeError, IndexOutOfRangeException > {};
		using AnyTaggedIndexOutOfRangeException= IndexOutOfRangeException::any_tagged_type;
		template< typename tag >
		using TaggedIndexOutOfRangeException= IndexOutOfRangeException::tagged_type< tag >;

		class IndexOutOfRangeCriticalError
			: virtual public synthetic_exception< struct index_out_of_range_throwable, OutOfRangeCriticalError, IndexOutOfRangeException > {};
		using AnyTaggedIndexOutOfRangeCriticalError= IndexOutOfRangeCriticalError::any_tagged_type;
		template< typename tag >
		using TaggedIndexOutOfRangeCriticalError= IndexOutOfRangeCriticalError::tagged_type< tag >;


		class IndexOutOfRangeViolation
			: virtual public synthetic_exception< struct index_out_of_range_throwable, OutOfRangeViolation, IndexOutOfRangeException > {};
		using AnyTaggedIndexOutOfRangeViolation= IndexOutOfRangeViolation::any_tagged_type;
		template< typename tag >
		using TaggedIndexOutOfRangeViolation= IndexOutOfRangeViolation::tagged_type< tag >;


		class AllocationAmountStorage;
		class AllocationAmountInterface
		{
			public:
				using storage_type= AllocationAmountStorage;
				virtual ~AllocationAmountInterface()= default;
				virtual std::size_t allocationAmount() const noexcept= 0;
		};
		class AllocationAmountStorage
			: virtual public AllocationAmountInterface
		{
			private:
				std::size_t amount;

			public:
				std::size_t allocationAmount() const noexcept final { return amount; }
		};
		class AllocationException
			: virtual public synthetic_exception< struct allocation_throwable, Exception >, virtual public AllocationAmountInterface {};
		using AnyTaggedAllocationException= AllocationException::any_tagged_type;
		template< typename tag >
		using TaggedAllocationException= AllocationException::tagged_type< tag >;

		using AllocationError= synthetic_exception< struct allocation_exception, Error, AllocationException >;
		using AnyTaggedAllocationError= AllocationError::any_tagged_type;
		template< typename tag >
		using TaggedAllocationError= AllocationError::tagged_type< tag >;

		using AllocationCriticalError= synthetic_exception< struct allocation_error, CriticalError, AllocationException >;
		using AnyTaggedAllocationCriticalError= AllocationCriticalError::any_tagged_type;
		template< typename tag >
		using TaggedAllocationCriticalError= AllocationCriticalError::tagged_type< tag >;

		using AllocationViolation= synthetic_exception< struct allocation_violation, Violation, AllocationException >;
		using AnyTaggedAllocationViolation= AllocationViolation::any_tagged_type;
		template< typename tag >
		using TaggedAllocationViolation= AllocationViolation::tagged_type< tag >;

		class MessageStorage
			: virtual public Exception
		{
			protected:
				std::string storage;

				MessageStorage()= default;
				explicit MessageStorage( std::string storage ) : storage( std::move( storage ) ) {}

			public:
				const char *message() const noexcept { return storage.c_str(); }
		};

		template< typename std_exception >
		class GenericExceptionBridge
			: virtual public std_exception, public virtual ErrorBridgeInterface, virtual public Exception
		{
			public:
				const char *what() const noexcept override { return message(); }
		};

		template< typename Kind >
		auto
		build_exception( std::string message )
		{
			if constexpr( false ) {}
			else if constexpr( std::is_base_of_v< AllocationError, Kind > )
			{
				class Undergird
					: virtual public Kind, virtual protected GenericExceptionBridge< std::bad_alloc >,
					virtual protected MessageStorage, virtual protected AllocationAmountStorage,
					virtual public std::bad_alloc
				{};

				class Error
					: virtual private Undergird, virtual public Kind, public virtual std::bad_alloc
				{
					public:
						explicit Error( std::string message ) : MessageStorage( std::move( message ) ) {}
				};

				return Error{ std::move( message ) };
			}
			else if constexpr( std::is_base_of_v< IndexOutOfRangeError, Kind > )
			{
				class Undergird
					: virtual public Kind, virtual protected GenericExceptionBridge< std::out_of_range >,
					virtual protected MessageStorage, virtual protected IndexedRangeInformationStorage,
					virtual public std::out_of_range
				{};

				class Error
					: virtual private Undergird, virtual public Kind, public virtual std::out_of_range
				{
					public:
						explicit Error( std::string message ) : MessageStorage( std::move( message ) ) {}
				};

				return Error{ std::move( message ) };
			}
			else if constexpr( std::is_base_of_v< Error, Kind > )
			{
				class Undergird
					: virtual public Kind, virtual protected GenericExceptionBridge< std::exception >,
					virtual protected MessageStorage, virtual public std::exception
				{};

				class Error
					: virtual private Undergird,
					virtual public Kind,
					virtual public std::exception
				{
					public:
						explicit Error( std::string message ) : MessageStorage( std::move( message ) ) {}
				};

				return Error{ std::move( message ) };
			}
			else if constexpr( true )
			{
				class Thrown
					: virtual public Kind, virtual private MessageStorage
				{
					public:
						explicit Thrown( std::string message ) : MessageStorage( std::move( message ) ) {}
				};

				return Thrown{ std::move( message ) };
			}
		}

		using FinishedException= synthetic_exception< struct finished_exception, Exception >;
		using AnyTaggedFinishedException= AnyTagged< FinishedException >;
		template< typename tag > using TaggedFinishedException= Tagged< FinishedException, tag >;

		using FinishedCondition= synthetic_exception< struct finished_exception, Condition, FinishedException >;
		using AnyTaggedFinishedCondition= AnyTagged< FinishedCondition >;
		template< typename tag > using TaggedFinishedCondition= Tagged< FinishedCondition, tag >;
	}

	inline namespace exports {}
	namespace exports::inline exceptions
	{
		using namespace detail::exceptions::exports;
	}
}
