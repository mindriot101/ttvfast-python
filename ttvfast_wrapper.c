#include <Python.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "transit.h"
#include "myintegrator.h"

void TTVFast(double *params,double dt, double Time, double total,int n_plan,CalcTransit *transit,CalcRV *RV_struct, int nRV, int n_events, int input_flag);

static PyObject *ttvfast_ttvfast(PyObject *self, PyObject *args);

static char module_docstring[] = "Fast TTV computation";
static char ttvfast_docstring[] = "Run the TTV fast function. See https://github.com/kdeck/TTVFast";

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyMethodDef ttvfast_methods[] = {
    {"ttvfast", (PyCFunction)ttvfast_ttvfast, METH_VARARGS, ttvfast_docstring},
    {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static int ttvfast_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int ttvfast_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "ttvfast",
        module_docstring,
        sizeof(struct module_state),
        ttvfast_methods,
        NULL,
        ttvfast_traverse,
        ttvfast_clear,
        NULL
};

#define INITERROR return NULL

PyObject *
PyInit_ttvfast(void)

#else
#define INITERROR return

void
initttvfast(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule3("ttvfast", ttvfast_methods, module_docstring);
#endif

    if (module == NULL)
        INITERROR;
    struct module_state *st = GETSTATE(module);

    st->error = PyErr_NewException("ttvfast.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

/* Takes integer and changes the value in place */
static PyObject *ttvfast_ttvfast(PyObject *self, PyObject *args) {
    PyObject *params_obj, *planet_obj, *epoch_obj, *time_obj, *rsky_obj, *vsky_obj;
    double dt, Time, total;
    int n_plan, n_events, input_flag;
    int i;

    n_events = 5000;

    if (!PyArg_ParseTuple(args, "Odddii", &params_obj, &dt, &Time, &total,
                &n_plan, &input_flag)) {
        return NULL;
    }

#ifdef DEBUG
    printf("dt: %lf\n", dt);
    printf("Time: %lf\n", Time);
    printf("total: %lf\n", total);
    printf("n_plan: %d\n", n_plan);
    printf("input_flag: %d\n", input_flag);
#endif

    /* Get the params list */
    double *params = malloc(sizeof(double) * (2 + n_plan * 7));
    PyObject *item;
    for (i=0; i<(2 + n_plan * 7); i++) {
        item = PySequence_GetItem(params_obj, i);
        params[i] = PyFloat_AsDouble(item);
#ifdef DEBUG
        printf("Params %d: %lf\n", i, params[i]);
#endif
    }

    CalcTransit *model;
    model = (CalcTransit*)calloc(n_events, sizeof(CalcTransit));
    double DEFAULT = -2; /* value for transit information that is not determined by TTVFast*/
    for(i=0;i<n_events;i++){
        (model+i)->time = DEFAULT;
    }

#ifdef DEBUG
    printf("Transit model container created\n");
#endif
    TTVFast(params, dt, Time, total, n_plan, model, NULL, 0, n_events, input_flag);
#ifdef DEBUG
    printf("Called\n");
#endif

    planet_obj = PyList_New(n_events);
    epoch_obj = PyList_New(n_events);
    time_obj = PyList_New(n_events);
    rsky_obj = PyList_New(n_events);
    vsky_obj = PyList_New(n_events);

#ifdef DEBUG
    printf("Creating output lists\n");
#endif
    for (i=0; i<n_events; i++) {
        item = PyInt_FromLong((model+i)->planet);
        PyList_SetItem(planet_obj, i, item);

        item = PyInt_FromLong((model+i)->epoch);
        PyList_SetItem(epoch_obj, i, item);

        item = PyFloat_FromDouble((model+i)->time);
        PyList_SetItem(time_obj, i, item);

        item = PyFloat_FromDouble((model+i)->rsky);
        PyList_SetItem(rsky_obj, i, item);

        item = PyFloat_FromDouble((model+i)->vsky);
        PyList_SetItem(vsky_obj, i, item);
    }

    free(model);
    free(params);

    return Py_BuildValue("OOOOO", planet_obj, epoch_obj, time_obj, rsky_obj, vsky_obj);
}
