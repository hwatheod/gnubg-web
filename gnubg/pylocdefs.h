/*
 * pylocdefs.h
 *
 * by Michael Petch <mpetch@capp-sysware.com>, 2014.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: pylocdefs.h,v 1.6 2014/08/08 13:38:16 mdpetch Exp $
 */

#ifndef PYLOCDEFS_H
#define PYLOCDEFS_H

#include "config.h"

#if USE_PYTHON

#if PY_MAJOR_VERSION >= 3
    #define MOD_ERROR_VAL NULL
    #define MOD_SUCCESS_VAL(val) val
    #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
    #define MOD_DEF(ob, name, doc, methods) \
            static struct PyModuleDef moduledef = { \
              PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
            ob = PyModule_Create(&moduledef);
#else
    #define MOD_ERROR_VAL
    #define MOD_SUCCESS_VAL(val)
    #define MOD_INIT(name) void init##name(void)
    #define MOD_DEF(ob, name, doc, methods) \
            ob = Py_InitModule3(name, methods, doc);
#endif

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#endif

#if PY_VERSION_HEX < 0x02060000
    #define PyBytes_FromStringAndSize PyString_FromStringAndSize
    #define PyBytes_FromString PyString_FromString
    #define PyBytes_AsString PyString_AsString
    #define PyBytes_Size PyString_Size
    #define PyBytes_Check PyString_Check
    #define PyUnicode_FromString PyString_FromString
#endif

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif                          /* PY_VERSION_CHK */

#if PY_MAJOR_VERSION >= 3
    #define PyInt_Check PyLong_Check
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_FromLong PyLong_FromLong
    #define PyExc_StandardError PyExc_Exception
#endif

#if (PY_VERSION_HEX < 0x02030000)
/* Bool introduced in 2.3 */
#define PyBool_FromLong PyLong_FromLong

/* Fix incorrect prototype in early python */
#define CHARP_HACK (char*)
#else
#define CHARP_HACK
#endif

#endif                         /* USE_PYTHON */

#endif
