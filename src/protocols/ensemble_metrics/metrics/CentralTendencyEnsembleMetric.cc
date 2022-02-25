// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (CentralTendencyEnsembleMetric.cc), WHICH WAS MADE
// PUBLICLY AVAILABLE IN THE FOLLOWING GITHUB REPOSITORY PRIOR TO ITS INCLUSION IN
// THE ROSETTA SOFTWARE SUITE: git@github.com:vmullig/ensemble_metrics.git
//
// MIT License
//
// Copyright (c) 2022 Vikram K. Mulligan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/// @file protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetric.cc
/// @brief An ensemble metric that takes a real-valued simple metric, applies it to all poses in an ensemble,
/// and calculates measures of central tendency (mean, median, mode) and other statistics about the distribution
/// (standard deviation, standard error of the mean, min, max, range, etc.).
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

// Unit headers
#include <protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetric.hh>
#include <protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetricCreator.hh>

// Core headers
#include <core/simple_metrics/util.hh>
#include <core/simple_metrics/SimpleMetric.hh>
#include <core/simple_metrics/RealMetric.hh>

// Protocols headers
#include <protocols/ensemble_metrics/util.hh>

// Basic headers
#include <basic/Tracer.hh>
#include <basic/datacache/DataMap.hh>

// Utility headers
#include <utility/tag/Tag.hh>
#include <utility/vector1.hh>
#include <utility/pointer/memory.hh>

// STL headers
#include <numeric>

// XSD Includes
#include <utility/tag/XMLSchemaGeneration.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>
#include <basic/citation_manager/CitationCollection.hh>

#ifdef USEMPI
#include <mpi.h>
#include <type_traits>
#endif

#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>
#include <utility/vector1.srlz.hh>

// Cereal headers
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

static basic::Tracer TR( "protocols.ensemble_metrics.metrics.CentralTendencyEnsembleMetric" );

