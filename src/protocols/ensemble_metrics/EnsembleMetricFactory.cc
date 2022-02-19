// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetricFactory.cc), WHICH WAS MADE
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

/// @file   core//ensemble_metric/EnsembleMetricFactory.cc
/// @brief  Implementation of the class for instantiating arbitrary EnsembleMetrics
///         from a string --> EnsembleMetricCreator map
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

// Unit headers
#include <protocols/ensemble_metrics/EnsembleMetricFactory.hh>

// Package headers
#include <protocols/ensemble_metrics/EnsembleMetric.hh>
#include <protocols/ensemble_metrics/EnsembleMetricCreator.hh>
#include <protocols/ensemble_metrics/util.hh>

// Utility headers
#include <utility/excn/Exceptions.hh>
#include <utility/tag/Tag.hh>
#include <utility/tag/XMLSchemaGeneration.fwd.hh>
#include <utility/tag/xml_schema_group_initialization.hh>

// Basic headers
#include <basic/citation_manager/CitationManager.hh>

namespace protocols {
namespace ensemble_metrics {


EnsembleMetricFactory::EnsembleMetricFactory():
	utility::SingletonBase< EnsembleMetricFactory >(),
	creator_map_()
{}

void
EnsembleMetricFactory::factory_register(
	EnsembleMetricCreatorOP creator
) {
	if ( creator_map_.find( creator->keyname() ) != creator_map_.end() ) {
		utility_exit_with_message(  "Factory Name Conflict: Two or more EnsembleMetricCreators registered with the name " + creator->keyname() + "!" );
	}
	creator_map_[ creator->keyname() ] = creator;
}

bool
EnsembleMetricFactory::has_type( std::string const & selector_type ) const
{
	return creator_map_.find( selector_type ) != creator_map_.end();
}

EnsembleMetricOP
EnsembleMetricFactory::new_ensemble_metric(
	std::string const & ensemble_metric_name,
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap
) const {
	auto iter = creator_map_.find( ensemble_metric_name );
	runtime_assert_string_msg(
		iter != creator_map_.end(),
		"No EnsembleMetricCreator with the name '" + ensemble_metric_name + "' has been registered with the EnsembleMetricFactory!"
	);
	EnsembleMetricOP new_ensemble_metric = iter->second->create_ensemble_metric();
	new_ensemble_metric->parse_common_ensemble_metric_options( tag, datamap );
	new_ensemble_metric->parse_my_tag( tag, datamap );

	//Register with the CitationManager:
	basic::citation_manager::CitationCollectionList citations;
	new_ensemble_metric->provide_citation_info( citations );
	basic::citation_manager::CitationManager::get_instance()->add_citations( citations );

	return new_ensemble_metric;
}


/// @brief Get the XML schema for a given ensemble metric.
/// @details Throws an error if the residue selector is unknown to Rosetta.
/// @author Vikram K. Mulligan (vmullig@uw.edu)
void
EnsembleMetricFactory::provide_xml_schema(
	std::string const & metric_name,
	utility::tag::XMLSchemaDefinition & xsd
) const {
	auto iter = creator_map_.find( metric_name );
	runtime_assert_string_msg(
		iter != creator_map_.end(),
		"No EnsembleMetric with the name '" + metric_name + "' has been registered with the EnsembleMetricFactory!"
	);
	iter->second->provide_xml_schema( xsd );
}

void
EnsembleMetricFactory::define_ensemble_metric_xml_schema(
	utility::tag::XMLSchemaDefinition & xsd
) const {
	try{
		utility::tag::define_xml_schema_group(
			creator_map_,
			ensemble_metric_xml_schema_group_name(),
			& complex_type_name_for_ensemble_metric,
			xsd
		);
	} catch( utility::excn::Exception const & e ) {
		throw CREATE_EXCEPTION(utility::excn::Exception,  "Could not generate an XML Schema for EnsembleMetrics from EnsembleMetricsFactory; offending class"
			" must call protocols::ensemble_metric::complex_type_name_for_ensemble_metric when defining"
			" its XML Schema.\n" + e.msg() );
	}
}

std::string
EnsembleMetricFactory::ensemble_metric_xml_schema_group_name() {
	return "ensemble_metric";
}

/// @brief Get a human-readable listing of the citations for a given ensemble metric, by metric name.
/// @details Returns an empty string if there are no citations.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org).
std::string
EnsembleMetricFactory::get_citation_humanreadable(
	std::string const & metric_name
) const {
	using namespace basic::citation_manager;
	CitationCollectionList citations;
	auto const iter = creator_map_.find( metric_name );
	runtime_assert_string_msg(
		iter != creator_map_.end(),
		"Error in EnsembleMetricFactory::get_citation_humanreadable(): Could not find ensemble metric \"" + metric_name + "\"!"
	);
	EnsembleMetricOP new_ensemble_metric = iter->second->create_ensemble_metric();
	runtime_assert_string_msg(
		new_ensemble_metric != nullptr,
		"Error in EnsembleMetricFactory::get_citation_humanreadable(): Could not instantiate " + metric_name + " ensemble metric!"
	);
	new_ensemble_metric->provide_citation_info(citations);
	if ( citations.empty() ) return "";
	std::ostringstream ss;
	ss << "References and author information for the " << metric_name << " ensemble metric:" << std::endl;
	ss << std::endl;
	basic::citation_manager::CitationManager::get_instance()->write_all_citations_and_unpublished_author_info_from_list_to_stream( citations, ss );
	return ss.str();
}

} //namespace ensemble_metrics
} //namespace protocols
