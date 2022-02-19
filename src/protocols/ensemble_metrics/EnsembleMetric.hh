// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetric.hh), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/EnsembleMetric.hh
/// @brief Pure virtual base class for ensemble metrics, which measure properties of an ensemble of poses.
/// @details Ensemble metrics expect to receive poses one by one, accumulating data internally as they
/// do.  At the end of a protocol, an ensemble metric can generate a report (written to tracer or to disk)
/// about the ensemble of poses that it has seen.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)


#ifndef INCLUDED_protocols_ensemble_metrics_EnsembleMetric_hh
#define INCLUDED_protocols_ensemble_metrics_EnsembleMetric_hh

#include <protocols/ensemble_metrics/EnsembleMetric.fwd.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>
#include <core/types.hh>

// Protocols headers
#include <protocols/moves/Mover.fwd.hh>

// Basic headers
#include <basic/citation_manager/CitationCollectionBase.fwd.hh>
#include <basic/Tracer.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>

// Utility headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/VirtualBase.hh>
#include <utility/tag/XMLSchemaGeneration.fwd.hh>
#include <utility/tag/Tag.fwd.hh>

//STL headers
#include <string>

#ifdef MULTI_THREADED
#include <mutex>
#endif

#ifdef    SERIALIZATION
// Cereal headers
#include <cereal/types/polymorphic.fwd.hpp>
#endif // SERIALIZATION

namespace protocols {
namespace ensemble_metrics {

/// @brief List of output modes.  If you add to this list, update
/// EnsembleMetric::output_mode_name_from_enum().
enum class EnsembleMetricOutputMode {
	UNKNOWN_MODE = 0, //Keep first.
	TRACER,
	TRACER_AND_FILE,
	FILE, //Keep second-to-last.
	N_OUTPUT_MODES = FILE //Keep last.
};

/// @brief Pure virtual base class for ensemble metrics, which measure properties of an ensemble of poses.
/// @details Ensemble metrics expect to receive poses one by one, accumulating data internally as they
/// do.  At the end of a protocol, an ensemble metric can generate a report (written to tracer or to disk)
/// about the ensemble of poses that it has seen.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
class EnsembleMetric : public utility::VirtualBase {

public:

	/// @brief Default constructor.
	EnsembleMetric();

	/// @brief Copy constructor.
	EnsembleMetric( EnsembleMetric const & );

	/// @brief Assignment operator.
	EnsembleMetric & operator=( EnsembleMetric const & );

	/// @brief Destructor.
	/// @note On destruction, an ensemble metric that has not yet reported does its final report.  This
	/// behaviour must be implemented by derived classes due to order of calls to destructors.
	~EnsembleMetric() override;

	/// @brief Clone operation: make a copy of this object, and return an owning pointer to the copy.
	/// @details Pure virtual.  Must be implemented by derived classes.
	virtual
	EnsembleMetricOP
	clone() const = 0;

public: // Public pure virtual functions

	/// @brief Provide the name of this EnsmebleMetric.
	/// @details Pure virtual.  Must be implemented by derived classes.
	virtual
	std::string
	name() const = 0;

	/// @brief Get a list of the names of the real-valued metrics that can be filtered on (e.g. by the EnsembleMetricFilter)
	/// or otherwise extracted from this EnsembleMetric.  Must be implemented by derived classes.  (Can be empty list if
	/// no real-valued metrics are computed.)
	virtual
	utility::vector1< std::string > const &
	real_valued_metric_names() const = 0;

private: // Private pure virtual functions

	/// @brief Write the final report produced by this metric to a string.
	/// @details This function is pure virtual, and must be implemented by derived classes.
	/// @note Output should not be terminated in a newline.
	virtual
	std::string
	produce_final_report_string() = 0;

	/// @brief Add another pose to the ensemble seen so far.  Nonconst to allow data to be
	/// accumulated.  Must be implemented by derived classes.
	virtual
	void
	add_pose_to_ensemble(
		core::pose::Pose const & pose
	) = 0;

	/// @brief Given a metric name, get its value.
	/// @details Must be implemented by derived classes.
	virtual
	core::Real
	derived_get_metric_by_name(
		std::string const & metric_name
	) const = 0;

	/// @brief Get the tracer for a derived class.
	/// @details Pure virtual.  Must be implemented for each derived class.
	virtual
	basic::Tracer &
	get_derived_tracer() const = 0;

	/// @brief Reset the data collected by the derived classes.  Must be
	/// implemented by derived classes.
	/// @note Note that the implementation should only reset the accumulated
	/// data and any values calculated from it, not the configuration.  The
	/// intent is to be able to call this and then to accumulate new data from
	/// a new ensemble of poses and generate a new report with the same settings.
	virtual
	void
	derived_reset() = 0;

public: // Static enum functions

	/// @brief Given an output mode name, get the enum.
	/// @details Returns UNKNOWN_MODE if string can't be interpreted.
	static
	EnsembleMetricOutputMode
	output_mode_enum_from_name(
		std::string const & mode_name
	);