namespace protocols {
namespace ensemble_metrics {
namespace metrics {

/// @brief The names of all the float-valued metrics that this ensemble metric
/// is capable of returning must go here, initialized in the parentheses.
/// @details Const global data.
static utility::vector1< std::string > const metric_names_for_class{
"mean", "median", "mode",
"stddev", "stderr",
"min", "max", "range"
};

////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION AND DESTRUCTION
////////////////////////////////////////////////////////////////////////////////

/// @brief Default constructor
CentralTendencyEnsembleMetric::CentralTendencyEnsembleMetric() = default;

/// @brief Copy constructor
CentralTendencyEnsembleMetric::CentralTendencyEnsembleMetric( CentralTendencyEnsembleMetric const & ) = default;

/// @brief Destructor (important for properly forward-declaring smart-pointer members)
/// @note On destruction, an ensemble metric that has not yet reported does its final report.  This
/// behaviour must be implemented by derived classes due to order of calls to destructors.
CentralTendencyEnsembleMetric::~CentralTendencyEnsembleMetric() {
	if ( !finalized() && poses_in_ensemble() > 0 ) {
		produce_final_report();
	}
}

protocols::ensemble_metrics::EnsembleMetricOP
CentralTendencyEnsembleMetric::clone() const {
	return utility::pointer::make_shared< CentralTendencyEnsembleMetric >( *this );
}

////////////////////////////////////////////////////////////////////////////////
// Virtual functions overrides of public pure virtual functions from base class.
////////////////////////////////////////////////////////////////////////////////

/// @brief Provide the name of this EnsmebleMetric.
/// @details Must be implemented by derived classes.
std::string
CentralTendencyEnsembleMetric::name() const {
	return name_static();
}

/// @brief Name of the class for creator.
std::string
CentralTendencyEnsembleMetric::name_static() {
	return "CentralTendency";
}

/// @brief Get a list of the names of the real-valued metrics that can be filtered on (e.g. by the EnsembleMetricFilter)
/// or otherwise extracted from this EnsembleMetric.  Must be implemented by derived classes.  (Can be empty list if
/// no real-valued metrics are computed.)
utility::vector1< std::string > const &
CentralTendencyEnsembleMetric::real_valued_metric_names() const {
	return metric_names_for_class;
}

////////////////////////////////////////////////////////////////////////////////
// Virtual functions overrides of private pure virtual functions from base class.
////////////////////////////////////////////////////////////////////////////////

/// @brief Write the final report produced by this metric to a string.
/// @details Must be implemented by derived classes.
/// @note Output should not be terminated in a newline.
std::string
CentralTendencyEnsembleMetric::produce_final_report_string() {
	std::ostringstream ss;
	finalize_values();
	ss << "Computed values for " << simple_metric_->name() << " real-valued simple metric." << std::endl;
	ss << "\tmean:\t" << mean_ << std::endl;
	ss << "\tmedian:\t" << median_ << std::endl;
	ss << "\tmode:\t" << mode_ << std::endl;
	ss << "\tstddev:\t" << stddev_ << std::endl;
	ss << "\tstderr:\t" << stderr_ << std::endl;
	ss << "\tmin:\t" << min_ << std::endl;
	ss << "\tmax:\t" << max_ << std::endl;
	ss << "\trange:\t" << range_;
	return ss.str();
}

/// @brief Add another pose to the ensemble seen so far.  Nonconst to allow data to be
/// accumulated.
/// @details Must be implemented by derived classes.
void
CentralTendencyEnsembleMetric::add_pose_to_ensemble(
	core::pose::Pose const & pose
) {
	runtime_assert_string_msg( simple_metric_ != nullptr, "Error in CentralTendencyEnsembleMetric::add_pose_to_ensemble(): A simple metric must be passed to this ensemble metric before it can be used on a set of poses." );
	values_.push_back( simple_metric_->calculate(pose) );
	TR << simple_metric_->name() << " simple metric reported value " << values_[values_.size()] << " for pose " << poses_in_ensemble() << "." << std::endl;
}

/// @brief Given a metric name, get its value.
/// @details Must be implemented by derived classes.
core::Real
CentralTendencyEnsembleMetric::derived_get_metric_by_name(
	std::string const & metric_name
) const {
	debug_assert( metric_names_for_class.has_value( metric_name ) );

	//Return the appropriate metric here given the name.
	if ( metric_name == "mean" ) {
		return mean_;
	} else if ( metric_name == "median" ) {
		return median_;
	} else if ( metric_name == "mode" ) {
		return mode_;
	} else if ( metric_name == "stddev" ) {
		return stddev_;
	} else if ( metric_name == "stderr" ) {
		return stderr_;
	} else if ( metric_name == "min" ) {
		return min_;
	} else if ( metric_name == "max" ) {
		return max_;
	} else if ( metric_name == "range" ) {
		return range_;
	} else {
		utility_exit_with_message( "Error in CentralTendencyEnsembleMetric::derived_get_metric_by_name(): \"" + metric_name + "\" is not a metric that the " + name() + " ensemble metric returns." );
	}

	return 0.0; //Keep older compilers happy.
}

/// @brief Get the tracer for a derived class.
/// @details Must be implemented for each derived class.
basic::Tracer &
CentralTendencyEnsembleMetric::get_derived_tracer() const {
	return TR;
}

/// @brief Reset the data collected by the derived classes.  Must be
/// implemented by derived classes.
void
CentralTendencyEnsembleMetric::derived_reset() {
	mean_ = median_ = mode_ = 0.0;
	stddev_ = stderr_ = 0.0;
	min_ = max_ = range_ = 0.0;
	values_.clear();
	derived_finalized_ = false;
}

////////////////////////////////////////////////////////////////////////////////
// RosettaScripts functions
////////////////////////////////////////////////////////////////////////////////

/// @brief Parse XML setup.
/// @details Must be implemented for each derived class.
void
CentralTendencyEnsembleMetric::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & data
) {
	std::string const errmsg( "Error in CentralTendencyEnsembleMetric::parse_my_tag(): " );
	parse_common_ensemble_metric_options( tag, data );

	// Add logic here to parse XML.  Each item passed should
	// call a setter so that the object can also be configured
	// from C++ or PyRosetta code.
	if ( tag->hasOption("real_valued_metric") ) {
		core::simple_metrics::SimpleMetricCOP metric( core::simple_metrics::get_metric_from_datamap_and_subtags( tag, data, "real_valued_metric" ) );
		runtime_assert_string_msg( metric != nullptr, errmsg + "No simple metric named \"" + tag->getOption<std::string>("real_valued_metric") + "\" has been defined!" );
		core::simple_metrics::RealMetricCOP realmetric( utility::pointer::dynamic_pointer_cast< core::simple_metrics::RealMetric const >(metric) );
		runtime_assert_string_msg( realmetric != nullptr, errmsg + "The \"" + tag->getOption< std::string >("real_valued_metric") + "\" simple metric is not a real-valued simple metric!" );
		set_real_metric( realmetric );
	}
}

/// @brief Provide a machine-readable description (XSD) of the XML interface
/// for this ensemble metric.
/// @details Must be implemented for each derived class.
void
CentralTendencyEnsembleMetric::provide_xml_schema(
	utility::tag::XMLSchemaDefinition & xsd
) {
	using namespace utility::tag;
	using namespace core::simple_metrics;

	AttributeList attlist;
	attlist + XMLSchemaAttribute::attribute_w_default(
		"real_valued_metric", xs_string,
		"The name of a real-valued simple metric defined previously.  Required input.", ""
	);

	protocols::ensemble_metrics::xsd_ensemble_metric_type_definition_w_attributes(
		xsd, name_static(),
		"An ensemble metric that takes a real-valued simple metric, applies it to all poses in an ensemble, "
		"and calculates measures of central tendency (mean, median, mode) and other statistics about the distribution "
		"(standard deviation, standard error of the mean, min, max, range, etc.).  Values that this ensemble metric returns "
		"are referred to in scripts as: mean, median, mode, stddev, stderr, min, max, and range.",
		attlist
	);
}

////////////////////////////////////////////////////////////////////////////////
// CitationManager functions
////////////////////////////////////////////////////////////////////////////////

/// @brief Provide citations to the passed CitationCollectionList
/// Subclasses should add the info for themselves and any other classes they use.
/// @details The default implementation of this function does nothing.  It ought to be
/// overriden by ensemble metrics so that they can provide citation information or
/// unpublished author information.
void
CentralTendencyEnsembleMetric::provide_citation_info(
	basic::citation_manager::CitationCollectionList & citations
) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"CentralTendencyEnsembleMetric", basic::citation_manager::CitedModuleType::SimpleMetric,
		"Vikram K. Mulligan",
		"Systems Biology group, Center for Computational Biology, Flatiron Institute",
		"vmulligan@flatironinstitute.org",
		"Created the ensemble metric framework and wote the CentralTendency ensemble metric."
		)
	);
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC MPI PARALLEL COMMUNICATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

