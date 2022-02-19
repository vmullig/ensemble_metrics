// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetricLoader.cc), WHICH WAS MADE
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

/// @file   protocols/parser/EnsembleMetricLoader.cc
/// @brief  Implementation of the XML parser's DataLoader base class (ctor & dstor) for EnsembleMetrics.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

// Unit Headers
#include <protocols/parser/EnsembleMetricLoader.hh>
#include <protocols/parser/EnsembleMetricLoaderCreator.hh>

// Project headers
#include <protocols/ensemble_metrics/EnsembleMetric.hh>
#include <protocols/ensemble_metrics/EnsembleMetricFactory.hh>

// Basic headers
#include <basic/Tracer.hh>
#include <basic/datacache/DataMap.hh>

// Utility headers
#include <utility/tag/Tag.hh>
#include <utility/tag/XMLSchemaGeneration.hh>
#include <utility/vector0.hh>

#include <core/types.hh> // AUTO IWYU For Size

namespace protocols {
namespace parser {



static basic::Tracer TR( "protocols.parser.EnsembleMetricLoader" );


using namespace protocols::ensemble_metrics;


EnsembleMetricLoader::EnsembleMetricLoader() = default;
EnsembleMetricLoader::~EnsembleMetricLoader() = default;

void EnsembleMetricLoader::load_data(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap
) const
{
	using namespace utility::tag;
	using protocols::ensemble_metrics::EnsembleMetricOP;
	using TagCOPs = utility::vector0<TagCOP>;

	TagCOPs const & selector_tags( tag->getTags() );
	for ( core::Size ii = 0; ii < selector_tags.size(); ++ii ) {
		TagCOP ii_tag = selector_tags[ ii ];
		EnsembleMetricOP metric = protocols::ensemble_metrics::EnsembleMetricFactory::get_instance()->new_ensemble_metric(
			ii_tag->getName(),
			ii_tag,
			datamap
		);

		// If "name" is specified, add it to the data map under that name. Otherwise use the type name.
		std::string const name_to_use( ii_tag->getOption( "name", ii_tag->getName() ) );

		bool const data_add_status(
			datamap.add( "EnsembleMetric", name_to_use, metric )
		);

		if ( !data_add_status ) {
			utility_exit_with_message( "EnsembleMetric \"" + name_to_use + "\" already exists in the basic::datacache::DataMap. Please rename." );
		}

	}
	TR.flush();
}

std::string
EnsembleMetricLoader::loader_name() { return "ENSEMBLE_METRICS"; }

std::string
EnsembleMetricLoader::ensemble_metric_loader_ct_namer( std::string const & element_name )
{
	return "ensemble_metric_loader_" + element_name + "_type";
}

void EnsembleMetricLoader::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd )
{
	using namespace utility::tag;

	EnsembleMetricFactory::get_instance()->define_ensemble_metric_xml_schema( xsd );

	XMLSchemaSimpleSubelementList rs_loader_subelements;
	rs_loader_subelements.add_group_subelement( & EnsembleMetricFactory::ensemble_metric_xml_schema_group_name );

	XMLSchemaComplexTypeGenerator rs_ct;
	rs_ct.element_name( loader_name() ).complex_type_naming_func( & ensemble_metric_loader_ct_namer )
		.description( "EnsembleMetrics may be defined as subelements of the " + loader_name() + " element, and then will be placed into the DataMap"
		" for later retrieval by Movers and Filters or anything else that might use a EnsembleMetric. All immediate subelements should have the 'name' attribute"
		" as that is how they will be identified in the DataMap." )
		.set_subelements_repeatable( rs_loader_subelements )
		.write_complex_type_to_schema( xsd );

}


DataLoaderOP
EnsembleMetricLoaderCreator::create_loader() const {
	return utility::pointer::make_shared< EnsembleMetricLoader >();
}

std::string
EnsembleMetricLoaderCreator::keyname() const {
	return EnsembleMetricLoader::loader_name();
}

EnsembleMetricLoaderCreator::DerivedNameFunction
EnsembleMetricLoaderCreator::schema_ct_naming_function() const
{
	return & EnsembleMetricLoader::ensemble_metric_loader_ct_namer;
}

void
EnsembleMetricLoaderCreator::provide_xml_schema( utility::tag::XMLSchemaDefinition & xsd ) const
{
	EnsembleMetricLoader::provide_xml_schema( xsd );
}



} //namespace parser
} //namespace protocols
