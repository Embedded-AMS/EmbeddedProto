
# Building and running unit tests
To build the unit tests use: `./build_test.sh`
To run the unit tests use: `./run_tests.sh` 

# Buildin the python package
Activate the virtual env if not already active: `source ./venv/bin/activate`
Build the package: `build_package.sh`

# Standarts
C++ code should comply with MISRA C++ 2023.

# C++ code formatting example
* Note the two spaces as indentation.
* Use Unix style line endings.

```cpp
namespace EmbeddedProto 
{

enum class Error 
{
  NO_ERRORS = 0,
  END_OF_BUFFER = 1,
};

class MyMessage final : public ::EmbeddedProto::MessageInterface 
{
    //! Private constexpr
    static constexpr uint32_t PRIVATE = 1;

  public:
    //! Public constexpr
    static constexpr uint32_t PUBLIC = 1;
    
    //! Convenience typedef
    void some_func(SomeClass& someClass);
    
    // ... etc  ...
        
  private:
    //! Internal helpers for each phase
    Error serialize_tag_phase(WriteBufferInterface& buffer, MessageState& state) const;

};

void function_name(int32_t varA)
{ 
  if(5 == varA) // Not the constant 5 is the lefthand argument and the variable the righthand.
  { // Bracket on the next line
    
  }
  else if(10 == varA)
  {

  }
  else 
  {
    // Do nothing
  }

  for(uint32_t i = 0; i < 10; ++i)
  {
    // Do stuff.
  }

  while(true)
  {
    // Do stuff.
  }
}

} // namespace EmbeddedProto
```

# Doxygen C++ code documentation
When documenting C and C++ code with doxygen do not use the @brief format but the \brief format. 

For short function documentation use the //! format.

For large bloks use:
/*! \brief Brief description.
 *         Brief description continued.
 *
 *  Detailed description starts here.
 */

When documenting variables enum values inline use //<!:
enum class Phase : uint8_t 
{
  TAG = 0,  //<! Reading/writing field tag (field number + wire type)
  SIZE = 1, //<! Reading/writing length prefix (for LENGTH_DELIMITED fields)
};

# Design
* When designing do not estimate time.
