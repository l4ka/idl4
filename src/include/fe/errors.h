#ifndef __errors_h__
#define __errors_h__

#define ERR_SCAN_UNMATCHED_LT		"unmatched <"
#define ERR_SCAN_UNMATCHED_STR		"unmatched \""
#define ERR_SCAN_ILLEGAL_CHARACTER	"illegal character: '%s'"
#define ERR_SCAN_DECIMAL_OVERFLOW	"decimal constant overflow"
#define ERR_SCAN_HEX_OVERFLOW		"hexadecimal constant overflow"
#define ERR_SCAN_OCTAL_OVERFLOW		"octal constant overflow"
#define ERR_SCAN_INVALID_ESCAPE		"invalid escape sequence: %s"

#define WRN_SCAN_NESTED_COMMENT		"contains nested comment"

// ---

#define ERR_SCAN_MISSING_INCLUDE	"missing include: %s"
#define ERR_SCAN_INCLUDE_DEPTH		"exceeded maximum include depth"
#define ERR_SCAN_INVALID_INCLUDE	"invalid include"

// ---

#define ERR_PARS_DEFINITION_INVALID	"invalid definition"

#define ERR_PARS_CONST_REDEFINED	"constant already defined: %s"
#define ERR_PARS_CONST_TYPE		"type mismatch in constant"
#define ERR_PARS_CONST_INVALID		"invalid constant type"

#define ERR_PARS_SCOPE_REDEFINED	"scope already defined: %s"
#define ERR_PARS_SCOPE_EXPECTED		"expecting scope: %s"

#define ERR_PARS_INTERFACE_EXPECTED	"not an interface: %s"

#define ERR_PARS_IDENTIFIER_EXPECTED	"expecting identifier"

#define ERR_PARS_EXPORT_INVALID		"invalid interface export"

#define ERR_PARS_REFERENCE_UNDEFINED	"undefined reference"

#define ERR_PARS_CONSTANT_INVALID	"invalid constant expression"

#define ERR_PARS_TYPE_INVALID		"invalid type"
#define ERR_PARS_TYPE_REDEFINED		"type already defined: %s"
#define ERR_PARS_TYPE_EXPECTED		"not a type"
#define ERR_PARS_TYPE_UNSUPPORTED	"not supported by this backend: %s"
#define ERR_PARS_TYPE_MISPLACED		"type not allowed here: %s"

#define ERR_PARS_LANG_INT		"use `long' or `short' instead of `int'"
#define ERR_PARS_LANG_UNSIGNED		"`unsigned' must be followed by `long' or `short'"
#define ERR_PARS_LANG_SEQUENCE		"`sequence<...>' cannot be used as a parameter type.  (Hint: use a `typedef'ed sequence type)"

#define ERR_PARS_STRUCT_REDEFINED	"struct already defined: %s"
#define ERR_PARS_STRUCT_UNBOUNDED	"size of struct is not known: %s"
#define ERR_PARS_STRUCT_EXPECTED	"struct type expected"
#define ERR_PARS_STRUCT_MEMBER		"invalid structure member"
#define ERR_PARS_STRUCT_AMBIGUOUS	"ambiguous struct member: %s"

#define ERR_PARS_PROPERTY_INVALID	"invalid attribute: %s"

#define ERR_PARS_UNION_UNBOUNDED	"size of union is not known: %s"
#define ERR_PARS_UNION_REDEFINED	"union already defined: %s"
#define ERR_PARS_UNION_EXPECTED		"union type expected"

#define ERR_PARS_SWITCH_DISCRIM		"invalid switch type"
#define ERR_PARS_SWITCH_CASE		"invalid case"

#define ERR_PARS_ENUM_REDEFINED		"enum already defined: %s"

#define ERR_PARS_SEQUENCE_LENGTH	"sequence length is not an integer"

#define ERR_PARS_STRING_LENGTH		"string length is not an integer"
#define ERR_PARS_STRING_EXPECTED	"expecting string"

#define ERR_PARS_FIXED_SIZE		"fixed size is not an integer"

#define ERR_PARS_ARRAY_DIMENSION	"malformed array dimension"

#define ERR_PARS_EXCEPTION_UNBOUNDED	"size of exception is not known: %s"
#define ERR_PARS_EXCEPTION_REDEFINED	"exception already defined: %s"
#define ERR_PARS_EXCEPTION_EXPECTED	"exception expected: %s"

#define ERR_PARS_OPERATION_REDEFINED	"operation already defined: %s"
#define ERR_PARS_OPERATION_INVALID	"invalid operation"
#define ERR_PARS_OPERATION_AMBIGUOUS	"ambiguous parameter: %s"
#define ERR_PARS_OPERATION_ONEWAYOUT	"no out values allowed for oneway operation"

#define ERR_PARS_PARAMETER_PROPERTY	"expecting `in', `inout', or `out'"

#define ERR_PARS_CONTEXT_STRING		"context expression missing string"

#define ERR_PARS_ATTRIBUTE_REDEFINED	"attribute already defined: %s"

#define ERR_PARS_GENERAL_NUMBER		"number not expected"
#define ERR_PARS_GENERAL_EXPECTING	"expecting '%c'"

#define ERR_PARS_IMPORT_FAILED		"parse error in imported file: %s"

#endif
