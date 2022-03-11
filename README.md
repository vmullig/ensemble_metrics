# EnsembleMetrics

## Description

A set of tools for analyzing a collection of heteropolymer structures, and for measuring properties of or producing information about the ensemble as a whole.  These were developed with the [Rosetta software suite](https://www.rosettacommons.org/software]) in mind; however, they may be more broadly useful, so I am first releasing this code under a permissive (MIT) licence.  This allows it to be altered for incorporation into other software projects should it prove useful.

## Author

Vikram K. Mulligan (vmulligan@flatironinstitute.org)

## Usage

### Patching the Rosetta software suite

The `patches/` directory contains the following patchfile(s), which can be used to add `EnsembleMetrics` to Rosetta.  Note that the intention is for these to be incorporated into the public releases of Rosetta in the near future.

Name | Description | Can be applied to
---- | ----------- | -----------------
ensemble\_metrics.patch | The `EnsembleMetrics` base class, plus the derived `CentralTendencyEnsembleMetric` (which measures mean, median, mode, _etc._ of an input value produced by a Rosetta `SimpleMetric`). | Rosetta Git SHA 9907e74b22a2c9bb71d52b56a7110a107baf621d (master branch, 8 March 2022).
mpi\_support.patch | Adds support for analysing a large ensemble of poses on a cluster using MPI (massive parallelism). | Rosetta Git SHA 9907e74b22a2c9bb71d52b56a7110a107baf621d (master branch + ensemble\_metrics.patch, 11 March 2022).

Patchfiles can be applied by navigating to your `Rosetta/main` directory (_e.g._ `cd my_rosetta_installation/Rosetta/main`), copying the patchfile to the current directory, and using the Linux `patch` command:

```sh
patch -p1 < ensemble_metrics.patch
```

### Adapting for other software projects

The `src/protocols/ensemble_metrics` directory contains source code for the `EnsembleMetrics` framework.  Although this is intended to be compiled against Rosetta headers and linked against Rosetta, one may easily replace Rosetta objects (such as `Poses`, `SimpleMetrics`, and `ResidueSelectors`) with their equivalents from other software packages.  (The beauty of an MIT licence is that it allows full refactoring of the code to suit one's needs).

In `src/protocols/ensemble_metrics`, source code for derived classes (particular `EnsembleMetrics`) such as the `CentralTendencyEnsembleMetric` may be found.  The `src/protocols/init` directory contains initialization functions for a factory system (which may or may not be useful in a new context).  The `src/protocols/parser` directory contains code allowing the instantiation of `EnsembleMetric` subclasses when they are invoked in an XML script.  (These functions are used to make `EnsembleMetrics` accessible to the [RosettaScripts](https://www.rosettacommons.org/docs/latest/scripting_documentation/RosettaScripts/RosettaScripts) scripting language in Rosetta, but could be useful elsewhere.)

The `test` directory contains unit tests for the derived classes of the `EnsembleMetric` base class.

### Citing this work

This work is currently unpublished.  If you find it useful for your work, please consider including the author (Vikram K. Mulligan) in your publication.  This page will be updated when this work is published.

### Licence

This work is made available under an MIT licence (see LICENCE file).  This gives anyone great leeway to modify, refactor, or redistribute this code, and to use it in their own personal, open-source, or commercial projects. 
