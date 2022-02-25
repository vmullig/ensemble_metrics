// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetric.cc), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/EnsembleMetric.cc
/// @brief Pure virtual base class for ensemble metrics, which measure properties of an ensemble of poses.
/// @details Ensemble metrics expect to receive poses one by one, accumulating data internally as they
/// do.  At the end of a protocol, an ensemble metric can generate a report (written to tracer or to disk)
/// about the ensemble of poses that it has seen.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

// Project headers:
#include <protocols/ensemble_metrics/EnsembleMetric.hh>
#include <protocols/ensemble_metrics/util.hh>

// Core headers:
#include <core/pose/Pose.hh>

// Protocols headers:
#include <protocols/jd2/util.hh>
#include <protocols/moves/Mover.hh>
#include <protocols/rosetta_scripts/util.hh>

// Basic headers:
#include <basic/Tracer.hh>
#include <basic/thread_manager/RosettaThreadManager.hh>
#include <basic/thread_manager/RosettaThreadAssignmentInfo.hh>

// Utility headers:
#include <utility/tag/Tag.hh>
#include <utility/io/ozstream.hh>
#include <utility/pointer/memory.hh>
#include <utility/tag/XMLSchemaGeneration.hh>
#include <utility/file/file_sys_util.hh>

//STL headers:
#include <functional>

#ifdef    SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/polymorphic.hpp>
#endif // SERIALIZATION

static basic::Tracer TR( "protocols.ensemble_metrics.EnsembleMetric" );


