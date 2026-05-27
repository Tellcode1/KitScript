#ifndef E_PROGRAM_VALIDATOR_H
#define E_PROGRAM_VALIDATOR_H

#include "stdafx.h"

#include <stdio.h>

struct e_exec_info;

/**
 * Validate a program. 0 on success.
 * Errors are printed to f.
 * Validation does not stop on error, continuing until
 * every error is reported.
 */
int e_validate(const struct e_exec_info* info, FILE* f) RETURNS_ERRCODE;

#endif // E_PROGRAM_VALIDATOR_H