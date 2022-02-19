// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleFilter.hh), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/filters/EnsembleFilter.hh
/// @brief A filter that filters based on some named float-valued property measured by an EnsembleMetric.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

#ifndef INCLUDED_protocols_ensemble_metrics_filters_EnsembleFilter_hh
#define INCLUDED_protocols_ensemble_metrics_filters_EnsembleFilter_hh

// Unit headers
#include <protocols/ensemble_metrics/filters/EnsembleFilter.fwd.hh>
#include <protocols/filters/Filter.hh>

// Protocols headers
#include <protocols/ensemble_metrics/EnsembleMetric.fwd.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>

// Basic/Utility headers
#include <basic/datacache/DataMap.fwd.hh>
#include <basic/citation_manager/CitationCollectionBase.fwd.hh>

namespace protocols {
namespace ensemble_metrics {
namespace filters {

/// @brief The behaviour of this filter.
enum class EnsembleFilterAcceptanceMode {
	GREATER_THAN = 1, //Keep first
	LESS_THAN,
	GREATER_THAN_EQ,
	LESS_THAN_EQ,
	EQ,
	NOT_EQ, //Keep second-to-last
	N_MODES = NOT_EQ //Keep last.
};

/// @brief Given the filter's behaviour mode enum, get the corresponding string.
std::string
acceptance_mode_string_from_enum( EnsembleFilterAcceptanceMode const mode );

/// @brief Given the filter's behaviour mode string, get the corresponding enum.
EnsembleFilterAcceptanceMode
acceptance_mode_enum_from_string( std::string const & mode_name );

///@brief A filter that filters based on some named float-valued property measured by an EnsembleMetric.
class EnsembleFilter : public protocols::filters::Filter {

public:
	EnsembleFilter();

	// destructor (important for properly forward-declaring smart-pointer members)
	~EnsembleFilter() override;

	/// @brief returns true if the structure passes the filter, false otherwise
	bool
	apply( core::pose::Pose const & pose ) const override;

	/// @brief required for reporting score values
	core::Real
	report_sm( core::pose::Pose const & pose ) const override;

	/// @brief allows printing data to a stream
	void
	report( std::ostream & os, core::pose::Pose const & pose ) const override;

public:
	std::string
	name() const override;

	static
	std::string
	class_name();

	static
	void
	provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd );

	/// @brief parse XML tag (to use this Filter in Rosetta Scripts)
	void parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data ) override;

	/// @brief required in the context of the parser/scripting scheme
	protocols::filters::FilterOP
	fresh_instance() const override;

	/// @brief required in the context of the parser/scripting scheme
	protocols::filters::FilterOP
	clone() const override;

	/// @brief This filter is unpublished.  It returns Vikram K. Mulligan as its author.
	void provide_citation_info( basic::citation_manager::CitationCollectionList & ) const override;

	/// @brief Confirm that this filter has been properly configured prior to filtering with it.
	/// @details Throws if it has not.
	void validate_my_configuration() const;

public: //Setters

	/// @brief Sets the metric directly; does not clone.
	void
	set_ensemble_metric(
		protocols::ensemble_metrics::EnsembleMetricOP metric_in
	);

	/// @brief Set the name of the value produced by the EnsembleMetric and used for filtering.
	void
	set_named_value(
		std::string const & setting
	);

	/// @brief Set the cutoff threshold for filtering.
	void
	set_threshold(
		core::Real const setting
	);

	/// @brief Set the acceptance mode.
	void
	set_acceptance_mode(
		EnsembleFilterAcceptanceMode const setting
	);

	/// @brief Set the acceptance mode, by string.
	void
	set_acceptance_mode(
		std::string const & setting
	);

public: //Getters

	/// @brief Get the ensemble metric.
	/// @details Will be nullptr of not set.
	protocols::ensemble_metrics::EnsembleMetricOP
	ensemble_metric() const;

	/// @brief Get the name of the value produced by the EnsembleMetric and used for filtering.
	std::string const &
	named_value() const;

	/// @brief Get the cutoff threshold for filtering.
	core::Real
	threshold() const;

	/// @brief Get the acceptance mode.
	EnsembleFilterAcceptanceMode
	acceptance_mode() const;

private: //Functions

	/// @brief Given a value, determine if it's greater than, less than, or equal to the threshold.
	/// Return pass (true) or fail (false) based on the acceptance mode.
	bool value_passes( core::Real const value ) const;

private:

	/// @brief An ensemble metric that will be used for filtering.
	protocols::ensemble_metrics::EnsembleMetricOP ensemble_metric_;

	/// @brief The name of the value produced by the EnsembleMetric and used for filtering.
	std::string named_value_;

	/// @brief The cutoff threshold for filtering.
	core::Real threshold_ = 0.0;

	/// @brief Should we reject things over or under the threshold?
	EnsembleFilterAcceptanceMode acceptance_mode_ = EnsembleFilterAcceptanceMode::LESS_THAN_EQ;

};

} //filters
} //ensemble_metrics
} //protocols

#endif //INCLUDED_protocols_ensemble_metrics_filters_EnsembleFilter_hh
