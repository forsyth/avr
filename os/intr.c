#include "u.h"
#include "mem.h"
#include "os.h"
#include "io.h"

/*
 * vectors are in flash and can't usually be changed (even by SPM).
 * they are statically initialised by mkvec/vec.s from a config file.
 * (there seems little need for dynamically-programmed software vectors.)
 * state is saved by l.s on an interrupt, and restored on return from the C handler.
 */