	/// @brief Given an output mode enum, get the name.
	/// @details Throws if bad mode.
	static
	std::string
	output_mode_name_from_enum(
		EnsembleMetricOutputMode const mode_enum
	);

public: // Apply function (NOT virtual).

	/// @brief Measure data from the current pose.
	/// @details This function will do one of three things:
	/// (1) If the previous mover was a multiple pose mover, it calls the implementation of add_pose_to_ensemble()
	/// to collect data on each pose generated, then provides its report.
	/// (2) If the previous mover was not a multiple pose mover and a parsed protocol was provided, it runs the
	/// parsed protocol N times, calling add_pose_to_ensemble() on each pose generated, then provides its report.
	/// (3) If the previous mover is not a multiple pose mover and no parsed protocol has been provided,
	/// it calls the implmentation of add_pose_to_ensemble() for the current pose, storing data about the
	/// current pose.  The report is not provided until the end of the RosettaScripts script, or when
	/// produce_final_report() is called.
	/// @note This function is deliberately NOT virtual.  The overrides are in add_pose_to_ensemble(), which
	/// this function calls as appropriate.
	void
	apply(
		core::pose::Pose const & pose
	);

public: // Reporting functions

	/// @brief Write the final report produced by this metric to a file or to tracer.
	/// @details If output_mode_ == EnsembleMetricOutputMode::TRACER, writes to tracer.
	/// Writes to disk if output_mode_ == EnsembleMetricOutputMode::FILE!
	void produce_final_report();

public: // RosettaScripts functions

	/// @brief Generate the type name for the RosettaScripts XSD.
	static
	utility::tag::XMLSchemaComplexTypeGeneratorOP
	complex_type_generator_for_ensemble_metric(
		utility::tag::XMLSchemaDefinition const & xsd
	);

	/// @brief Parse XML setup.  Required for sub-classes to implement.
	virtual
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data
	) = 0;

	/// @brief Parse XML options that are common to all EnsembleMetrics.
	void
	parse_common_ensemble_metric_options(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data
	);

public: // Setters

	/// @brief Reset this ensemble metric.  Calls derived_reset() to reset
	/// the data collected by the derived class.
	void reset();

	/// @brief Set the optional prefix added to the start of the label for this metric.
	void
	set_label_prefix(
		std::string const & setting
	);

	/// @brief Set the optional suffix added to the end of the label for this metric.
	void
	set_label_suffix(
		std::string const & setting
	);

	/// @brief Set the protocol that will generate an ensemble of states.
	/// @details If not set, the ensemble metric just collects data from the current pose.  If set,
	/// the ensemble metric runs this N times to generate N poses, collects data from each, and then
	/// reports on the generated ensemble.
	/// @note Input owning pointer is stored directly; object is not cloned.
	void
	set_ensemble_generating_protocol(
		protocols::moves::MoverCOP const & protocol_in
	);

	/// @brief Set the number of times that the ensemble-generating protocol is run (the maximum size of
	/// ensemble generated).  Defaults to 1.  Only used if an ensemble generating protocol is provided.
	void
	set_ensemble_generating_protocol_repeats(
		core::Size const setting
	);

	/// @brief Set output mode by string.
	void
	set_output_mode(
		std::string const & mode_string
	);

	/// @brief Set output mode.
	/// @details Indicate where output will be directed.
	void
	set_output_mode(
		EnsembleMetricOutputMode const setting
	);

	/// @brief Set the output file, if output_mode_ == EnsembleMetricOutputMode::FILE.
	void
	set_output_filename(
		std::string const & setting
	);

	/// @brief Set the last mover that ran before this ensemble metric.
	/// @details Only used to get additional output, if any, and only if use_additional_output_ is true.
	void
	set_previous_mover(
		protocols::moves::MoverCOP const mover_in
	);

	/// @brief Set whether we use the additional output from the last mover as the source of the ensemble.
	void
	set_use_additional_output_from_last_mover(
		bool const setting
	);

	/// @brief Set the number of threads to request.  Zero means to request all available.
	void
	set_n_threads(
		core::Size const setting
	);

public: // Getters

	/// @brief Has this ensemble metric finished accumulating data and produced its report?
	inline
	bool
	finalized() const {
		return finalized_;
	}

	/// @brief Is the configuration set so that this metric expects to give its report at the end of
	/// a protocol (true) or immediately after internally generating an ensemble or inheriting an ensemble
	/// from a multiple pose mover (false)?
	bool reports_at_end() const;

	/// @brief Get whether this ensemble metric is configured to use multiple poses from the last mover
	/// as the ensemble on which it will report.
	inline
	bool
	use_additional_output_from_last_mover() const {
		return use_additional_output_from_last_mover_;
	}

	/// @brief Get output mode.
	/// @details Indicates where output will be directed.
	inline
	EnsembleMetricOutputMode
	output_mode() const {
		return output_mode_;
	}

