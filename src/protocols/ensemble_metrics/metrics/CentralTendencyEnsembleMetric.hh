// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (CentralTendencyEnsembleMetric.hh), WHICH WAS MADE
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

/// @file protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetric.hh
/// @brief An ensemble metric that takes a real-valued simple metric, applies it to all poses in an ensemble,
/// and calculates measures of central tendency (mean, median, mode) and other statistics about the distribution
/// (standard deviation, standard error of the mean, min, max, range, etc.).
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)

#ifndef INCLUDED_protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric_HH
#define INCLUDED_protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric_HH

// Unit headers
#include <protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetric.fwd.hh>
#include <protocols/ensemble_metrics/EnsembleMetric.hh>

// Utility headers
#include <utility/vector1.hh>

// Core headers
#include <core/simple_metrics/RealMetric.fwd.hh>
#include <core/types.hh>

namespace protocols {
namespace ensemble_metrics {
namespace metrics {

/// @brief An ensemble metric that takes a real-valued simple metric, applies it to all poses in an ensemble,
/// and calculates measures of central tendency (mean, median, mode) and other statistics about the distribution
/// (standard deviation, standard error of the mean, min, max, range, etc.).
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)
class CentralTendencyEnsembleMetric : public protocols::ensemble_metrics::EnsembleMetric {

public:

	/// @brief Default constructor.
	CentralTendencyEnsembleMetric();

	/// @brief Copy constructor.
	CentralTendencyEnsembleMetric( CentralTendencyEnsembleMetric const & );

	/// @brief Destructor.
	/// @note On destruction, an ensemble metric that has not yet reported does its final report.  This
	/// behaviour must be implemented by derived classes due to order of calls to destructors.
	~CentralTendencyEnsembleMetric() override;

	/// @brief Clone operation: make a copy of this object, and return an owning pointer to the copy.
	protocols::ensemble_metrics::EnsembleMetricOP
	clone() const override;

public: // Virtual functions overrides of public pure virtual functions from base class.

	/// @brief Provide the name of this EnsmebleMetric.
	/// @details Must be implemented by derived classes.
	std::string
	name() const override;

	/// @brief Name of the class for creator.
	static
	std::string
	name_static();

	/// @brief Get a list of the names of the real-valued metrics that can be filtered on (e.g. by the EnsembleMetricFilter)
	/// or otherwise extracted from this EnsembleMetric.  Must be implemented by derived classes.  (Can be empty list if
	/// no real-valued metrics are computed.)
	utility::vector1< std::string > const &
	real_valued_metric_names() const override;

private: // Virtual functions overrides of private pure virtual functions from base class.

	/// @brief Write the final report produced by this metric to a string.
	/// @details Must be implemented by derived classes.
	/// @note Output should not be terminated in a newline.
	std::string
	produce_final_report_string() override;

	/// @brief Add another pose to the ensemble seen so far.  Nonconst to allow data to be
	/// accumulated.
	/// @details Must be implemented by derived classes.
	void
	add_pose_to_ensemble(
		core::pose::Pose const & pose
	) override;

	/// @brief Given a metric name, get its value.
	/// @details Must be implemented by derived classes.
	core::Real
	derived_get_metric_by_name(
		std::string const & metric_name
	) const override;

	/// @brief Get the tracer for a derived class.
	/// @details Must be implemented for each derived class.
	basic::Tracer &
	get_derived_tracer() const override;

	/// @brief Reset the data collected by the derived classes.  Must be
	/// implemented by derived classes.
	void
	derived_reset() override;

public: // RosettaScripts functions

	/// @brief Parse XML setup.
	/// @details Must be implemented for each derived class.
	void
	parse_my_tag(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data
	) override;

	/// @brief Provide a machine-readable description (XSD) of the XML interface
	/// for this ensemble metric.
	/// @details Must be implemented for each derived class.
	static
	void
	provide_xml_schema(
		utility::tag::XMLSchemaDefinition & xsd
	);

public: // Citation manager functions

	/// @brief Provide citations to the passed CitationCollectionList
	/// Subclasses should add the info for themselves and any other classes they use.
	/// @details The default implementation of this function does nothing.  It ought to be
	/// overriden by ensemble metrics so that they can provide citation information or
	/// unpublished author information.
	void
	provide_citation_info(
		basic::citation_manager::CitationCollectionList & citations
	) const override;

private: // Private functions for this subclass.

	/// @brief At the end of accumulation and start of reporting, finalize the values.
	void finalize_values();

public: // Public functions for this subclass.

	/// @brief Set the real-valued metric that this ensemble metric will use.
	/// @details Used directly; not cloned.
	void
	set_real_metric(
		core::simple_metrics::RealMetricCOP const & metric_in
	);

	/// @brief The mean.
	/// @details Must be finalized first!
	core::Real mean() const;

	/// @brief The median.
	/// @details Must be finalized first!
	core::Real median() const;

	/// @brief The mode.
	/// @details Must be finalized first!
	core::Real mode() const;

	/// @brief The standard deviation of the mean.
	/// @details Must be finalized first!
	core::Real stddev() const;

	/// @brief The standard error of the mean.
	/// @details Must be finalized first!
	core::Real stderror() const;

	/// @brief The minimum value.
	/// @details Must be finalized first!
	core::Real min() const;

	/// @brief The maximum value.
	/// @details Must be finalized first!
	core::Real max() const;

	/// @brief The range of values.
	/// @details Must be finalized first!
	core::Real range() const;

private: // Private data

	/// @brief The simple metric whose value we will be measuring.
	core::simple_metrics::RealMetricCOP simple_metric_;

	/// @brief The values that we have accumulated so far.
	utility::vector1< core::Real > values_;

	/// @brief The average (mean).
	core::Real mean_ = 0.0;

	/// @brief The median.
	core::Real median_ = 0.0;

	/// @brief The mode.
	core::Real mode_ = 0.0;

	/// @brief The standard error of the mean.
	core::Real stderr_ = 0.0;

	/// @brief The standard deviation of the mean.
	core::Real stddev_ = 0.0;

	/// @brief The min.
	core::Real min_ = 0.0;

	/// @brief The max.
	core::Real max_ = 0.0;

	/// @brief The range.
	core::Real range_ = 0.0;

	/// @brief Have we already finalized the values?
	bool derived_finalized_ = false;

#ifdef    SERIALIZATION
public:
	template< class Archive > void save( Archive & arc ) const;
	template< class Archive > void load( Archive & arc );
#endif // SERIALIZATION

};

} //metrics
} //ensemble_metrics
} //protocols

#ifdef    SERIALIZATION
CEREAL_FORCE_DYNAMIC_INIT( protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric )
#endif // SERIALIZATION

#endif //protocols_ensemble_metrics_metrics_CentralTendencyEnsembleMetric_HH
