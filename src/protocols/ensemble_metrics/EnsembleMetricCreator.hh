// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetricCreator.hh), WHICH WAS MADE
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

/// @file   protocols/ensemble_metrics/EnsembleMetricCreator.hh
/// @brief  Class for instantiating a particular EnsembleMetric.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

#ifndef INCLUDED_protocols_ensemble_metrics_EnsembleMetricCreator_HH
#define INCLUDED_protocols_ensemble_metrics_EnsembleMetricCreator_HH

// Package headers
#include <protocols/ensemble_metrics/EnsembleMetric.fwd.hh>

// Utility headers
#include <utility/VirtualBase.hh>
#include <utility/tag/XMLSchemaGeneration.fwd.hh>

// C++ headers
#include <string>

namespace protocols {
namespace ensemble_metrics {

/// @brief  Class for instantiating a particular EnsembleMetric.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
class EnsembleMetricCreator : public utility::VirtualBase {
public:
	/// @brief Instantiate a particular EnsembleMetric
	virtual EnsembleMetricOP
	create_ensemble_metric() const = 0;

	/// @brief Return a string that will be used to instantiate the particular EnsembleMetric
	virtual std::string
	keyname() const = 0;

	virtual void
	provide_xml_schema( utility::tag::XMLSchemaDefinition &) const = 0;
};


} //namespace ensemble_metrics
} //namespace protocols


#endif
