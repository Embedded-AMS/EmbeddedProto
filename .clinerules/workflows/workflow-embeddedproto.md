
# Building and running unit tests
To build the unit tests use: `./build_test.sh`
To run the unit tests use: `./build/test/test_EmbeddedProto`

# Buildin the python package
Activate the virtual env if not already active: `source ./venv/bin/activate`
Build the package: `build_package.sh`

# Doxygen C++ code documentation
When documenting C and C++ code with doxygen do not use the @brief format but the \brief format. 

For short function documentation use the //! format.

For large bloks use:
/*! \brief Brief description.
 *         Brief description continued.
 *
 *  Detailed description starts here.
 */