#ifdef USEMPI

/// @brief Does this EnsembleMetric support MPI-based collection of ensemble properties from an ensemble
/// sampled in a distributed manner?  Overrides base class and returns true.
/// @details To collect results from many MPI processes at the end of a JD2 RosettaScripts run,
/// an EnsembleMetric must implement send_mpi_summary() and recv_mpi_summary().  The MPI JD2 job
/// distributor will ensure that all of the distributed instances of an EnsembleMetric synchronously
/// send their data to the master process EnsembleMetric instance, which receives it.  The base class
/// function exits with an error, so any derived class that fails to override these functions cannot
/// be used for MPI-distributed ensemble analysis.  To allow early catching of issues with EnsembleMetric
/// derived classes that do not support MPI, the base class implements bool supports_mpi() as returning
/// false, and derived classes must override this to return true if the derived class supports MPI.  This
/// function is called by the parse_common_ensemble_metric_options() function if the configuration
/// has been set for MPI-based collection at the end.
bool
CentralTendencyEnsembleMetric::supports_mpi() const {
	return true;
}

/// @brief Send all of the data collected by this EnsembleMetric to another node.  Overrides base class.
/// @details To collect results from many MPI processes at the end of a JD2 RosettaScripts run,
/// an EnsembleMetric must implement send_mpi_summary() and recv_mpi_summary().  The MPI JD2 job
/// distributor will ensure that all of the distributed instances of an EnsembleMetric synchronously
/// send their data to the master process EnsembleMetric instance, which receives it.  The base class
/// function exits with an error, so any derived class that fails to override these functions cannot
/// be used for MPI-distributed ensemble analysis.  To allow early catching of issues with EnsembleMetric
/// derived classes that do not support MPI, the base class implements bool supports_mpi() as returning
/// false, and derived classes must override this to return true if the derived class supports MPI.  This
/// function is called by the parse_common_ensemble_metric_options() function if the configuration
/// has been set for MPI-based collection at the end.
/// @note This will do one or more MPI_Send operations!  It is intended only to be called by callers that can
/// guarantee synchronicity and which can avoid deadlock (e.g. the JD2 MPI job distributor)!
void
CentralTendencyEnsembleMetric::send_mpi_summary(
	core::Size const receiving_node_index
) const {
	static_assert( std::is_same< double, core::Real >::value, "Compile-time error!  MPI communication requires that core::Real is defined as a double-precision float." ); //We're in trouble if someone has redefined Real.
	static_assert( std::is_same< unsigned long int, core::Size >::value, "Compile-time error!  MPI communication requires that core::Size is defined as an unsigned long integer." );

	//Note that we have to use int and double for MPI:
	int const n_poses_seen( static_cast< int >( poses_in_ensemble() ) );
	debug_assert(n_poses_seen >= 0 ); //Must be true.
	runtime_assert( static_cast<core::Size>(n_poses_seen) == values_.size() ); //Should be true.

	//Transmit the number of values:
	MPI_Send( static_cast< const void * >( &n_poses_seen ), 1, MPI_INT, static_cast<int>(receiving_node_index), 0, MPI_COMM_WORLD );
	//Transmit the array of values:
	if( n_poses_seen > 0 ) {
		MPI_Send( static_cast< const void * >( values_.data() ), n_poses_seen, MPI_DOUBLE, static_cast<int>(receiving_node_index), 0, MPI_COMM_WORLD );
	}
}

