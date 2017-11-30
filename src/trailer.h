/*
 * Copyright (C) 2010 IEQualize GmbH <info@iequalize.de>
 *
 */
#ifndef _TRAILER_H
#define _TRAILER_H

/* max length of version string, e.g. "0.33.1pre1" */
#define MAX_VERSION 16

/* "Pack" our struct which is necessary for mingw cross compile.
 * Windows uses a different byte alignment then Linux!
 */
struct update_trailer {
	uint8_t		version[MAX_VERSION];
	uint32_t	model_tag;
	uint32_t	loopaes_length;
	/* keep the trailer version info last */
	uint32_t	trailer_version;
} __attribute__((packed, gcc_struct));

#endif
