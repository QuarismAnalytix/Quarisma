/*
 * Some parts of this implementation were inspired by code from VTK
 * (The Visualization Toolkit), distributed under a BSD-style license.
 * See LICENSE for details.
 */

#ifndef __wrapping_hints_h__
#define __wrapping_hints_h__

#ifdef __QUARISMA_WRAP__
#define QUARISMA_WRAP_HINTS_DEFINED
// Exclude a method or class from wrapping
#define QUARISMA_WRAPEXCLUDE [[quarisma::wrapexclude]]
// The return value points to a newly-created QUARISMA object.
#define QUARISMA_NEWINSTANCE [[quarisma::newinstance]]
// The parameter is a pointer to a zerocopy buffer.
#define QUARISMA_ZEROCOPY [[quarisma::zerocopy]]
// The parameter is a path on the filesystem.
#define QUARISMA_FILEPATH [[quarisma::filepath]]
// Set preconditions for a function
#define QUARISMA_EXPECTS(x) [[quarisma::expects(x)]]
// Set size hint for parameter or return value
#define QUARISMA_SIZEHINT(...) [[quarisma::sizehint(__VA_ARGS__)]]
// Opt-in a class for automatic code generation of (de)serializers.
#define QUARISMA_MARSHALAUTO [[quarisma::marshalauto]]
// Specifies that a class has hand written (de)serializers.
#define QUARISMA_MARSHALMANUAL [[quarisma::marshalmanual]]
// Excludes a function from the auto-generated (de)serialization process.
#define QUARISMA_MARSHALEXCLUDE(reason) [[quarisma::marshalexclude(reason)]]
// Enforces a function as the getter for `property`
#define QUARISMA_MARSHALGETTER(property) [[quarisma::marshalgetter(#property)]]
// Enforces a function as the setter for `property`
#define QUARISMA_MARSHALSETTER(property) [[quarisma::marshalsetter(#property)]]
#endif

#ifndef QUARISMA_WRAP_HINTS_DEFINED
#define QUARISMA_WRAPEXCLUDE
#define QUARISMA_NEWINSTANCE
#define QUARISMA_ZEROCOPY
#define QUARISMA_FILEPATH
#define QUARISMA_EXPECTS(x)
#define QUARISMA_SIZEHINT(...)
#define QUARISMA_MARSHALAUTO
#define QUARISMA_MARSHALMANUAL
#define QUARISMA_MARSHALEXCLUDE(reason)
#define QUARISMA_MARSHALGETTER(property)
#define QUARISMA_MARSHALSETTER(property)
#endif

#define QUARISMA_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT "is redundant"
#define QUARISMA_MARSHAL_EXCLUDE_REASON_IS_INTERNAL "is internal"
#define QUARISMA_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED \
    "(de)serialization is not supported for this type of property"

#endif
// QUARISMA-HeaderTest-Exclude: wrapping_hints.h