namespace protocols {
namespace ensemble_metrics {

////////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR AND DESTRUCTOR
////////////////////////////////////////////////////////////////////////////////

/// @brief Default constructor.
EnsembleMetric::EnsembleMetric() = default;

/// @brief Copy constructor.
/// @details Has to be explicit because std::mutex has a deleted copy constructor.
EnsembleMetric::EnsembleMetric(
	EnsembleMetric const &src
) :
	VirtualBase( src ),
	finalized_( src.finalized_ ),
	use_additional_output_from_last_mover_( src.use_additional_output_from_last_mover_ ),
	output_mode_( src.output_mode_ ),
	output_filename_( src.output_filename_ ),
	label_prefix_( src.label_prefix_ ),
	label_suffix_( src.label_suffix_ ),
	last_mover_( src.last_mover_ == nullptr ? nullptr : src.last_mover_->clone() ),
	ensemble_generating_protocol_( src.ensemble_generating_protocol_ == nullptr ? nullptr : src.ensemble_generating_protocol_->clone() ),
	ensemble_generating_protocol_repeats_( src.ensemble_generating_protocol_repeats_ ),
	poses_in_ensemble_( src.poses_in_ensemble_ ),
	n_threads_( src.n_threads_ )
{}

/// @brief Assignment operator.
/// @details Has to be explicit because std::mutex has a deleted assignment operator.
EnsembleMetric &
EnsembleMetric::operator=(
	EnsembleMetric const &src
) {
	if ( &src == this ) {
		return *this; //Do nothing if we're copying from ourself.
	}
	finalized_ = src.finalized_;
	use_additional_output_from_last_mover_ = src.use_additional_output_from_last_mover_;
	output_mode_ = src.output_mode_;
	output_filename_ = src.output_filename_;
	label_prefix_ = src.label_prefix_;
	label_suffix_ = src.label_suffix_;
	last_mover_ = ( src.last_mover_ == nullptr ? nullptr : src.last_mover_->clone() );
	ensemble_generating_protocol_ = ( src.ensemble_generating_protocol_ == nullptr ? nullptr : src.ensemble_generating_protocol_->clone() );
	ensemble_generating_protocol_repeats_ = src.ensemble_generating_protocol_repeats_;
	poses_in_ensemble_ = src.poses_in_ensemble_;
	n_threads_ = src.n_threads_;
	return *this;
}

/// @brief Destructor.
/// @note On destruction, an ensemble metric that has not yet reported does its final report.  This
/// behaviour must be implemented by derived classes due to order of calls to destructors.
EnsembleMetric::~EnsembleMetric() = default;

////////////////////////////////////////////////////////////////////////////////
// STATIC ENUM FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Given an output mode name, get the enum.
/// @details Returns UNKNOWN_MODE if string can't be interpreted.
EnsembleMetricOutputMode
EnsembleMetric::output_mode_enum_from_name(
	std::string const & mode_name
) {
	for ( core::Size i(1); i <= static_cast<core::Size>(EnsembleMetricOutputMode::N_OUTPUT_MODES); ++i ) {
		if ( mode_name == output_mode_name_from_enum( static_cast< EnsembleMetricOutputMode >(i) ) ) {
			return static_cast< EnsembleMetricOutputMode >(i);
		}
	}
	return EnsembleMetricOutputMode::UNKNOWN_MODE;
}

/// @brief Given an output mode enum, get the name.
/// @details Throws if bad mode.
std::string
EnsembleMetric::output_mode_name_from_enum(
	EnsembleMetricOutputMode const mode_enum
) {
	switch( mode_enum ) {
	case EnsembleMetricOutputMode::TRACER :
		return "tracer";
	case EnsembleMetricOutputMode::TRACER_AND_FILE :
		return "tracer_and_file";
	case EnsembleMetricOutputMode::FILE :
		return "file";
	default :
		utility_exit_with_message( "Error in EnsembleMetric::output_mode_name_from_enum(): Unknown enum found!  This should not happen.  Please consult a developer." );
	};
	return "FAIL"; //Should never reach here; keeps compiler happy.
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC APPLY FUNCTION (NOT VIRUTAL)
////////////////////////////////////////////////////////////////////////////////

/// @brief Measure data from the current pose.
/// @details This function will do one of three things:
/// (1) If the previous mover was a multiple pose mover, it calls the implementation of add_pose_to_ensemble()
/// to collect data on each pose generated, then provides its report.
/// (2) If the previous mover was not a multiple pose mover and a parsed protocol was provided, it runs the
/// parsed protocol N times, calling add_pose_to_ensemble() on each pose generated, then provides its report.
/// (3) If the previous mover is not a multiple pose mover and no parsed protocol has been provided,
/// it calls the implementation of add_pose_to_ensemble() for the current pose, storing data about the
/// current pose.  The report is not provided until the end of the RosettaScripts script, or when
/// produce_final_report() is called.
/// @note This function is deliberately NOT virtual.  The overrides are in add_pose_to_ensemble(), which
/// this function calls as appropriate.
void
EnsembleMetric::apply(
	core::pose::Pose const & pose
) {
	std::string const errmsg( "Error in EnsembleMetric::apply(): " );
	runtime_assert_string_msg(
		finalized_ == false, errmsg + "The " + name() + " ensemble metric "
		"has already been finalized (i.e. produced its final report).  The reset() function "
		"must be called before accumulating more data from fresh input poses."
	);

#ifdef USEMPI
	if( ensemble_generating_protocol_ == nullptr && !use_additional_output_from_last_mover_ ) {
		runtime_assert_string_msg(
			supports_mpi(),
			errmsg + "The " + name() + " ensemble metric does not support collection of results by "
			"MPI.  To use this ensemble metric in an MPI context, you must provide an ensmeble-generating "
			"protocol, or set the use_addtional_output_from_last_mover option to true."
		);
	}
#endif

	if ( ensemble_generating_protocol_ == nullptr ) {
		++poses_in_ensemble_;
		add_pose_to_ensemble( pose );
		if ( use_additional_output_from_last_mover_ && last_mover_ != nullptr ) {
			protocols::moves::MoverOP mover_copy( last_mover_->clone() );
			core::pose::PoseOP curpose;
			do {
				curpose = mover_copy->get_additional_output();
				if ( curpose == nullptr ) break;
				++poses_in_ensemble_;
				add_pose_to_ensemble( *curpose );
			} while(true);
			produce_final_report();
		}
	} else {
		generate_ensemble_and_apply_to_poses( pose );
		produce_final_report();
	}
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC REPORTING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Write the final report produced by this metric to a file or to tracer.
/// @details If output_mode_ == EnsembleMetricOutputMode::TRACER, writes to tracer.
/// Writes to disk if output_mode_ == EnsembleMetricOutputMode::FILE!
void
EnsembleMetric::produce_final_report() {
	switch( output_mode_ ) {
	case EnsembleMetricOutputMode::TRACER :
		produce_final_report_to_tracer( get_derived_tracer() );
		break;
	case EnsembleMetricOutputMode::TRACER_AND_FILE :
		produce_final_report_to_tracer( get_derived_tracer() );
		produce_final_report_to_file( output_filename_);
		break;
	case EnsembleMetricOutputMode::FILE :
		produce_final_report_to_file( output_filename_ );
		break;
	default :
		utility_exit_with_message( "Invalid output mode for EnsembleMetric " + name() + "!" );
	}
	finalized_ = true;
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC ROSETTASCRIPTS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Generate the type name for the RosettaScripts XSD.
utility::tag::XMLSchemaComplexTypeGeneratorOP
EnsembleMetric::complex_type_generator_for_ensemble_metric(
	utility::tag::XMLSchemaDefinition const & /*xsd*/
) {
	using namespace utility::tag;

	AttributeList attlist;

	attlist
		+ XMLSchemaAttribute( "label_prefix", xs_string,
		"If provided, this prefix is prepended to the label for this ensemble metric (with an underscore after the "
		"prefix and before the ensemble metric name)."
		)
		+ XMLSchemaAttribute( "label_suffix", xs_string,
		"If provided, this suffix is appended to the label for this ensemble metric (with an underscore after the "
		"ensemble metric name and before the suffix)."
		)
		+ XMLSchemaAttribute::attribute_w_default( "output_mode", xs_string,
		"The output mode for reports from this ensemble metric.  Default is 'tracer'.  Allowed modes are: 'tracer', "
		"'tracer_and_file', or 'file'.",
		"tracer"
		)
		+ XMLSchemaAttribute( "output_filename", xs_string,
		"The file to which the ensemble metric report will be written if output mode is 'tracer_and_file' or 'file'.  Note that "
		"this filename will have the job name and number prepended so that each report is unique."
		)
		+ XMLSchemaAttribute( "ensemble_generating_protocol", xs_string,
		"An optional ParsedProtocol or other mover for generating an ensemble from the current pose.  "
		"This protocol will be applied repeatedly (ensemble_generating_protocol_repeats times) to generate "
		"the ensemble of structures.  Each generated pose will be measured by this metric, then discarded.  "
		"The ensemble properties are then reported.  If not provided, the current pose is measured and the "
		"report will be produced later (e.g. at termination with the JD2 rosetta_scripts application)."
		)
		+ XMLSchemaAttribute::attribute_w_default( "ensemble_generating_protocol_repeats", xsct_non_negative_integer,
		"The number of times that the ensemble_generating_protocol is applied.  This is the maximum "
		"number of structures in the ensemble (though the actual number may be smaller if "
		"the protocol contains filters or movers that can fail for some attempts).  Only used if an "
		"ensemble-generating protocol is provided with the ensemble_generating_protocol option.  Defaults "
		"to 1.",
		"1"
		)
		+ XMLSchemaAttribute::attribute_w_default( "n_threads", xsct_non_negative_integer,
		"The number of threads to request for generating ensembles in parallel.  This is only used in "
		"multi-threaded compilations of Rosetta (compiled with extras=cxx11thread), and only when an "
		"ensemble-generating protocol is provided with the ensemble_generating_protocol option.  A value "
		"of 0 means to use all available threads.  In single-threaded builds, this must be set to "
		"0 or 1.  Defaults to 1.  NOTE THAT MULTI-THREADING IS HIGHLY EXPERIMENTAL AND LIKELY TO FAIL "
		"FOR MANY ENSEMBLE-GENERATING PROTOCOLS.  When in doubt, leave this set to 1.",
		"1"
		)
		+ XMLSchemaAttribute::attribute_w_default( "use_additional_output_from_last_mover", xsct_rosetta_bool,
		"If true, this ensemble metric will use the additional output from the previous pose (assuming the previous pose "
		"generates multiple outputs) as the ensemble, analysing it and producing a report immediately.  If false, "
		"then it will behave normally.  False by default.",
		"false"
	);

	XMLSchemaComplexTypeGeneratorOP ct_gen(
		utility::pointer::make_shared< utility::tag::XMLSchemaComplexTypeGenerator >()
	);
	ct_gen->
		add_attributes( attlist )
		.complex_type_naming_func( & complex_type_name_for_ensemble_metric );

	return ct_gen;
}

/// @brief Parse XML options that are common to all EnsembleMetrics.
void
EnsembleMetric::parse_common_ensemble_metric_options(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & data
) {
	if ( tag->hasOption( "label_prefix" ) ) {
		set_label_prefix( tag->getOption< std::string >( "label_prefix" ) );
	}
	if ( tag->hasOption( "label_suffix" ) ) {
		set_label_suffix( tag->getOption< std::string >( "label_suffix" ) );
	}
	if ( tag->hasOption( "ensemble_generating_protocol" ) ) {
		set_ensemble_generating_protocol(
			protocols::rosetta_scripts::parse_mover( tag->getOption< std::string >( "ensemble_generating_protocol" ), data )
		);
		if ( tag->hasOption( "ensemble_generating_protocol_repeats" ) ) {
			set_ensemble_generating_protocol_repeats( tag->getOption<core::Size>( "ensemble_generating_protocol_repeats" ) );
		}
	} else {
		if ( tag->hasOption( "ensemble_generating_protocol_repeats" ) ) {
			TR.Warning << "WARNING! The ensemble_generating_protocol_repeats option has no effect if no ensemble-generating protocol is provided with the ensemble_generating_protocol option." << std::endl;
		}
	}
	if ( tag->hasOption( "n_threads" ) ) {
		set_n_threads( tag->getOption<core::Size>( "n_threads" ) );
	}
	if ( tag->hasOption("use_additional_output_from_last_mover") ) {
		set_use_additional_output_from_last_mover( tag->getOption<bool>("use_additional_output_from_last_mover") );
	}
	if ( tag->hasOption( "output_mode" ) ) {
		set_output_mode( tag->getOption< std::string >( "output_mode" ) );
	}
	if ( tag->hasOption( "output_filename" ) ) {
		runtime_assert_string_msg(
			output_mode_ != EnsembleMetricOutputMode::TRACER,
			"Error in EnsembleMetric::parse_common_ensemble_metric_options(): The output filename was set, but output "
			"mode is set to tracer only!  This must be set to \"file\" or \"tracer_and_file\"."
		);
		set_output_filename( tag->getOption< std::string >( "output_filename" ) );
	}

#ifdef USEMPI
	if( ensemble_generating_protocol_ == nullptr && !use_additional_output_from_last_mover_ ) {
		runtime_assert_string_msg(
			supports_mpi(),
			"Error in EnsembleMetric::parse_common_ensemble_metric_options(): The " + name() + " ensemble "
			"metric does not support collection of results by MPI.  To use this ensemble metric in an MPI "
			"context, you must provide an ensmeble-generating protocol, or set the "
			"use_addtional_output_from_last_mover option to true."
		);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC SETTERS
////////////////////////////////////////////////////////////////////////////////

/// @brief Reset this ensemble metric.  Calls derived_reset() to reset
/// the data collected by the derived class.
void
EnsembleMetric::reset() {
	poses_in_ensemble_ = 0;
	finalized_ = false;
	derived_reset();
}

/// @brief Set the optional prefix added to the start of the label for this metric.
void
EnsembleMetric::set_label_prefix(
	std::string const & setting
) {
	label_prefix_ = setting;
}

/// @brief Set the optional suffix added to the end of the label for this metric.
void
EnsembleMetric::set_label_suffix(
	std::string const & setting
) {
	label_suffix_ = setting;
}

/// @brief Set the protocol that will generate an ensemble of states.
/// @details If not set, the ensemble metric just collects data from the current pose.  If set,
/// the ensemble metric runs this N times to generate N poses, collects data from each, and then
/// reports on the generated ensemble.
/// @note Input owning pointer is stored directly; object is not cloned.
void
EnsembleMetric::set_ensemble_generating_protocol(
	protocols::moves::MoverCOP const & protocol_in
) {
	ensemble_generating_protocol_ = protocol_in;
}

/// @brief Set the number of times that the ensemble-generating protocol is run (the maximum size of
/// ensemble generated).  Defaults to 1.  Only used if an ensemble generating protocol is provided.
void
EnsembleMetric::set_ensemble_generating_protocol_repeats(
	core::Size const setting
) {
	runtime_assert_string_msg( setting > 0, "Error in EnsembleMetric::set_ensemble_generating_protocol_repeats(): The number of replicates of the ensemble-generating protocol must be 1 or more." );
	ensemble_generating_protocol_repeats_ = setting;
}

/// @brief Set output mode by string.
void
EnsembleMetric::set_output_mode(
	std::string const & mode_string
) {
	EnsembleMetricOutputMode const mode_enum( output_mode_enum_from_name( mode_string ) );
	runtime_assert_string_msg( mode_enum != EnsembleMetricOutputMode::UNKNOWN_MODE, "Error in EnsembleMetric::set_output_mode(): \"" + mode_string + "\" is not a valid output mode." );
	set_output_mode( mode_enum );
}

/// @brief Set output mode.
/// @details Indicate where output will be directed.
void
EnsembleMetric::set_output_mode(
	EnsembleMetricOutputMode const setting
) {
	runtime_assert( setting > EnsembleMetricOutputMode::UNKNOWN_MODE && setting <= EnsembleMetricOutputMode::N_OUTPUT_MODES );
	output_mode_ = setting;
}

/// @brief Set the output file, if output_mode_ == EnsembleMetricOutputMode::FILE.
void
EnsembleMetric::set_output_filename(
	std::string const & setting
) {
	output_filename_ = setting;
}

/// @brief Set the last mover that ran before this ensemble metric.
/// @details Only used to get additional output, if any, and only if use_additional_output_ is true.
void
EnsembleMetric::set_previous_mover(
	protocols::moves::MoverCOP const mover_in
) {
	last_mover_ = mover_in;
}

/// @brief Set whether we use the additional output from the last mover as the source of the ensemble.
void
EnsembleMetric::set_use_additional_output_from_last_mover(
	bool const setting
) {
	use_additional_output_from_last_mover_ = setting;
}

/// @brief Set the number of threads to request.  Zero means to request all available.
void
EnsembleMetric::set_n_threads(
	core::Size const setting
) {
	n_threads_ = setting;
#ifndef MULTI_THREADED
	runtime_assert_string_msg( n_threads_ < 2, "Error in EnsembleMetric::set_n_threads(): The number of threads must be set to 0 (meaning use all available) or 1 in single-threaded builds of Rosetta.  To use more threads, build Rosetta with extras=cxx11thread." );
#endif
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC GETTERS
////////////////////////////////////////////////////////////////////////////////

/// @brief Is the configuration set so that this metric expects to give its report at the end of
/// a protocol (true) or immediately after internally generating an ensemble or inheriting an ensemble
/// from a multiple pose mover (false)?
bool
EnsembleMetric::reports_at_end() const {
	return ( ensemble_generating_protocol_ == nullptr && !use_additional_output_from_last_mover_ );
}

/// @brief Get the label.
/// @details By default, this is just the name().  If a prefix is provided, it is prepended
/// followed by an underscore; if a suffix is provided, it is appended preceded by an underscore.
std::string
EnsembleMetric::get_ensemble_metric_label() const {
	std::ostringstream ss;
	if ( !label_prefix_.empty() ) {
		ss << label_prefix_ << "_";
	}
	ss << name();
	if ( !label_suffix_.empty() ) {
		ss << "_" << label_suffix_;
	}
	return ss.str();
}

/// @brief Get the number of poses in the ensemble so far.
/// @details Calling reset() resets this.
core::Size
EnsembleMetric::poses_in_ensemble() const {
	return poses_in_ensemble_;
}

/// @brief Given a metric name, get its value.
/// @details Calls derived_get_real_metric_value_by_name().
core::Real
EnsembleMetric::get_real_metric_value_by_name(
	std::string const & metric_name
) const {
	std::string const errmsg( "Error in EnsembleMetric::get_real_metric_value_by_name(): " );
	runtime_assert_string_msg( finalized_, errmsg + "The final report has not yet been generated for the " + name() + " ensemble metric." );
	runtime_assert_string_msg( real_valued_metric_names().has_value( metric_name ), errmsg + "Metric name \"" + metric_name + "\" was requested, but the " + name() + " ensemble metric produces no such real-valued metric." );
	return derived_get_real_metric_value_by_name( metric_name );
}

/// @brief Get the ensemble generating protocol.
/// @details Could be nullptr if none is set.
protocols::moves::MoverCOP
EnsembleMetric::ensemble_generating_protocol() const {
	return ensemble_generating_protocol_;
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC CITATION MANAGER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Provide citations to the passed CitationCollectionList
/// Subclasses should add the info for themselves and any other classes they use.
/// @details The default implementation of this function does nothing.  It may be
/// overriden by ensemble metrics wishing to provide citation information.
void
EnsembleMetric::provide_citation_info(
	basic::citation_manager::CitationCollectionList &
) const {
	//GNDN
}

////////////////////////////////////////////////////////////////////////////////
// PROTECTED FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Allow derived classes to indicate that additional poses have been
/// added to the ensemble.
void
EnsembleMetric::increment_poses_in_ensemble(
	core::Size const n_additional_poses
) {
	poses_in_ensemble_ += n_additional_poses;
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC MPI PARALLEL COMMUNICATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

#ifdef USEMPI

/// @brief Does this EnsembleMetric support MPI-based collection of ensemble properties from an ensemble
/// sampled in a distributed manner?  The default implementation returns false; derived classes that support
/// MPI must override this to return true.  IF THIS FUNCTION IS OVERRIDDEN, BE SURE TO IMPLEMENT OVERRIDES
/// FOR send_mpi_summary() AND recv_mpi_summary()!
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
EnsembleMetric::supports_mpi() const {
	return false;
}

/// @brief Send all of the data collected by this EnsembleMetric to another node.  The base class implementation
/// throws, so this must be overridden by any derived EnsembleMetric class that supports MPI.  IF THIS FUNCTION
/// IS OVERRIDDEN, BE SURE TO IMPLEMENT OVERRIDES FOR recv_mpi_summary() AND supports_mpi()!
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
EnsembleMetric::send_mpi_summary(
	core::Size const /*receiving_node_index*/
) const {
	utility_exit_with_message( "Error in EnsembleMetric::send_mpi_summary(): The " + name() + " ensemble metric "
		"does not support distributed ensemble generation and analysis with MPI.  This function must be overridden "
		"to enable support."
	);
}

/// @brief Receive all of the data collected by this EnsembleMetric on another node.  The base class implementation
/// throws, so this must be overridden by any derived EnsembleMetric class that supports MPI.  IF THIS FUNCTION
/// IS OVERRIDDEN, BE SURE TO IMPLEMENT OVERRIDES FOR send_mpi_summary() AND supports_mpi()!  Note that this should
/// receive from any MPI process, and report the process index that it received from.
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
EnsembleMetric::recv_mpi_summary() {
	utility_exit_with_message( "Error in EnsembleMetric::recv_mpi_summary(): The " + name() + " ensemble metric "
		"does not support distributed ensemble generation and analysis with MPI.  This function must be overridden "
		"to enable support."
	);
	return 0; //Keep older compiler happy.
}

#endif //USEMPI

////////////////////////////////////////////////////////////////////////////////
// PRIVATE REPORTING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Write the final report to the tracer.
void
EnsembleMetric::produce_final_report_to_tracer(
	basic::Tracer & tracer
) {
	runtime_assert(
		output_mode_ == EnsembleMetricOutputMode::TRACER ||
		output_mode_ == EnsembleMetricOutputMode::TRACER_AND_FILE
	);
	tracer << "Report from " << name() << ":\n";
	if ( protocols::jd2::jd2_used() ) {
		tracer << "\tjob_name:\t" << protocols::jd2::current_output_name() << "\n";
		tracer << "\tjob_nstruct_index:\t" << protocols::jd2::current_nstruct_index() << "\n";
	}
#ifdef USEMPI
	int mpirank;
	MPI_Comm_rank( MPI_COMM_WORLD, &mpirank );
	tracer << "\tMPI_process:\t" << mpirank << "\n";
#endif
	tracer << "\tposes_in_ensemble:\t" << poses_in_ensemble() << "\n";
	tracer << produce_final_report_string() << std::endl;
}

/// @brief Write the final report to an output file.
void
EnsembleMetric::produce_final_report_to_file(
	std::string const & output_file
) {
	runtime_assert(
		output_mode_ == EnsembleMetricOutputMode::FILE ||
		output_mode_ == EnsembleMetricOutputMode::TRACER_AND_FILE
	);
	runtime_assert_string_msg( !output_file.empty(), "Error in EnsembleMetric::produce_final_report_to_file(): An output file must be set in order to use file output." );

	std::string const output_file_basename( utility::file::file_basename( output_file ) );
	std::string const output_file_extn( utility::file::file_extension( output_file ) );

	bool const jd2_used( protocols::jd2::jd2_used() );
	std::string const jobstring(
		jd2_used ? protocols::jd2::current_output_name() : ""
	);

#ifdef USEMPI
	int mpirank;
	MPI_Comm_rank( MPI_COMM_WORLD, &mpirank );
#endif

	std::string const output_file_fullname(
		label_prefix_ +
		(label_prefix_.empty() ? "" : "_") +
		jobstring +
		(jobstring.empty() ? "" : "_" ) +
#ifdef USEMPI
		"proc_" + std::to_string( mpirank ) + "_" +
#endif
		output_file_basename +
		( label_suffix_.empty() ? "" : "_" ) +
		label_suffix_ +
		( output_file_extn.empty() ? "" : "." ) +
		output_file_extn
	);

	std::string output( "Report from " + name() + ":\n" );
	if ( protocols::jd2::jd2_used() ) {
		output += "\tjob_name:\t" + protocols::jd2::current_output_name() + "\n"
			+ "\tjob_nstruct_index:\t" + std::to_string( protocols::jd2::current_nstruct_index() ) + "\n";
	}
#ifdef USEMPI
		output += "\tMPI_process:\t" + std::to_string( mpirank ) + "\n";
#endif
	output += "\tposes_in_ensemble:\t" + std::to_string( poses_in_ensemble() ) + "\n" + produce_final_report_string() + "\n";
	utility::io::ozstream outfile( output_file_fullname );
	outfile << output;
	outfile.close();
	TR << "Wrote " << name() << " ensemble metric output to file \"" << output_file_fullname << "\"." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE CALCULATION FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

/// @brief Called by apply() function if and only if an ensemble-generating protocol is provided.
/// @details Generates an ensemble of poses (in parallel, if multi-threading is enabled) and measures
/// properties of each.
void
EnsembleMetric::generate_ensemble_and_apply_to_poses(
	core::pose::Pose const & pose
) {
	utility::vector1< basic::thread_manager::RosettaThreadFunction > workvec;
	workvec.reserve( ensemble_generating_protocol_repeats_ );

	bool const doing_multiple_outputs( last_mover_ != nullptr && use_additional_output_from_last_mover_ );

	// Set up the work vector:
	for ( core::Size i(1); i<=ensemble_generating_protocol_repeats_; ++i ) {
		workvec.push_back(
			std::bind(
			&EnsembleMetric::generate_one_ensemble_entry, this,
			i,
			std::cref( pose ),
			std::cref( *ensemble_generating_protocol_ ),
			(doing_multiple_outputs ? last_mover_->clone() : nullptr)
			)
		);
	}

	basic::thread_manager::RosettaThreadAssignmentInfo thread_assignments( basic::thread_manager::RosettaThreadRequestOriginatingLevel::PROTOCOLS_GENERIC );
	TR << "Generated a work vector of " << workvec.size() << " jobs.";
#ifdef MULTI_THREADED
	TR << "  Requesting " << ( n_threads_ == 0 ? "all available" : std::to_string( n_threads_ ) ) << " threads for calculation." << std::endl;
#endif
	basic::thread_manager::RosettaThreadManager::get_instance()->do_work_vector_in_threads( workvec, n_threads_, thread_assignments );
	TR << "Completed " << workvec.size() << " jobs";
#ifdef MULTI_THREADED
	TR << " using " << thread_assignments.get_assigned_total_thread_count() << " available threads.  ";
#else
	TR << ".  ";
#endif
	TR << poses_in_ensemble_ << " poses are in the ensemble." << std::endl;
}

/// @brief Given a protocol and a pose, clone the pose, clone the protocol, apply the protocol to the pose,
/// and collect stats on the resulting pose.
/// @details Runs multi-threaded in multi-threaded builds of Rosetta.  Intended to be called by the RosettaThreadManager.
/// Called from the work vector generated in generate_ensemble_and_apply_to_poses().
void
EnsembleMetric::generate_one_ensemble_entry(
	core::Size const attempt_index,
	core::pose::Pose const & master_pose,
	protocols::moves::Mover const & master_protocol,
	protocols::moves::MoverOP last_mover_copy
) {
	basic::Tracer & TR_derived( get_derived_tracer() );

	// Make thread-local copies of pose and protocol.
	core::pose::PoseOP my_pose( utility::pointer::make_shared< core::pose::Pose >() );
	protocols::moves::MoverOP my_protocol;
	{
#ifdef MULTI_THREADED
		std::lock_guard< std::mutex > lock( pose_mutex_ );
#endif
		my_pose->detached_copy( master_pose ); // Detached copy the pose.
	}

	// Clone the protocol.
	{
#ifdef MULTI_THREADED
		std::lock_guard< std::mutex > lock( ensemble_generating_protocol_mutex_ );
#endif
		my_protocol = master_protocol.clone();
	}

	core::Size counter(0);

	// The following loop is for getting output from a previous multiple
	// pose mover.  The first pass applies the ensemble generating mover
	// to an input pose.  If there's no multiple pose mover, we're done,
	// and we return.  If there is a multiple pose mover, we keep looping
	// and applying the protocol to each pose in turn until we run out
	// of poses:
	do {
		// Apply to the current pose.  If we're not using a multiple pose
		// mover, return here if the mover fails on the primary pose.
		my_protocol->apply( *my_pose );
		if ( my_protocol->get_last_move_status() != protocols::moves::MoverStatus::MS_SUCCESS ) {
			if ( last_mover_copy == nullptr ) {
				TR_derived << "Attempt " << attempt_index << " failed.  Continuing on..." << std::endl;
				return;
			} else {
				TR_derived << "Attempt " << attempt_index << "-" << counter << " failed.  Continuing on..." << std::endl;
			}
		} else {
#ifdef MULTI_THREADED
			std::lock_guard< std::mutex > lock( ensemble_metric_mutex_ );
#endif
			++poses_in_ensemble_;
			add_pose_to_ensemble( *my_pose );
			if ( last_mover_copy == nullptr ) {
				TR_derived << name() << " ensemble metric generated ensemble entry " << attempt_index << " and added its measurements to the ensemble." << std::endl;
			} else {
				TR_derived << name() << " ensemble metric generated ensemble entry " << attempt_index << "-" << counter << " and added its measurements to the ensemble." << std::endl;
			}
		}

		// Get additional poses from the multiple pose mover, if any:
		if ( last_mover_copy != nullptr ) {
			my_pose->clear();
#ifdef MULTI_THREADED
			std::lock_guard< std::mutex > lock( pose_mutex_ );
#endif
			my_pose->detached_copy( *last_mover_copy->get_additional_output() );
			if ( my_pose != nullptr ) {
				++counter;
			}
		} else {
			break;
		}
	} while( my_pose != nullptr );
}

} //ensemble_metrics
} //protocols

#ifdef SERIALIZATION

template< class Archive >
void
protocols::ensemble_metrics::EnsembleMetric::save( Archive & arc ) const {
	arc( CEREAL_NVP( finalized_ ) );
	arc( CEREAL_NVP( use_additional_output_from_last_mover_ ) );
	arc( CEREAL_NVP( output_mode_ ) );
	arc( CEREAL_NVP( output_filename_ ) );
	arc( CEREAL_NVP( label_prefix_ ) );
	arc( CEREAL_NVP( label_suffix_ ) );
	arc( CEREAL_NVP( last_mover_ ) );
	arc( CEREAL_NVP( ensemble_generating_protocol_ ) );
	arc( CEREAL_NVP( ensemble_generating_protocol_repeats_ ) );
	arc( CEREAL_NVP( poses_in_ensemble_ ) );
	arc( CEREAL_NVP( n_threads_ ) );
}

template< class Archive >
void
protocols::ensemble_metrics::EnsembleMetric::load( Archive & arc ) {
	arc( finalized_ );
	arc( use_additional_output_from_last_mover_ );
	arc( output_mode_ );
	arc( output_filename_ );
	arc( label_prefix_ );
	arc( label_suffix_ );
	arc( last_mover_ );
	arc( ensemble_generating_protocol_ );
	arc( ensemble_generating_protocol_repeats_ );
	arc( poses_in_ensemble_ );
	arc( n_threads_ );
}

SAVE_AND_LOAD_SERIALIZABLE( protocols::ensemble_metrics::EnsembleMetric );
CEREAL_REGISTER_TYPE( protocols::ensemble_metrics::EnsembleMetric )

CEREAL_REGISTER_DYNAMIC_INIT( protocols_ensemble_metrics_EnsembleMetric )
#endif // SERIALIZATION
