# CSC2525 Project: Weighing Elias-Fano against Golomb Delta Compression Methods for Sorted Integers

**Author:** Michael Zhou  
**Date:** 2025-04-09

## Overview

This repository contains a C++ project that implements and evaluates two compression methods for sorted integers: **Elias-Fano** and **Golomb Delta** encoding. These methods exploit the small differences between successive integers to achieve efficient compression. The project includes:

- An implementation of a custom **BitVector** class with constant-time rank and select operations.
- Implementations of the **Elias-Fano** and **Golomb Delta** encoding schemes.
- Benchmarks to test and compare:
  - Encoded size (in bits)
  - Encoding time (in microseconds)
  - Random access time for retrieving the \(i\)-th element (in microseconds)
  - Compression ratio (relative to storing integers as 64-bit values)

Results are printed to standard output and exported to a CSV file (`results.csv`) for visualization.

## Building the Project

This project uses a Makefile to compile the C++ sources. All object files and the executable are placed in the `build/` directory.

To build and run the project, run:

```bash
make run
```

This will execute the test driver, print performance metrics to the terminal, and create a CSV file (results.csv) with the experimental data.

You may also run the visualization script to generate the graphs:
```bash
python plot_results.py
```

## Acknowledgments
Thank you CSC2525 and Niv for teaching me all this knowledge!

