/*
 * Copyright 2020  Ismael Luceno <ismael@iodev.co.uk>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#if defined(HEADER_ASN1_H) && !defined(HAVE_ASN1_STRING_GET0_DATA)
const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *);
#endif
