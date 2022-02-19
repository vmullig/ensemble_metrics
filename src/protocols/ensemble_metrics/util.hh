// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (util.hh), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/util.hh
/// @brief Util files for EnsembleMetrics.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
/// @note Shamelessly copied and modified from Jared Adolf-Bryfogle's simple metric code (jadolfbr@gmail.com).

#ifndef INCLUDED_protocols_ensemble_metrics_util_hh
#define INCLUDED_protocols_ensemble_metrics_util_hh

#include <protocols/ensemble_metrics/EnsembleMetric.fwd.hh>

#include <core/pose/Pose.fwd.hh>

// Basic headers
#include <basic/datacache/DataMap.fwd.hh>
#include <utility/tag/XMLSchemaGeneration.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <utility/vector1.hh>

//C++ headers

namespace protocols {
namespace ensemble_metrics {

/// @brief Get the naming system for ensemble metrics.
std::string
complex_type_name_for_ensemble_metric( std::string const & metric_name );

/// @brief Generate the ComplexTypeGenerator from the EnsembleMetric base class.
///  Add any additional schema options from sub-derived classes
void
xsd_ensemble_metric_type_definition_w_attributes(
	utility::tag::XMLSchemaDefinition & xsd,
	std::string const & ensemble_metric_name,
	std::string const & description,
	utility::tag::AttributeList const & attributes);

void
xsd_ensemble_metric_type_definition_w_attributes_and_repeatable_subelements(
	utility::tag::XMLSchemaDefinition & xsd,
	std::string const & ensemble_metric_name,
	std::string const & description,
	utility::tag::AttributeList const & attributes,
	utility::tag::XMLSchemaSimpleSubelementList const & subelements
);

utility::vector1< EnsembleMetricOP >
get_metrics_from_datamap_and_subtags(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap,
	std::string tag_name="ensemble_metrics"
);

EnsembleMetricOP
get_metric_from_datamap_and_subtags(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & datamap,
	std::string tag_name="ensemble_metric"
);

/// @brief Get an informative error message if the SM data already exists and is not overriden.
void
throw_sm_override_error(
	std::string const & out_tag,
	std::string const & metric_name
);

} //core
} //ensemble_metrics


#endif //protocols/ensemble_metrics_util_hh

