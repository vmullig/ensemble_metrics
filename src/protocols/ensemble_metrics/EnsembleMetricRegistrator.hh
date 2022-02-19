// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetricRegistrator.hh), WHICH WAS MADE
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

/// @file   protocols/analysis/ensemble_metrics/EnsembleMetricRegistrator.hh
/// @brief  Declaration of the template class for registrating EnsembleMetricCreators with
///         the EnsembleMetricFactory
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
/// @note Shamelessly copied from Jared Adolf-Bryfogle's simple metric code (jadolfbr@gmail.com).

#ifndef INCLUDED_protocols_ensemble_metrics_EnsembleMetricRegistrator_hh
#define INCLUDED_protocols_ensemble_metrics_EnsembleMetricRegistrator_hh

// Package headers
#include <protocols/ensemble_metrics/EnsembleMetricFactory.fwd.hh>
#include <utility/factory/WidgetRegistrator.hh>

namespace protocols {
namespace ensemble_metrics {


/// @brief This templated class will register an instance of an
/// EnsembleMetricCreator (class T) with the EnsembleMetricFactory.  It will ensure
/// that no EnsembleMetric creator is registered twice, and, centralizes
/// this registration logic so that thread safety issues can be handled in
/// one place
template < class T >
class EnsembleMetricRegistrator : public utility::factory::WidgetRegistrator< EnsembleMetricFactory, T >
{
public:
	typedef utility::factory::WidgetRegistrator< EnsembleMetricFactory, T > parent;
public:
	EnsembleMetricRegistrator() : parent() {}
};

}
}

#endif