/// @brief Receive all of the data collected by this EnsembleMetric on another node.  Overrides base class.
/// @details To collect results from many MPI processes at the end of a JD2 RosettaScripts run,
/// an EnsembleMetric must implement send_mpi_summary() and recv_mpi_summary().  The MPI JD2 job
/// distributor will ensure that all of the distributed instances of an EnsembleMetric synchronously
/// send their data to the master process EnsembleMetric instance, which receives it.  The base class
/// function exits with an error, so any derived class that fails to override these functions cannot
/// be used for MPI-distributed ensemble analysis.  To allow early catching of issues with EnsembleMetric
/// derived classes that do not support MPI, the base class implements bool supports_mpi() as returning
/// false, and derived classes must override this to return true if the derived class supports MPI.  This
/// function is called by the parse_common_ensemble_metric_options() function if the configuration
/// has been set for MPI-based collection at the end.
/// @returns Originating process index that generated the data that this process received.
/// @note This will do one or more MPI_Recv operations!  It is intended only to be called by callers that can
/// guarantee synchronicity and which can avoid deadlock (e.g. the JD2 MPI job distributor)!
core::Size
CentralTendencyEnsembleMetric::recv_mpi_summary() {
	static_assert( std::is_same< double, core::Real >::value, "Compile-time error!  MPI communication requires that core::Real is defined as a double-precision float." ); //We're in trouble if someone has redefined Real.
	static_assert( std::is_same< unsigned long int, core::Size >::value, "Compile-time error!  MPI communication requires that core::Real is defined as a double-precision float." );

	//Note that we have to use int and double for MPI:
	int n_additional_poses(-1);

	//Status object:
	MPI_Status mystatus;
	int originating_proc(-1);

	//Receive the number of values:
	MPI_Recv( static_cast< void * >( &n_additional_poses ), 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &mystatus);

	//Check what we've got:
	runtime_assert( n_additional_poses >= 0 );
	originating_proc = mystatus.MPI_SOURCE; //The node that sent the message.
	runtime_assert( originating_proc >= 0 );
	if( n_additional_poses == 0 ) return static_cast< core::Size >( originating_proc );

	//Allocate storage for what we're about to receive.
	core::Size const oldsize( values_.size() );
	values_.resize( oldsize + n_additional_poses );

	//From the same process, receive the list of values.
	MPI_Recv( static_cast< void * >( values_.data() + oldsize ), n_additional_poses, MPI_DOUBLE, originating_proc, 0, MPI_COMM_WORLD, &mystatus);
	runtime_assert( mystatus.MPI_SOURCE == originating_proc ); //Should be true.

	//Update the number of poses we've seen:
	increment_poses_in_ensemble( static_cast< core::Size >( n_additional_poses ) );

	//Return the index of the originating proc:
	return static_cast< core::Size >( originating_proc );
}