	/// @brief Get the output file, if output_mode_ == EnsembleMetricOutputMode::FILE.
	inline
	std::string const &
	output_filename() const {
		return output_filename_;
	}

	/// @brief Get the label.
	/// @details By default, this is just the name().  If a prefix is provided, it is prepended
	/// followed by an underscore; if a suffix is provided, it is appended preceded by an underscore.
	std::string
	get_ensemble_metric_label() const;

	/// @brief Get the number of poses in the ensemble so far.
	/// @details Calling reset() resets this.
	core::Size
	poses_in_ensemble() const;

	/// @brief Given a metric name, get its value.
	/// @details Calls derived_get_metric_by_name().
	core::Real
	get_metric_by_name(
		std::string const & metric_name
	) const;

	/// @brief Get the ensemble generating protocol.
	/// @details Could be nullptr if none is set.
	protocols::moves::MoverCOP
	ensemble_generating_protocol() const;

public: // Citation manager functions

	/// @brief Provide citations to the passed CitationCollectionList
	/// Subclasses should add the info for themselves and any other classes they use.
	/// @details The default implementation of this function does nothing.  It may be
	/// overriden by ensemble metrics wishing to provide citation information.
	virtual
	void
	provide_citation_info(
		basic::citation_manager::CitationCollectionList &
	) const;

private: // Private reporting functions

	/// @brief Write the final report to the tracer.
	void produce_final_report_to_tracer( basic::Tracer & tracer );

	/// @brief Write the final report to an output file.
	void produce_final_report_to_file( std::string const & output_file );

private: // Private calculating functions

	/// @brief Called by apply() function if and only if an ensemble-generating protocol is provided.
	/// @details Generates an ensemble of poses (in parallel, if multi-threading is enabled) and measures
	/// properties of each.
	void
	generate_ensemble_and_apply_to_poses(
		core::pose::Pose const & pose
	);

	/// @brief Given a protocol and a pose, clone the pose, clone the protocol, apply the protocol to the pose,
	/// and collect stats on the resulting pose.
	/// @details Runs multi-threaded in multi-threaded builds of Rosetta.  Intended to be called by the RosettaThreadManager.
	/// Called from the work vector generated in generate_ensemble_and_apply_to_poses().
	void
	generate_one_ensemble_entry(
		core::Size const attempt_index,
		core::pose::Pose const & master_pose,
		protocols::moves::Mover const & master_protocol,
		protocols::moves::MoverOP last_mover_copy
	);

private:

	/// @brief Has this metric finished its computations and given its report?
	bool finalized_ = false;

	/// @brief Should we use the additional output from the last mover as the source
	/// of the ensemble?
	bool use_additional_output_from_last_mover_ = false;

	/// @brief Where the output is directed, by default.
	EnsembleMetricOutputMode output_mode_ = EnsembleMetricOutputMode::TRACER;

	/// @brief File to which output will be written, if output_mode_ == EnsembleMetricOutputMode::FILE.
	std::string output_filename_;

	/// @brief An optional prefix added to the start of the label for this metric.
	std::string label_prefix_;

	/// @brief An optional suffix added to the end of the label for this metric.
	std::string label_suffix_;

	/// @brief What was the last mover that was applied to the pose?
	/// @details Could be nullptr.  Only used for getting additional output if this metric is supposed to
	/// apply to the ensemble from a mover that produces many poses.
	protocols::moves::MoverCOP last_mover_;

	/// @brief An optional parsed protocol or other mover, providing the means by which a diverse ensemble will be
	/// generated from the input pose.
	protocols::moves::MoverCOP ensemble_generating_protocol_;

	/// @brief Set the number of times the ensemble generating protocol is run.  Defaults to 1.
	core::Size ensemble_generating_protocol_repeats_ = 1;

	/// @brief Number of poses seen by this ensemble metric so far.
	core::Size poses_in_ensemble_ = 0;

#ifdef MULTI_THREADED
	/// @brief A mutex used when cloning the input pose for use by the ensemble generating protocol.
	/// @details Only used if the ensemble generating protocol is used.
	std::mutex pose_mutex_;

	/// @brief A mutex used when cloning the ensemble generating protocol.
	/// @details Only used if the ensemble generating protocol is used.
	std::mutex ensemble_generating_protocol_mutex_;

	/// @brief A mutex used when collecting data on the cloned pose, in a multi-threaded context.
	std::mutex ensemble_metric_mutex_;
#endif

	/// @brief Number of threads to request.  1 means request all available.
	core::Size n_threads_ = 1;

#ifdef    SERIALIZATION
public: //Serialization functions.
	template< class Archive > void save( Archive & arc ) const;
	template< class Archive > void load( Archive & arc );
#endif // SERIALIZATION

};

} //ensemble_metrics
} //protocols

#ifdef    SERIALIZATION
CEREAL_FORCE_DYNAMIC_INIT( protocols_ensemble_metrics_EnsembleMetric )
#endif // SERIALIZATION

#endif //INCLUDED_protocols_ensemble_metrics_EnsembleMetric_hh
