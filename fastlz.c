#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "fastlz/fastlz.h"

#ifdef _MSC_VER
typedef unsigned __int32 uint32_t;
#else
#include <stdint.h>
#endif

PyObject* FastlzError;


static PyObject *
compress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *result;
    const char *input;
    char *output;
    Py_ssize_t level = 0, input_len, output_len;

    static char *arglist[] = {"string", "level", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|i", arglist, &input,
                                     &input_len, &level))
        return NULL;

    if (level == 0) {
        if (input_len < 65536)
            level = 1;
        else
            level = 2;
    } else if (level != 1 && level != 2) {
        PyErr_SetString(PyExc_ValueError, "level must be either 1 or 2");
        return NULL;
    }

    output_len = sizeof(uint32_t) + (uint32_t)((input_len * 1.05) + 2);
    output = (char *) malloc(output_len);
    if (output == NULL)
        return PyErr_NoMemory();
    memcpy(output, &input_len, sizeof(uint32_t));

    output_len = fastlz_compress_level(level, input, input_len,
                                       output + sizeof(uint32_t));
    if (output_len == 0 && input_len != 0) {
        free(output);
        PyErr_SetString(FastlzError, "could not compress");
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    result = Py_BuildValue("y#", output, output_len + sizeof(uint32_t));
#else
    result = Py_BuildValue("s#", output, output_len + sizeof(uint32_t));
#endif
    free(output);
    return result;
}


static PyObject *
decompress(PyObject *self, PyObject *args)
{
    PyObject *result;
    const char *input;
    Py_ssize_t input_len;
    char *output;
    Py_ssize_t output_len, decompressed_len;

    if (!PyArg_ParseTuple(args, "s#", &input, &input_len))
        return NULL;

    if ((uint32_t)input_len < sizeof(uint32_t)) {
        PyErr_SetString(FastlzError, "invalid input");
        return NULL;
    }

    memcpy(&output_len, input, sizeof(uint32_t));

    if (output_len / 256.0 > input_len) {
        PyErr_SetString(FastlzError, "invalid input");
        return NULL;
    }

    output = (char *) malloc(output_len + 1);
    if (output == NULL)
        return PyErr_NoMemory();

    input_len -= sizeof(uint32_t);
    input += sizeof(uint32_t);

    decompressed_len = (uint32_t)fastlz_decompress(input, input_len, output,
                                                   output_len);

    if (decompressed_len != output_len) {
        free(output);
        PyErr_SetString(FastlzError, "could not decompress");
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    result = Py_BuildValue("y#", output, output_len);
#else
    result = Py_BuildValue("s#", output, output_len);
#endif
    free(output);
    return result;
}


static PyMethodDef module_methods[] = {
    {"compress", (PyCFunction)compress, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("Compress a string. Optionally provide a compression level.")},
    {"decompress", (PyCFunction)decompress, METH_VARARGS,
     PyDoc_STR("Decompress a string.")},
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(module_doc,
"Python wrapper for FastLZ, a lightning-fast lossless compression library.");

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "fastlz",
    module_doc,
    -1,
    module_methods
};

PyMODINIT_FUNC
PyInit_fastlz(void)

#else

PyMODINIT_FUNC
initfastlz(void)

#endif

{
    PyObject *m, *d;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&module_def);
#else
    m = Py_InitModule3("fastlz", module_methods, module_doc);
#endif

    PyModule_AddObject(m, "__version__", Py_BuildValue("s", "0.0.2"));
    PyModule_AddObject(m, "__author__", Py_BuildValue("s", "Jared Suttles"));

    d = PyModule_GetDict(m);
    FastlzError = PyErr_NewException("fastlz.FastlzError", NULL, NULL);
    PyDict_SetItemString(d, "FastlzError", FastlzError);

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}