#endif //USEMPI

////////////////////////////////////////////////////////////////////////////////
// Private functions for this subclass
////////////////////////////////////////////////////////////////////////////////

/// @brief At the end of accumulation and start of reporting, finalize the values.
void
CentralTendencyEnsembleMetric::finalize_values() {
	if ( derived_finalized_ ) return;
	derived_finalized_ = true;
	debug_assert( poses_in_ensemble() == values_.size() ); // Should be true.
	runtime_assert_string_msg( poses_in_ensemble() > 0, "Error in CentralTendencyEnsembleMetric::finalize_values(): At least one pose must be seen before ensemble properties can be calculated." );

	// Mean:
	mean_ = std::accumulate( values_.begin(), values_.end(), 0.0 ) / static_cast< core::Real >( poses_in_ensemble() );

	// Median, min, max, range:
	utility::vector1< core::Real > values_sorted = values_;
	std::sort( values_sorted.begin(), values_sorted.end() );
	if ( values_sorted.size() % 2 == 0 ) {
		core::Size const pos( values_sorted.size() / 2 );
		median_ = (values_sorted[pos] + values_sorted[pos+1]) / 2.0;
	} else {
		median_ = values_sorted[ values_sorted.size() / 2 + 1 ];
	}
	min_ = values_sorted[1];
	max_ = values_sorted[values_sorted.size()];
	range_ = max_ - min_;

	// Mode, StdDev, StdErr:
	std::map< core::Real, core::Size > counts;
	core::Real accumulator( 0.0 );
	for ( core::Size i(1), imax(values_.size()); i<=imax; ++i ) {
		core::Real const curval( values_[i]);
		accumulator += std::pow( curval - mean_, 2 );
		if ( counts.count( curval ) == 0 ) {
			counts[curval] = 1;
		} else {
			counts[curval] += 1;
		}
	}
	accumulator /= static_cast< core::Real >( values_.size() );
	stddev_ = std::sqrt( accumulator );
	stderr_ = stddev_ / std::sqrt( static_cast< core::Real >( values_.size() ) );
	core::Real accumulator2(0.0);
	core::Size maxsize( 0 );
	core::Size maxsize_counter( 0 );
	for ( std::map< core::Real, core::Size >::const_iterator it( counts.begin() ); it != counts.end(); ++it ) {
		if ( it->second > maxsize ) {
			accumulator2 = it->first;
			maxsize_counter = 1;
			maxsize = it->second;
		} else if ( it->second == maxsize ) {
			accumulator2 += it->first;
			++maxsize_counter;
		}
	}
	mode_ = accumulator2 / static_cast< core::Real >(maxsize_counter);
}

