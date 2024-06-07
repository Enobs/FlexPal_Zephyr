# zcbor v. 0.7.0 (2023-04-01)

Any new bugs, requests, or missing features should be reported as [Github issues](https://github.com/NordicSemiconductor/zcbor/issues).

## Improvements:

### Code generation
 * Add support for float16 encoding and decoding
 * Add support for C++ in generated code
 * Use (U)INT(8|16|32|64)_(MIN|MAX) macros in generated code
 * Add the --c-code-columns option for adding newlines to c_code
 * Refactor CBOR <-> YAML/JSON translation
 * Allow adding file headers to generated files, e.g. for copyright.

### C libraries
 * Add support for float16 encoding and decoding
 * Add helper functions zcbor_compare_strings() and zcbor_header_len()
 * Rename prim/primitive to simple
 * Add new APIs zcbor_uint_(encode/decode)
 * Move tags enum into separate file zcbor_tags.h and add more tags from the IANA list
 * When generating cmake files, make all paths relative to the cmake file.
 * Use the zcbor_compare_strings() function in str_expect()
 * Make the zcbor source files compile with -Wall and -Wconversion
 * Refactor zcbor_any_skip()
 * Change all uint_fast32_t usage to use size_t instead

### Other
 * README: Improve docs, particularly to help new users
 * Add two samples: a hello world with no code generation, and one with code generation and script usage
 * Port decode/encode/unit tests to new ztest API
 * Fix fuzz tests

## Bugfixes:
 * zcbor.py: Fix 'zcbor --version' so it works again
 * zcbor.py: Fix cbor bstrs so they properly check unambiguous values
 * zcbor.py: Fix rendering of entry functions when they have no output
 * zcbor.py: Don't flag cbor bstrs as unambiguous

## Unsupported CDDL features
Not all features outlined in the CDDL specs [RFC8610](https://datatracker.ietf.org/doc/html/rfc8610), [RFC9090](https://datatracker.ietf.org/doc/html/rfc9090), and [RFC9165](https://datatracker.ietf.org/doc/html/rfc9165) are supported by zcbor.
The following is a list of limitations and missing features:

 * Map elements in data must appear in the same order as they appear in the CDDL.
 * C API doesn't support searching through a map.
 * Using `&()` to turn groups into choices (unions). `&()` is supported when used with `.bits`.
 * Representation Types (`#x.y`), except for tags (`#6.y(foo)`) which are supported.
 * Unwrapping (`~`)
 * The control operators `.regexp`, `.ne`, `.default`, and `.within` from RFC8610.
 * The control operators `.sdnv`, `.sdnvseq`, and `.oid` from RFC9090.
 * The control operators `.plus`, `.cat`, `.det`, `.abnf`, `.abnfb`, and `.feature` from RFC9165.
 * Generics (`foo<a, b>`).
 * Using `:` for map keys.
 * Cuts, either via `^` or implicitly via `:`.
 * Most of the "Extended Diagnostic Notation" is unsupported.


# zcbor v. 0.6.0 (2022-10-12)

Any new bugs, requests, or missing features should be reported as [Github issues](https://github.com/NordicSemiconductor/zcbor/issues).

## Improvements:
 * Refactor zcbor CLI to add 'verify' and put all options after keyword
 * Make several C++ related improvements in the C library
 * Fix Coverity warnings in C library and generated code
 * zcbor_decode: Add zcbor_int[32|64]_expect_union
 * zcbor_common.h: Add new zcbor_assert_state() macro
 * zcbor_common.h: Rename ARRAY_SIZE to ZCBOR_ARRAY_SIZE to avoid #include order problems in Zephyr
 * .github: Refactor Github action and add QEMU testing on mps2_an521 (ARM) and qemu_malta_be (MIPS, big-endian)
 * zcbor.py: Raise argparse error on insufficient --output-* args
 * zcbor_decode.h: Define variables to avoid rvalue address errors
 * Add release dates to all RELEASE_NOTES entries

## Bugfixes:
 * zcbor.py: Fix bug where the union_int optimization decodes twice
 * zcbor_encode.c: Fix error for big-endian architectures

## Unsupported CDDL features
Not all features outlined in the [CDDL spec](https://datatracker.ietf.org/doc/html/rfc8610) are supported by zcbor.
The following is a list of limitations and missing features:

 * Map elements in data must appear in the same order as they appear in the CDDL.
 * C API doesn't support searching through a map.
 * 16-bit floating point numbers (float16).
 * Using `&()` to turn groups into choices (unions). `&()` is supported when used with `.bits`.
 * Representation Types (`#x.y`), except for tags (`#6.y(foo)`) which are supported.
 * Unwrapping (`~`)
 * The control operator `.regexp`.
 * The control operator `.ne`.
 * The control operator `.default`.
 * Generics (`foo<a, b>`).
 * Most of the "Extended Diagnostic Notation" is unsupported.


# zcbor v. 0.5.1 (2022-06-21)

Any new bugs, requests, or missing features should be reported as [Github issues](https://github.com/NordicSemiconductor/zcbor/issues).

## Improvements:
 * requirements.txt: Change PyYAML requirement from >6.0.0 to >5.4.1 to be more widely compatible.
 * zcbor_encode/zcbor_decode: Add new functions zcbor_int_encode() and zcbor_int_decode() which accept the integer size as an argument.
 * zcbor.py: Use zcbor_int_encode() and zcbor_int_decode() when operating directly on _choice enums, since enums can be any size, depending on compiler and project. This removes the restrictions that enums must be 32 bits.

## Bugfixes:
 * tests: Fix an error in manifest3.cddl (encode/test1) (naming collision with 'uri')

## Unsupported CDDL features
Not all features outlined in the [CDDL spec](https://datatracker.ietf.org/doc/html/rfc8610) are supported by zcbor.
The following is a list of limitations and missing features:

 * Map elements in data must appear in the same order as they appear in the CDDL.
 * C API doesn't support searching through a map.
 * 16-bit floating point numbers (float16).
 * Using `&()` to turn groups into choices (unions). `&()` is supported when used with `.bits`.
 * Representation Types (`#x.y`), except for tags (`#6.y(foo)`) which are supported.
 * Unwrapping (`~`)
 * The control operator `.regexp`.
 * The control operator `.ne`.
 * The control operator `.default`.
 * Generics (`foo<a, b>`).
 * Most of the "Extended Diagnostic Notation" is unsupported.


# zcbor v. 0.5.0 (2022-06-13)

Any new bugs, requests, or missing features should be reported as [Github issues](https://github.com/NordicSemiconductor/zcbor/issues).

## Improvements:

### Code generation
 * Add the --short-names option when generating code, which shortens names in struct definitions.
 * Allow --default-max-qty to accept a string label when generating code so that the value can be configured after generation.
 * Update versions of dependencies in pip requirements.

### C Libraries
 * Add const to *ptr argument of cbor_tstr_encode_ptr.
 * Make it easier to initialize zcbor_state_t objects: Allow single states as input to zcbor_new_state() and remove return value.
 * Make zcbor_list/map_end_encode() more robust to wrong max_num arguments (when ZCBOR_CANONICAL is enabled).
 * Add zcbor_peek_error() for reading the error value without clearing it.
 * Add zcbor_size_put/encode/decode/expect functions, to allow directly encoding and decoding size_t type variables.

### Tests
 * Test on multiple Python versions
 * Add tests for using ranges (< or >) with floats in code generation.
 * Add/enable test for signature check of SUIT manifests.


## Bugfixes:
 * zcbor.py: Fix missing size check for repeated bstrs with their own type
 * zcbor_encode.c: Fix encoding of floats equal to 0.0 (or any with leading 0s)
 * zcbor_encode.h: Fix "taking address of rvalue" errors with some compiler configurations, stemming from zcbor_b|tstr_encode_ptr()
 * zcbor.py: Add forward declarations to functions in generated code.
 * zcbor.py: Fix bug when adding tagged (#6.x) items to larger types, e.g. lists.
 * zcbor.py: Fix naming bug for container types with no members.
 * zcbor.py: "union_int" optimization: If all union members start with an integer, the optimization does a single int_decode() and identifies the correct union member by the result.
   * zcbor.py: Fix a bug preventing the "union_int" optimization from being applied.
   * zcbor.py: Expand the "union_uint" optimization to work on all integers, instead of only positive integers.


## Unsupported CDDL features
Not all features outlined in the [CDDL spec](https://datatracker.ietf.org/doc/html/rfc8610) are supported by zcbor.
The following is a list of limitations and missing features:

 * Map elements in data must appear in the same order as they appear in the CDDL.
 * C API doesn't support searching through a map.
 * 16-bit floating point numbers (float16).
 * Using `&()` to turn groups into choices (unions). `&()` is supported when used with `.bits`.
 * Representation Types (`#x.y`), except for tags (`#6.y(foo)`) which are supported.
 * Unwrapping (`~`)
 * The control operator `.regexp`.
 * The control operator `.ne`.
 * The control operator `.default`.
 * Generics (`foo<a, b>`).
 * Most of the "Extended Diagnostic Notation" is unsupported.


# zcbor v. 0.4.0 (2022-03-31)

**cddl-gen has been renamed to zcbor!**

Any new bugs, requests, or missing features should be reported as [Github issues](https://github.com/NordicSemiconductor/zcbor/issues).

## Improvements:

### CDDL language
 * Add support for 32-bit and 64-bit floating point numbers.
 * Add support for `undefined` in CDDL.
 * Add the CDDL prelude (from RFC 8610 Appendix D) to all builds (unless excluded with `--no-prelude`)
 * Add support for more complex `.cbor` and `.cborseq` expressions.


### Command line and Python module
 * Rename script/module to zcbor.
 * Better 64-bit support:
   * New argument: `--default-bit-size` which controls the assumed size of integers. See zcbor code `--help` for more information.
 * Add the `--include-prefix` option for path prefix to generated h files.
 * Allow using both `--encode` and `--decode` together to generate both at the same time.
 * Make changes to generated code to build with `-Wpedantic` and `-Wconversion`.


### C Libraries
 * All public function names have been prefixed with `zcbor_`.
 * Many function names have also been changed slightly.
 * Better API docs.
 * Add error value to state, which is populated when a function fails (returns `false`).
   * Add a `stop_on_error` feature that makes functions abort when an error has occurred (to avoid checking the output of all functions).
 * Add support for 32-bit and 64-bit floating point numbers.
 * Better 64-bit support:
   * New 64-bit integer decoding and encoding functions (in addition to existing 32-bit functions).
   * `uint32_t` usage has been largely transitioned to `size_t` or `uint_fast32_t` which are typically the same size as the architecture (32 or 64 bits).
   * This means that list/map/string sizes can be 64 bits on 64-bit architectures. Note that having 64-bit sizes on 32-bit architectures rarely make sense since they won't fit in the address space.
 * New helper macros for instantiating state variables: `ZCBOR_STATE_D()` and `ZCBOR_STATE_E()` for decoding and encoding respectively.
 * Add support for decoding payloads split over multiple memory chunks.
   * There are new APIs in zcbor_common.h and zcbor_decode.h to handle this. Look for `zcbor_update_state()` and various functions with `fragment` in the name.
   * Strings can be split between chunks, there is a new type `struct zcbor_string_fragment` for this purpose.
   * Data headers can not be split.
   * Backups to previous chunks cannot be restored.
 * Split `zcbor_multi_encode()` into `zcbor_multi_encode()` and `zcbor_multi_encode_minmax()`. The former is more useful for manual use, while the latter is for generated code.
 * Remove `zcbor_any_encode()` since it was just an alias for `zcbor_nil_put()`.
 * Rename `zcbor_string_type_t` to `struct zcbor_string`.
 * Rename `zcbor_state_backups_t` to `struct zcbor_state_constant`.
 * Add support for decoding and encoding `undefined` CBOR values.
 * `zcbor_any_decode()`:
   * Rename to `zcbor_any_skip()`
   * Add support for indeterminate-length arrays.
   * Have `zcbor_any_skip()` consume any tags that are present.
 * Introduce `zcbor_bstr_expect_ptr()`, `zcbor_bstr_put_ptr()`, `zcbor_tstr_expect_ptr()`, and `zcbor_tstr_put_ptr()`
 * zcbor_common.h: Add an enum (`zcbor_rfc8949_tag`) with some tag values.
 * Move CBOR intro text from zcbor_decode.h to README.
 * Control asserts separately from verbose (add ZCBOR_ASSERT option)
 * Introduce `zcbor_list_map_end_force_encode()` and `zcbor_list_map_end_force_decode()`.


### Tests
 * decode: Remove test4 since it hasn't been used or updated.
 * New unit test directory and project. Tests:
   * 64-bit vs 32-bit integers.
   * 64-bit vs 32-bit strings.
   * String encoding and decoding macros (`_lit()`, `_arr()`, `_term()`).
   * Payload chunks and string fragments.
 * Collect boilerplate Cmake code for tests into a new file test_template.cmake.
 * Add tests for draft-ietf-suit-manifest-16.
 * Rename "strange" tests to "corner_cases"
 * Build tests with `-Wpedantic` and `-Wconversion`


## Bugfixes:
 * zcbor.py: Avoid #include-ing the full path of the type file.
 * zcbor.py: Avoid creating functions with "None" result type.
 * src: Fix code so it builds correctly with asserts enabled.
 * zcbor_decode.c: Fix `list_map_start_decode()` to handle elem_count correctly
 * zcbor_decode.c: Fix `zcbor_any_skip()` so it reverts the state correctly on fail.
 * zcbor.py: Fix regex for ints/floats to ignore names with hyphens (-)
 * zcbor.py: Fix error in 'convert' where a bstr dict value was incorrectly translated
 * setup.py: Fix windows install
 * zcbor.py: Add suffix for literal values larger than 32-bits.
 * src: Fix code to build with `-Wpedantic` and `-Wconversion`.
 * zcbor.py: Fix some code generation bugs to do with range checking (min/max size) of ints and CBOR-encoded bstrs.
 * zcbor.py: Fix bug where indirection for ints with range checks was wrong.
 * zcbor.py: Fix bugs in range checks in `bstr`s with `.cbor` statements.

## Unsupported CDDL features

Not all features outlined in the [CDDL spec](https://datatracker.ietf.org/doc/html/rfc8610) are supported by zcbor.
The following is a list of limitations and missing features:

 * Map elements in data must appear in the same order as they appear in the CDDL.
 * C API doesn't support searching through a map.
 * 16-bit floating point numbers (float16).
 * Using `&()` to turn groups into choices (unions). `&()` is supported when used with `.bits`.
 * Representation Types (`#x.y`), except for tags (`#6.y(foo)`) which are supported.
 * Unwrapping (`~`)
 * The control operator `.regexp`.
 * The control operator `.ne`.
 * The control operator `.default`.
 * Generics (`foo<a, b>`).
 * Most of the "Extended Diagnostic Notation" is unsupported.


# cddl-gen v. 0.3.0 (2021-11-17)

Any new bugs, requests, or missing features should be reported as [Github issues](https://github.com/NordicSemiconductor/cddl-gen/issues).

## Improvements:

### CDDL language
 * Add more support for hexadecimal/octal/binary numbers in CDDL. E.g. in size ranges or value limits
 * Add support for ranges with three dots in CDDL files
 * Change CDDL quantifier syntax from `x**y` to `x*y`
 * Add support for `$$` and `$`
 * Add support for the `.bits` operator.

### Command line
 * Change command line name from `cddl_gen` to `cddl-gen`
 * Add `--dq` as a shorthand for `--default-max-qty`
 * Allow passing multiple cddl files via command line
 * Allow adding the current git sha to generated file headers.
 * Various changes to the logic for naming in code generation
   * This can give different names of e.g. struct members compared with previous versions of cddl-gen
 * Generate a cmake file when generating code.
   * The `target_cddl_source()` Cmake function has been removed.
   * See README.md for more info.

### Python module
 * Improve tags support in DataTranslator
 * Improve decoding of cbor bstrs in DataTranslator
   * Provide the raw string along with the decoded object.

### C Libraries
 * Refactor cbor_common.c
 * Add support for indefinite length arrays in cbor_decode.c.

### Tests
 * Move all cddl files into tests/cases
 * Run tests with twister instead of custom bash scripts
 * Add tests for draft-ietf-suit-manifest-14


## Bugfixes:
 * cbor_encode.h: Add missing `.` in `tstrx_put` and `tstrx_put_term`
 * cbor_encode.c: Fix `get_result_len` and add big-endian support
 * cddl_gen.py: Fix bug where range checks were dropped
 * cbor_decode.c: Fix bug where a `nil` could be interpreted as `true`
 * cddl_gen.py: Fix parsing of ranges with no max (`n*` or `n**`)
 * cbor_encode.c: Fix bug where minimum quantity restrictions were ignored
 * cddl_gen: Fix generation of map-within-map
 * cbor_decode.c: Add overflow checks and fix assert in `any_decode()`.


## Unsupported CDDL features

Not all features outlined in the [CDDL spec](https://datatracker.ietf.org/doc/html/rfc8610) are supported by cddl-gen.
The following is a list of limitations and missing features:

 * Map elements in data must appear in the same order as they appear in the CDDL.
 * Floating point numbers.
 * Using `&()` to turn groups into choices (unions). `&()` is supported when used with `.bits`.
 * Representation Types (`#x.y`), except for tags (`#6.y(foo)`) which are supported.
 * Unwrapping (`~`)
 * The control operator `.regexp`.
 * The control operator `.ne`.
 * The control operator `.default`.
 * Generics (`foo<a, b>`).
 * Not all types from the "standard prelude" are supported.
 * Most of the "Extended Diagnostic Notation" is unsupported.
