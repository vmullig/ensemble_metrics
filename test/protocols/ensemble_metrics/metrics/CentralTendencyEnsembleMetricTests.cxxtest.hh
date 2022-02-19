// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// THE FOLLOWING LICENSE APPLIES ONLY TO THIS FILE (EnsembleMetricLoader.hh), WHICH WAS MADE
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

/// @file  protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetricTests.cxxtest.hh
/// @brief  Unit tests for the central tendency ensemble metric.
/// @author Vikram K. Mulligan (vmulligan@flatironinstitute.org)


// Test headers
#include <test/UMoverTest.hh>
#include <test/UTracer.hh>
#include <cxxtest/TestSuite.h>
#include <test/util/pose_funcs.hh>
#include <test/core/init_util.hh>

// Project Headers
#include <protocols/ensemble_metrics/metrics/CentralTendencyEnsembleMetric.hh>

// Protocols Headers
#include <protocols/cyclic_peptide/PeptideStubMover.hh>
#include <protocols/simple_moves/SimpleThreadingMover.hh>

// Core Headers
#include <core/pose/Pose.hh>
#include <core/import_pose/import_pose.hh>
#include <core/select/residue_selector/ResidueNameSelector.hh>
#include <core/simple_metrics/metrics/SelectedResidueCountMetric.hh>

// Utility, etc Headers
#include <basic/Tracer.hh>

static basic::Tracer TR("CentralTendencyEnsembleMetricTests");


class CentralTendencyEnsembleMetricTests : public CxxTest::TestSuite {
	//Define Variables

public:

	void setUp() {
		core_init();

		using namespace protocols::simple_moves;

		core::pose::PoseOP master_pose( utility::pointer::make_shared< core::pose::Pose >() );
		protocols::cyclic_peptide::PeptideStubMover stubmover;
		stubmover.add_residue( protocols::cyclic_peptide::PSM_StubMode::PSM_append, "ALA", 0, true, "", 0, 0, nullptr, "" );
		for ( core::Size i(1); i<=6; ++i ) {
			stubmover.add_residue( protocols::cyclic_peptide::PSM_StubMode::PSM_append, "ALA", 0, false, "", 0, 0, nullptr, "" );
		}
		stubmover.apply( *master_pose );


		for ( core::Size i(1); i<=8; ++i ) {
			if ( i<= 5 ) {
				ensemble1_.push_back( master_pose->clone() );
			}
			if ( i <= 7 ) {
				ensemble2_.push_back( master_pose->clone() );
			}
			ensemble3_.push_back( master_pose->clone() );
		}

		{
			// Ensemble 1:
			SimpleThreadingMover thread1( "AAAAVAA", 1 ); // 1 val
			SimpleThreadingMover thread2( "AVAAAAA", 1 ); // 1 val
			SimpleThreadingMover thread3( "AAVVAAA", 1 ); // 2 val
			SimpleThreadingMover thread4( "AAAAVAA", 1 ); // 1 val
			SimpleThreadingMover thread5( "AAAAIAA", 1 ); // 0 val
			thread1.apply( *ensemble1_[1] );
			thread2.apply( *ensemble1_[2] );
			thread3.apply( *ensemble1_[3] );
			thread4.apply( *ensemble1_[4] );
			thread5.apply( *ensemble1_[5] );
			// Avg. 1.0, median 1.0, mode 1.0.
			// Stdev. sqrt(2/5), Stderr. sqrt(2)/5
			// Min 0, Max 2, range 2
		}

		{
			// Ensemble 2:
			SimpleThreadingMover thread1( "AVVAVAA", 1 ); // 3 val
			SimpleThreadingMover thread2( "AVAAAVV", 1 ); // 3 val
			SimpleThreadingMover thread3( "AAVVAAA", 1 ); // 2 val
			SimpleThreadingMover thread4( "VAAAVAA", 1 ); // 2 val
			SimpleThreadingMover thread5( "AVAAIAA", 1 ); // 1 val
			SimpleThreadingMover thread6( "AAAAIAA", 1 ); // 0 val
			SimpleThreadingMover thread7( "AAVVIVV", 1 ); // 4 val
			thread1.apply( *ensemble2_[1] );
			thread2.apply( *ensemble2_[2] );
			thread3.apply( *ensemble2_[3] );
			thread4.apply( *ensemble2_[4] );
			thread5.apply( *ensemble2_[5] );
			thread6.apply( *ensemble2_[6] );
			thread7.apply( *ensemble2_[7] );
			// Avg. 2.14285714285714, median 2.0, mode 2.0.
			// Stdev. 1.24539969815448, Stderr. 0.470716840598808
			// Min 0, Max 4, range 4
		}

		{
			// Ensemble 3:
			SimpleThreadingMover thread1( "VVVVVVV", 1 ); // 7 val
			SimpleThreadingMover thread2( "VVVVVVV", 1 ); // 7 val
			SimpleThreadingMover thread3( "AAVAAAA", 1 ); // 1 val
			SimpleThreadingMover thread4( "GGVVGGG", 1 ); // 2 val
			SimpleThreadingMover thread5( "VVVIVVV", 1 ); // 6 val
			SimpleThreadingMover thread6( "VVVLVVV", 1 ); // 6 val
			SimpleThreadingMover thread7( "ACEVVVA", 1 ); // 3 val
			SimpleThreadingMover thread8( "SPQRAAA", 1 ); // 0 val
			thread1.apply( *ensemble3_[1] );
			thread2.apply( *ensemble3_[2] );
			thread3.apply( *ensemble3_[3] );
			thread4.apply( *ensemble3_[4] );
			thread5.apply( *ensemble3_[5] );
			thread6.apply( *ensemble3_[6] );
			thread7.apply( *ensemble3_[7] );
			thread8.apply( *ensemble3_[8] );
			// Avg. 4.0, median 4.5, mode 6.5.
			// Stdev. 2.64575131106459, Stderr. 0.935414346693485
			// Min 0, Max 7, range 7
		}

	}

