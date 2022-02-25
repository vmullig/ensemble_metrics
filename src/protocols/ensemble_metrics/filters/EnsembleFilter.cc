// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleFilter.cc), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/filters/EnsembleFilter.cc
/// @brief A filter that filters based on some named float-valued property measured by an EnsembleMetric.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

// Project headers
#include <protocols/ensemble_metrics/filters/EnsembleFilter.hh>
#include <protocols/ensemble_metrics/filters/EnsembleFilterCreator.hh>

// Protocols headers
#include <protocols/filters/filter_schemas.hh>
#include <protocols/ensemble_metrics/EnsembleMetric.hh>
#include <protocols/ensemble_metrics/util.hh>

// Basic headers
#include <basic/Tracer.hh>
#include <basic/citation_manager/UnpublishedModuleInfo.hh>
#include <basic/citation_manager/CitationCollection.hh>

// Utility headers
#include <utility/tag/Tag.hh>
#include <utility/tag/XMLSchemaGeneration.hh>
#include <utility/pointer/memory.hh>




static basic::Tracer TR( "protocols.ensemble_metrics.filters.EnsembleFilter" );

namespace protocols {
namespace ensemble_metrics {
namespace filters {

/// @brief Given the filter's behaviour mode enum, get the corresponding string.
std::string
acceptance_mode_string_from_enum(
	EnsembleFilterAcceptanceMode const mode
) {
	switch( mode ) {
	case EnsembleFilterAcceptanceMode::GREATER_THAN :
		return "greater_than";
	case EnsembleFilterAcceptanceMode::LESS_THAN :
		return "less_than";
	case EnsembleFilterAcceptanceMode::GREATER_THAN_EQ :
		return "greater_than_or_equal";
	case EnsembleFilterAcceptanceMode::LESS_THAN_EQ :
		return "less_than_or_equal";
	case EnsembleFilterAcceptanceMode::EQ :
		return "equal";
	case EnsembleFilterAcceptanceMode::NOT_EQ :
		return "not_equal";
	default :
		utility_exit_with_message( "Program error.  This should not happen." );
	}
	return ""; //Keep older compilers happy.  Never reached.
}

/// @brief Given the filter's behaviour mode string, get the corresponding enum.
EnsembleFilterAcceptanceMode
acceptance_mode_enum_from_string(
	std::string const & mode_name
) {
	for ( core::Size i(1); i <= static_cast<core::Size>( EnsembleFilterAcceptanceMode::N_MODES ); ++i ) {
		if ( mode_name == acceptance_mode_string_from_enum( static_cast<EnsembleFilterAcceptanceMode>(i) ) ) {
			return static_cast<EnsembleFilterAcceptanceMode>(i);
		}
	}

	utility_exit_with_message( "Error in protocols::ensemble_metrics::filters::acceptance_mode_enum_from_string(): "
		"The string \"" + mode_name + "\" could not be parsed as a filter mode.  Allowed modes are: \"greater_than\", "
		"\"less_than\", \"greater_than_or_equal\", \"less_than_or_equal\", \"equal\", and \"not_equal\"."
	);

	return EnsembleFilterAcceptanceMode::GREATER_THAN; //Keep compiler happy.  Never reached.
}

EnsembleFilter::EnsembleFilter():
	protocols::filters::Filter( "EnsembleFilter" )
{}

EnsembleFilter::~EnsembleFilter()
{}

void
EnsembleFilter::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & data
) {
	try {
		set_ensemble_metric( protocols::ensemble_metrics::get_metric_from_datamap_and_subtags( tag, data, "ensemble_metric" ) );
	} catch( utility::excn::Exception & excn ) {
		utility_exit_with_message(
			"The EnsembleFilter requires that an EnsembleMetric be provided with the \"ensemble_metric\" option.  Error message follows:\n"
			+ excn.msg()
		);
	}
	try {
		set_named_value( tag->getOption< std::string >("named_value") );
	} catch( utility::excn::Exception & excn ) {
		utility_exit_with_message(
			"The EnsembleFilter requires that a floating-point value produced by the EnsembleMetric be specified with "
			"the \"named_value\" option.  Error message follows:\n"
			+ excn.msg()
		);
	}
	if ( tag->hasOption("filter_acceptance_mode") ) {
		set_acceptance_mode( tag->getOption<std::string>( "filter_acceptance_mode" ) );
	}
	if ( tag->hasOption("threshold") ) {
		set_threshold( tag->getOption<core::Real>("threshold") );
	}
}

protocols::filters::FilterOP
EnsembleFilter::clone() const
{
	return utility::pointer::make_shared< EnsembleFilter >( *this );
}

/// @brief This filter is unpublished.  It returns Vikram K. Mulligan as its author.
void
EnsembleFilter::provide_citation_info( basic::citation_manager::CitationCollectionList & citations ) const {
	citations.add(
		utility::pointer::make_shared< basic::citation_manager::UnpublishedModuleInfo >(
		"EnsembleFilter", basic::citation_manager::CitedModuleType::Filter,
		"Vikram K. Mulligan",
		"Systems Biology Group, Center for Computational Biology, Flatiron Institute.",
		"vmulligan@flatironinstitute.org",
		"Wrote the EnsembleFilter."
		)
	);
}

/// @brief Confirm that this filter has been properly configured prior to filtering with it.
/// @details Throws if it has not.
void
EnsembleFilter::validate_my_configuration() const {
	std::string const errmsg( "Error in EnsembleFilter::validate_my_configuration(): ");
	runtime_assert_string_msg( ensemble_metric_ != nullptr, errmsg + "An ensemble metric must be provided to the EnsembleFilter before using it!" );
	runtime_assert_string_msg( !named_value_.empty(), errmsg + "The name of a floating-point value returned by the " + ensemble_metric_->name() + " must be provided before using the EnsembleMetric." );
	runtime_assert_string_msg( ensemble_metric_->real_valued_metric_names().has_value( named_value_ ), errmsg +
		"The EnsembleMetric was configured to filter based on a floating-point value named \"" + named_value_ +
		"\" returned by the " + ensemble_metric_->name() + " EnsembleMetric, but this EnsembleMetric " +
		"returns no such value!"
	);
	if ( !ensemble_metric_->finalized() ) {
		ensemble_metric_->produce_final_report();
	}
}

/// @brief Sets the metric directly; does not clone.
void
EnsembleFilter::set_ensemble_metric(
	protocols::ensemble_metrics::EnsembleMetricOP metric_in
) {
	ensemble_metric_ = metric_in;
}

/// @brief Set the name of the value produced by the EnsembleMetric and used for filtering.
void
EnsembleFilter::set_named_value(
	std::string const & setting
) {
	named_value_ = setting;
}

/// @brief Set the cutoff threshold for filtering.
void
EnsembleFilter::set_threshold(
	core::Real const setting
) {
	threshold_ = setting;
}

/// @brief Set the acceptance mode.
void
EnsembleFilter::set_acceptance_mode(
	EnsembleFilterAcceptanceMode const setting
) {
	acceptance_mode_ = setting;
}

/// @brief Set the acceptance mode, by string.
void
EnsembleFilter::set_acceptance_mode(
	std::string const & setting
) {
	set_acceptance_mode( acceptance_mode_enum_from_string( setting ) );
}

/// @brief Get the ensemble metric.
/// @details Will be nullptr of not set.
protocols::ensemble_metrics::EnsembleMetricOP
EnsembleFilter::ensemble_metric() const {
	return ensemble_metric_;
}

/// @brief Get the name of the value produced by the EnsembleMetric and used for filtering.
std::string const &
EnsembleFilter::named_value() const {
	return named_value_;
}

/// @brief Get the cutoff threshold for filtering.
core::Real
EnsembleFilter::threshold() const {
	return threshold_;
}

/// @brief Get the acceptance mode.
EnsembleFilterAcceptanceMode
EnsembleFilter::acceptance_mode() const {
	return acceptance_mode_;
}

/// @brief Given a value, determine if it's greater than, less than, or equal to the threshold.
/// Return pass (true) or fail (false) based on the acceptance mode.
bool
EnsembleFilter::value_passes(
	core::Real const value
) const {
	switch( acceptance_mode_ ) {
	case EnsembleFilterAcceptanceMode::GREATER_THAN :
		return (value > threshold_);
	case EnsembleFilterAcceptanceMode::LESS_THAN :
		return (value < threshold_);
	case EnsembleFilterAcceptanceMode::GREATER_THAN_EQ :
		return (value >= threshold_);
	case EnsembleFilterAcceptanceMode::LESS_THAN_EQ :
		return (value <= threshold_);
	case EnsembleFilterAcceptanceMode::EQ :
		return (value == threshold_);
	case EnsembleFilterAcceptanceMode::NOT_EQ :
		return (value != threshold_);
	}
	return false; //Should never reach here.
}


protocols::filters::FilterOP
EnsembleFilter::fresh_instance() const
{
	return utility::pointer::make_shared< EnsembleFilter >();
}

bool
EnsembleFilter::apply(
	core::pose::Pose const &
) const {
	validate_my_configuration();
	core::Real const val( ensemble_metric_->get_real_metric_value_by_name( named_value_ ) );
	bool const passfail( value_passes(val) );
	TR << "EnsembleMetric " << ensemble_metric_->name() << " reports " << named_value_ << " = " << val;
	TR << ".  This " << (passfail ? "PASSES" : "FAILS") << " this filter." << std::endl;
	return passfail;
}

core::Real
EnsembleFilter::report_sm( core::pose::Pose const & ) const
{
	validate_my_configuration();
	return ensemble_metric_->get_real_metric_value_by_name( named_value_ );
}

void
EnsembleFilter::report( std::ostream & os, core::pose::Pose const & ) const
{
	validate_my_configuration();
	core::Real const val( ensemble_metric_->get_real_metric_value_by_name( named_value_ ) );
	os << "EnsembleMetric " << ensemble_metric_->name() << " reports " << named_value_ << " = " << val;
	os << ".  This " << (value_passes(val) ? "PASSES" : "FAILS") << " this filter." << std::endl;
}

std::string EnsembleFilter::name() const {
	return class_name();
}

std::string EnsembleFilter::class_name() {
	return "EnsembleFilter";
}

void EnsembleFilter::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{
	using namespace utility::tag;
	AttributeList attlist;

	attlist
		+ utility::tag::XMLSchemaAttribute::required_attribute(
		"ensemble_metric", xs_string, "A previously-defined EnsembleMetric that produces at least one "
		"floating-point value.  This filter will filter a pose based on that value."
		)
		+ utility::tag::XMLSchemaAttribute::required_attribute(
		"named_value", xs_string, "A named floating-point value produced by the EnsembleMetric, on which "
		"this filter will filter."
		)
		+ utility::tag::XMLSchemaAttribute::attribute_w_default(
		"threshold", xsct_real, "The threshold for rejecting a pose.", "0.0"
		)
		+ utility::tag::XMLSchemaAttribute::attribute_w_default(
		"filter_acceptance_mode", xs_string, "The criterion for ACCEPTING a pose.  For instance, if the value "
		"returned by the ensemble metric is greater than the threshold, and the mode is 'less_than_or_equal' (the "
		"default mode), then the pose is rejected.  Allowed modes are: 'greater_than', "
		"'less_than', 'greater_than_or_equal', 'less_than_or_equal', 'equal', and 'not_equal'.",
		"less_than_or_equal"
	);

	protocols::filters::xsd_type_definition_w_attributes(
		xsd,
		class_name(),
		"A filter that filters based on some named float-valued property measured by an EnsembleMetric.  "
		"Note that the value produced by the EnsembleMetric is based on an ensemble generated earlier in "
		"the protocol, presumably from the pose on which we are currently filtering.",
		attlist
	);
}

/////////////// Creator ///////////////

protocols::filters::FilterOP
EnsembleFilterCreator::create_filter() const
{
	return utility::pointer::make_shared< EnsembleFilter >( );
}

std::string
EnsembleFilterCreator::keyname() const
{
	return EnsembleFilter::class_name();
}

void EnsembleFilterCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	EnsembleFilter::provide_xml_schema( xsd );
}

} //filters
} //ensemble_metrics
} //protocols
