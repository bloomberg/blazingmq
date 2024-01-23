#ifndef INCLUDED_Z_BMQT_COMPRESSIONALGORITHMTYPE
#define INCLUDED_Z_BMQT_COMPRESSIONALGORITHMTYPE

#include <stdbool.h>

struct z_bmqt_CompressionAlgorithmType {
    // TYPES
    enum Enum { ec_UNKNOWN = -1, ec_NONE = 0, ec_ZLIB = 1 };

    // CONSTANTS

    /// NOTE: This value must always be equal to the lowest type in the
    /// enum because it is being used as a lower bound to verify that a
    /// header's `CompressionAlgorithmType` field is a supported type.
    static const int k_LOWEST_SUPPORTED_TYPE = ec_NONE;

    /// NOTE: This value must always be equal to the highest type in the
    /// enum because it is being used as an upper bound to verify that a
    /// header's `CompressionAlgorithmType` field is a supported type.
    static const int k_HIGHEST_SUPPORTED_TYPE = ec_ZLIB;

    /// Return the non-modifiable string representation corresponding to the
    /// specified enumeration `value`, if it exists, and a unique (error)
    /// string otherwise.  The string representation of `value` matches its
    /// corresponding enumerator name with the "ec_" prefix eluded.  For
    /// example:
    /// ```
    /// bsl::cout << CompressionAlgorithmType::toAscii(
    ///                             CompressionAlgorithmType::ec_NONE);
    /// ```
    /// will print the following on standard output:
    /// ```
    /// NONE
    /// ```
    /// Note that specifying a `value` that does not match any of the
    /// enumerators will result in a string representation that is distinct
    /// from any of those corresponding to the enumerators, but is otherwise
    /// unspecified.
    static const char* toAscii(z_bmqt_CompressionAlgorithmType::Enum value);

    /// Update the specified `out` with correct enum corresponding to the
    /// specified string `str`, if it exists, and return true.  Otherwise in
    /// case of an error or unidentified string, return false.  The expected
    /// `str` is the enumerator name with the "ec_" prefix excluded.
    /// For example:
    /// ```
    /// CompressionAlgorithmType::fromAscii(out, NONE);
    /// ```
    ///  will return true and the value of `out` will be:
    /// ```
    /// bmqt::CompresssionAlgorithmType::ec_NONE
    /// ```
    /// Note that specifying a `str` that does not match any of the
    /// enumerators excluding "ec_" prefix will result in the function
    /// returning false and the specified `out` will not be touched.
    static bool fromAscii(z_bmqt_CompressionAlgorithmType::Enum* out,
                          const char*              str);

    /// Return true incase of valid specified `str` i.e. a enumerator name
    /// with the "ec_" prefix excluded.  Otherwise in case of invalid `str`
    /// return false and populate the specified `stream` with error message.
    static bool isValid(const char* str);
};

#endif