	void tearDown() {

	}



	void test_central_tendency_metric() {
		TR << "Starting CentralTendencyEnsembleMetricTests:test_central_tendency_metric." << std::endl;

		core::select::residue_selector::ResidueNameSelectorCOP name_selector(
			utility::pointer::make_shared< core::select::residue_selector::ResidueNameSelector >( "VAL" )
		);
		core::simple_metrics::metrics::SelectedResidueCountMetricOP rescount(
			utility::pointer::make_shared< core::simple_metrics::metrics::SelectedResidueCountMetric >()
		);
		rescount->set_residue_selector( name_selector );

		protocols::ensemble_metrics::metrics::CentralTendencyEnsembleMetricOP ctmetric(
			utility::pointer::make_shared< protocols::ensemble_metrics::metrics::CentralTendencyEnsembleMetric >()
		);
		ctmetric->set_real_metric( rescount );

		// Ensemble 1:
		for ( core::Size i(1), imax( ensemble1_.size() ); i<=imax; ++i ) {
			ctmetric->apply( *ensemble1_[i] );
		}
		TS_ASSERT( !ctmetric->finalized() );
		TS_ASSERT_EQUALS( ctmetric->poses_in_ensemble(), 5 );
		ctmetric->produce_final_report();
		TS_ASSERT( ctmetric->finalized() );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("mean"), 1.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("median"), 1.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("mode"), 1.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("stddev"), 0.632455532033676, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("stderr"), 0.282842712474619, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("max"), 2.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("min"), 0.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("range"), 2.0, 1.0e-6 );

		TS_ASSERT_DELTA( ctmetric->mean(), 1.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->median(), 1.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->mode(), 1.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->stddev(), 0.632455532033676, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->stderror(), 0.282842712474619, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->max(), 2.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->min(), 0.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->range(), 2.0, 1.0e-6 );

		ctmetric->reset();
		TS_ASSERT( !ctmetric->finalized() );
		TS_ASSERT_EQUALS( ctmetric->poses_in_ensemble(), 0 );

		// Ensemble 2:
		for ( core::Size i(1), imax( ensemble2_.size() ); i<=imax; ++i ) {
			ctmetric->apply( *ensemble2_[i] );
		}
		TS_ASSERT( !ctmetric->finalized() );
		TS_ASSERT_EQUALS( ctmetric->poses_in_ensemble(), 7 );
		ctmetric->produce_final_report();
		TS_ASSERT( ctmetric->finalized() );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("mean"), 2.14285714285714, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("median"), 2.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("mode"), 2.5, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("stddev"), 1.24539969815448, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("stderr"), 0.470716840598808, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("max"), 4.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("min"), 0.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("range"), 4.0, 1.0e-6 );

		TS_ASSERT_DELTA( ctmetric->mean(), 2.14285714285714, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->median(), 2.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->mode(), 2.5, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->stddev(), 1.24539969815448, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->stderror(), 0.470716840598808, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->max(), 4.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->min(), 0.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->range(), 4.0, 1.0e-6 );

		ctmetric->reset();
		TS_ASSERT( !ctmetric->finalized() );
		TS_ASSERT_EQUALS( ctmetric->poses_in_ensemble(), 0 );

		// Ensemble 3:
		for ( core::Size i(1), imax( ensemble3_.size() ); i<=imax; ++i ) {
			ctmetric->apply( *ensemble3_[i] );
		}
		TS_ASSERT( !ctmetric->finalized() );
		TS_ASSERT_EQUALS( ctmetric->poses_in_ensemble(), 8 );
		ctmetric->produce_final_report();
		TS_ASSERT( ctmetric->finalized() );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("mean"), 4.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("median"), 4.5, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("mode"), 6.5, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("stddev"), 2.64575131106459, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("stderr"), 0.935414346693485, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("max"), 7.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("min"), 0.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->get_metric_by_name("range"), 7.0, 1.0e-6 );

		TS_ASSERT_DELTA( ctmetric->mean(), 4.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->median(), 4.5, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->mode(), 6.5, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->stddev(), 2.64575131106459, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->stderror(), 0.935414346693485, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->max(), 7.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->min(), 0.0, 1.0e-6 );
		TS_ASSERT_DELTA( ctmetric->range(), 7.0, 1.0e-6 );

		ctmetric->reset();
		TS_ASSERT( !ctmetric->finalized() );
		TS_ASSERT_EQUALS( ctmetric->poses_in_ensemble(), 0 );

		TR << "Completed CentralTendencyEnsembleMetricTests:test_central_tendency_metric." << std::endl;
	}


	utility::vector1< core::pose::PoseOP > ensemble1_, ensemble2_, ensemble3_;

};