////////////////////////////////////////////////////////////////////////////////
// Public functions for this subclass
////////////////////////////////////////////////////////////////////////////////

/// @brief Set the real-valued metric that this ensemble metric will use.
/// @details Used directly; not cloned.
void
CentralTendencyEnsembleMetric::set_real_metric(
	core::simple_metrics::RealMetricCOP const & metric_in
) {
	simple_metric_ = metric_in;
}

/// @brief The mean.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::mean() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::mean(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return mean_;
}

/// @brief The median.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::median() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::median(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return median_;
}

/// @brief The mode.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::mode() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::mode(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return mode_;
}

/// @brief The standard deviation of the mean.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::stddev() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::stddev(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return stddev_;
}

/// @brief The standard error of the mean.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::stderror() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::stderror(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return stderr_;
}

/// @brief The minimum value.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::min() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::min(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return min_;
}

/// @brief The maximum value.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::max() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::max(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return max_;
}

/// @brief The range of values.
/// @details Must be finalized first!
core::Real
CentralTendencyEnsembleMetric::range() const {
	runtime_assert_string_msg( finalized(), "Error in CentralTendencyEnsembleMetric::range(): The CentralTendencyEnsembleMetric has not been finalized!" );
	return range_;
}

////////////////////////////////////////////////////////////////////////////////
// Creator functions
////////////////////////////////////////////////////////////////////////////////

void
CentralTendencyEnsembleMetricCreator::provide_xml_schema(
	utility::tag::XMLSchemaDefinition & xsd
) const {
	CentralTendencyEnsembleMetric::provide_xml_schema( xsd );
}

std::string
CentralTendencyEnsembleMetricCreator::keyname() const {
	return CentralTendencyEnsembleMetric::name_static();
}

protocols::ensemble_metrics::EnsembleMetricOP
CentralTendencyEnsembleMetricCreator::create_ensemble_metric() const {
	return utility::pointer::make_shared< CentralTendencyEnsembleMetric >();
}

} //metrics
} //ensemble_metrics
} //protocols

#ifdef    SERIALIZATION

template< class Archive >
void
protocols::ensemble_metrics::metrics::CentralTendencyEnsembleMetric::save( Archive & arc ) const {
	arc( cereal::base_class< protocols::ensemble_metrics::EnsembleMetric >( this ) );
	arc( CEREAL_NVP( simple_metric_ ) );
	arc( CEREAL_NVP( values_ ) );
	arc( CEREAL_NVP( mean_ ) );
	arc( CEREAL_NVP( median_ ) );
	arc( CEREAL_NVP( mode_ ) );
	arc( CEREAL_NVP( stderr_ ) );
	arc( CEREAL_NVP( stddev_ ) );
	arc( CEREAL_NVP( min_ ) );
	arc( CEREAL_NVP( max_ ) );
	arc( CEREAL_NVP( range_ ) );
	arc( CEREAL_NVP( derived_finalized_ ) );
}

template< class Archive >
void
protocols::ensemble_metrics::metrics::CentralTendencyEnsembleMetric::load( Archive & arc ) {
	arc( cereal::base_class< protocols::ensemble_metrics::EnsembleMetric >( this ) );
	arc( simple_metric_ );
	arc( values_ );
	arc( mean_ );
	arc( median_ );
	arc( mode_ );
	arc( stderr_ );
	arc( stddev_ );
	arc( min_ );
	arc( max_ );
	arc( range_ );
	arc( derived_finalized_ );
}

SAVE_AND_LOAD_SERIALIZABLE( protocols::ensemble_metrics::metrics::CentralTendencyEnsembleMetric );
CEREAL_REGISTER_TYPE( protocols::ensemble_metrics::metrics::CentralTendencyEnsembleMetric )

CEREAL_REGISTER_DYNAMIC_INIT( protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric )
#endif // SERIALIZATION
