// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetricFactory.hh), WHICH WAS MADE
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

/// @file   protocols/ensemble_metrics/EnsembleMetricFactory.hh
/// @brief  Class for instantiating arbitrary EnsembleMetrics from a string --> EnsembleMetricCreator map.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

#ifndef INCLUDED_protocols_ensemble_metrics_EnsembleMetricFactory_HH
#define INCLUDED_protocols_ensemble_metrics_EnsembleMetricFactory_HH

#include <protocols/ensemble_metrics/EnsembleMetricFactory.fwd.hh>

// Unit headers
#include <utility/SingletonBase.hh>
#include <utility/tag/XMLSchemaGeneration.fwd.hh>
// Package headers
#include <protocols/ensemble_metrics/EnsembleMetric.fwd.hh>
#include <protocols/ensemble_metrics/EnsembleMetricCreator.fwd.hh>

// Basic headers
#include <basic/datacache/DataMap.fwd.hh>

// Utility headers
#include <utility/tag/Tag.fwd.hh>

// C++ headers
#include <map>
#include <string>

namespace protocols {
namespace ensemble_metrics {

/// @brief  Class for instantiating arbitrary EnsembleMetrics from a string --> EnsembleMetricCreator map.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
class EnsembleMetricFactory : public utility::SingletonBase< EnsembleMetricFactory > {
private:
	typedef std::map< std::string, EnsembleMetricCreatorOP > EnsembleMetricCreatorMap;

public:
	EnsembleMetricFactory();

	void factory_register( EnsembleMetricCreatorOP creator );

	bool has_type( std::string const & ensemble_metric_name ) const;

	EnsembleMetricOP new_ensemble_metric(
		std::string const & constraint_generator_name,
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & datamap
	) const;

	/// @brief Get the XML schema for a given residue selector.
	/// @details Throws an error if the residue selector is unknown to Rosetta.
	void
	provide_xml_schema(
		std::string const &selector_name,
		utility::tag::XMLSchemaDefinition & xsd
	) const;

	void
	define_ensemble_metric_xml_schema(
		utility::tag::XMLSchemaDefinition & xsd
	) const;

	static
	std::string
	ensemble_metric_xml_schema_group_name();

	/// @brief Get a human-readable listing of the citations for a given ensemble metric, by metric name.
	/// @details Returns an empty string if there are no citations.
	std::string
	get_citation_humanreadable(
		std::string const & metric_name
	) const;

private:

	EnsembleMetricCreatorMap creator_map_;

};

} //namespace ensemble_metrics
} //namespace protocols


#endif
