// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (CentralTendencyEnsembleMetric.fwd.hh), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetric.fwd.hh
/// @brief An ensemble metric that takes a real-valued simple metric, applies it to all poses in an ensemble,
/// and calculates measures of central tendency (mean, median, mode) and other statistics about the distribution
/// (standard deviation, standard error of the mean, min, max, range, etc.).
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

#ifndef INCLUDED_protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric_fwd_hh
#define INCLUDED_protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>


// Forward
namespace protocols {
namespace ensemble_metrics {
namespace metrics {

class CentralTendencyEnsembleMetric;

using CentralTendencyEnsembleMetricOP = utility::pointer::shared_ptr< CentralTendencyEnsembleMetric >;
using CentralTendencyEnsembleMetricCOP = utility::pointer::shared_ptr< CentralTendencyEnsembleMetric const >;

} //metrics
} //ensemble_metrics
} //protocols

#endif //INCLUDED_protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric_fwd_hh
