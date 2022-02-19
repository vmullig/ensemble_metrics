// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (util.cc), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/util.cc
/// @brief Util files for EnsembleMetrics.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
/// @note Shamelessly copied and modified from Jared Adolf-Bryfogle's simple metric code (jadolfbr@gmail.com).

#include <protocols/ensemble_metrics/util.hh>

#include <utility/tag/XMLSchemaGeneration.hh>
#include <basic/Tracer.hh>

#include <protocols/ensemble_metrics/EnsembleMetric.hh>
#include <protocols/ensemble_metrics/EnsembleMetricFactory.hh>

#include <utility/excn/Exceptions.hh>
#include <utility/vector1.hh>
#include <utility/tag/Tag.hh>
#include <utility/tag/XMLSchemaGeneration.fwd.hh>
#include <utility/string_util.hh>
#include <basic/datacache/DataMap.hh>
#include <basic/datacache/BasicDataCache.hh>

static basic::Tracer TR( "protocols.ensemble_metrics.util" );


namespace protocols {
namespace ensemble_metrics {

std::string
complex_type_name_for_ensemble_metric( std::string const & ensemble_metric_name){
	return "ensemble_metric_" + ensemble_metric_name + "_type";
}

void
xsd_ensemble_metric_type_definition_w_attributes(
	utility::tag::XMLSchemaDefinition & xsd,
	std::string const & rs_type,
	std::string const & description,
	utility::tag::AttributeList const & attributes
)
{
	utility::tag::XMLSchemaComplexTypeGeneratorOP ct_gen = EnsembleMetric::complex_type_generator_for_ensemble_metric(xsd);

	ct_gen->complex_type_naming_func( & complex_type_name_for_ensemble_metric )
		.element_name( rs_type )
		.description( description )
		.add_attributes( attributes )
		.add_optional_name_attribute()
		.write_complex_type_to_schema( xsd );
}

void
xsd_ensemble_metric_type_definition_w_attributes_and_repeatable_subelements(
	utility::tag::XMLSchemaDefinition & xsd,
	std::string const & rs_type,
	std::string const & description,
	utility::tag::AttributeList const & attributes,
	utility::tag::XMLSchemaSimpleSubelementList const & subelements
)
{
	utility::tag::XMLSchemaComplexTypeGeneratorOP ct_gen = EnsembleMetric::complex_type_generator_for_ensemble_metric(xsd);

	ct_gen->complex_type_naming_func( & complex_type_name_for_ensemble_metric )
		.element_name( rs_type )
		.description( description )
		.add_attributes( attributes )
		.set_subelements_repeatable( subelements )
		.add_optional_name_attribute()
		.write_complex_type_to_schema( xsd );
}

utility::vector1< EnsembleMetricOP >
get_metrics_from_datamap_and_subtags(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap,
	std::string tag_name)
{
	utility::vector1< EnsembleMetricOP > metrics;

	if ( tag->hasOption(tag_name ) ) {
		std::string const cst_gen_cslist( tag->getOption< std::string >( tag_name ) );
		utility::vector1< std::string > const cst_gen_vector( utility::string_split( cst_gen_cslist, ',' ) );
		for ( std::string const & gen: cst_gen_vector ) {
			//Retrieve from the data map
			if ( datamap.has( "EnsembleMetric", gen ) ) {
				EnsembleMetricOP metric( datamap.get_ptr< EnsembleMetric >( "EnsembleMetric", gen ) );
				metrics.push_back( metric );
				TR << "Added ensemble metric " << metric->name() << "." << std::endl;
			} else {
				throw CREATE_EXCEPTION(utility::excn::RosettaScriptsOptionError, "EnsembleMetric :" + gen + ": not found in basic::datacache::DataMap.");
			}
		}
	}
	//Note that this mover is a little unusual in that it adds any constraint generators defined as subtags to the DataMap.
	for ( auto subtag=tag->getTags().begin(); subtag!=tag->getTags().end(); ++subtag ) {
		EnsembleMetricOP metric = EnsembleMetricFactory::get_instance()->new_ensemble_metric( (*subtag)->getName(), *subtag, datamap );
		metrics.push_back( metric );
		TR << "Added ensemble metric " << metric->name() << "." << std::endl;
	}

	TR << "Parsed " << metrics.size() << " ensemble metrics." << std::endl;

	return metrics;
}

EnsembleMetricOP
get_metric_from_datamap_and_subtags(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap,
	std::string tag_name
) {
	if ( tag->hasOption(tag_name ) ) {
		std::string gen = tag->getOption< std::string >( tag_name );
		if ( datamap.has( "EnsembleMetric", gen ) ) {
			EnsembleMetricOP metric( datamap.get_ptr< EnsembleMetric >( "EnsembleMetric", gen ) );
			return metric;
		} else {
			throw CREATE_EXCEPTION(utility::excn::RosettaScriptsOptionError, "EnsembleMetric " + gen + " not found in basic::datacache::DataMap.");
		}

	}

	//If multiple metrics are passed in, we fail.
	utility::vector1< EnsembleMetricOP > metrics;
	for ( auto subtag=tag->getTags().begin(); subtag!=tag->getTags().end(); ++subtag ) {
		EnsembleMetricOP metric = EnsembleMetricFactory::get_instance()->new_ensemble_metric( (*subtag)->getName(), *subtag, datamap );
		metrics.push_back( metric );
	}

	if ( metrics.size() > 1 ) {
		TR.Error << "Too many EnsembleMetrics in tag:\n\t" << *tag << std::endl;
		throw CREATE_EXCEPTION(utility::excn::RosettaScriptsOptionError, "This class only accepts a single EnsembleMetric as a subtag.");
	} else if ( metrics.empty() ) {
		TR.Error << "EnsembleMetric not found in tag:\t\n" << *tag << std::endl;
		throw CREATE_EXCEPTION(utility::excn::RosettaScriptsOptionError, "No suitable EnsembleMetric found in entry.");
	} else {
		return metrics[1];
	}
}

void
throw_sm_override_error( std::string const & out_tag, std::string const & metric_name){
	std::string const msg = "\n\nEnsembleMetric error! \n The data of type "+ metric_name+ " with data output tag " + out_tag + " already exists! \n"
		"Please use the prefix/suffix settings or set a custom_type for the metric.\n  See the documentation for more:\n"
		"  https://www.rosettacommons.org/docs/latest/scripting_documentation/RosettaScripts/EnsembleMetrics/EnsembleMetrics#effective-use-of-ensemblemetrics.\n"
		" Note: If this was intentional, please set the override option to true in RunEnsembleMetricsMover\n\n";

	throw CREATE_EXCEPTION(utility::excn::Exception,  msg);
}

} //core
} //ensemble_metrics


