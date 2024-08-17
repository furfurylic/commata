/**
 * These codes are licensed under the Unlicense.
 * http://unlicense.org
 */

#ifndef COMMATA_GUARD_FDC314CE_0C3D_47F8_AD9E_EF60154FAB89
#define COMMATA_GUARD_FDC314CE_0C3D_47F8_AD9E_EF60154FAB89

#if defined(COMMATA_ENABLE_FULL_EBO) \
 && defined(_MSC_VER) && _MSC_FULL_VER >= 190024210
#define COMMATA_FULL_EBO __declspec(empty_bases)
#else
#define COMMATA_FULL_EBO
#endif

#endif
