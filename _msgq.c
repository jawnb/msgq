/*
 * msgq.c: System V IPC Message Queue Python Extension Module
 *
 * This extension module wraps and makes system calls related to System V
 * message queues available to Python applications.
 *
 * Copyright (c) 2012 Lars Djerf <lars.djerf@gmail.com>
 */
#include <Python.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stddef.h>
#include <string.h>
#include "_msgq.h"

static PyObject *
msgq_ftok(PyObject *self, PyObject *args) {
    const char *pathname;
    int         proj_id;
    key_t       key;

    if (!PyArg_ParseTuple(args, "si", &pathname, &proj_id)) {
        return NULL;
    }

    if ((key = ftok(pathname, proj_id)) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    return Py_BuildValue("i", key);
}

static PyObject *
msgq_msgget(PyObject *self, PyObject *args) {
    key_t key;
    int   msgflg, id;

    if (!PyArg_ParseTuple(args, "ii", &key, &msgflg)) {
        return NULL;
    }

    if ((id = msgget(key, msgflg)) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    return Py_BuildValue("i", id);
}

static PyObject *
msgq_msgsnd(PyObject *self, PyObject *args) {
    int                msqid, msgflg, rv;
    const char        *cstring;
    size_t             msgsz;
    PyObject          *string;
    int                msg_size_var;
    int                msg_data_var;
    struct size_msgbuf size_msg;
    struct msgbuf     *data_msg;

    msg_size_var = DEFAULT_SIZE_MSG;
    msg_data_var = DEFAULT_DATA_MSG;

    if (!PyArg_ParseTuple(args, "iiO|ii", &msqid, &msgflg, &string,
                          &msg_size_var, &msg_data_var)) {
        return NULL;
    }

    cstring         = PyString_AsString(string);

    data_msg        = PyMem_Malloc(offsetof(struct msgbuf, mtext) + strlen(cstring) + 1);
    if (data_msg == NULL) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    data_msg->mtype = msg_data_var;

    if (memcpy(&data_msg->mtext, cstring, strlen(cstring) + 1) == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to copy data into memory.");
        return NULL;
    }

    /* Send message size */
    size_msg.mtype = msg_size_var;
    size_msg.size  = strlen(cstring) + 1;
    msgsz          = sizeof(struct size_msgbuf) - sizeof(long);
    if ((rv = msgsnd(msqid, &size_msg, msgsz, msgflg)) == -1) {
        PyMem_Free(data_msg);
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    msgsz = offsetof(struct msgbuf, mtext) + size_msg.size - sizeof(long);
    /* Send message */
    if ((rv = msgsnd(msqid, data_msg, msgsz, msgflg)) == -1) {
        PyMem_Free(data_msg);
        /* If this fails we need to also remove last size message from the queue */
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    PyMem_Free(data_msg);
    return Py_BuildValue("i", rv);
} /* msgq_msgsnd */

static PyObject *
msgq_msgrcv(PyObject *self, PyObject *args) {
    int                msqid, msgflg;
    int                msg_size_var;
    int                msg_data_var;
    size_t             msgsz;
    PyObject          *data;
    struct size_msgbuf size_msg;
    struct msgbuf     *data_msg;

    msg_size_var = DEFAULT_SIZE_MSG;
    msg_data_var = DEFAULT_DATA_MSG;

    if (!PyArg_ParseTuple(args, "ii|ii",
                          &msqid, &msgflg, &msg_size_var, &msg_data_var)) {
        return NULL;
    }

    /* Get message size */
    msgsz = sizeof(struct size_msgbuf) - sizeof(long);

    if (msgrcv(msqid, &size_msg, msgsz, msg_size_var, IPC_NOWAIT) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    if ((data_msg = PyMem_Malloc(offsetof(struct msgbuf, mtext) + size_msg.size)) == NULL) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    /* Get message */
    if (msgrcv(msqid, data_msg, size_msg.size, msg_data_var, msgflg) == -1) {
        PyMem_Free(data_msg);
        return PyErr_SetFromErrno(PyExc_IOError);
    }

    /* Convert back to Python object */
    data = PyString_FromString(data_msg->mtext);

    PyMem_Free(data_msg);
    return data;
}

static PyObject *
msgq_msgctl(PyObject *self, PyObject *args) {
    int             msqid, cmd, rv;
    struct msqid_ds buf;
    PyObject       *dict_msqid_ds, *dict_ipc_perm, *tmp_obj;

    if (!PyArg_ParseTuple(args, "ii", &msqid, &cmd)) {
        return NULL;
    }
    switch (cmd) {
        case IPC_RMID:
            rv = msgctl(msqid, IPC_RMID, NULL);
            return NULL;
        case IPC_SET:
            PyErr_SetString(PyExc_NotImplementedError, "IPC_SET not supported");
            return NULL;
        case IPC_STAT:
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "Invalid value for command");
            return NULL;
    }
    if ((rv = msgctl(msqid, cmd, &buf)) == -1) {
        return PyErr_SetFromErrno(PyExc_IOError);
    }
    /* Build our permissions dictionary */
    dict_ipc_perm = PyDict_New();
    tmp_obj       = Py_BuildValue("i", buf.msg_perm.__key);
    PyDict_SetItemString(dict_ipc_perm, "__key", tmp_obj);
    Py_DECREF(tmp_obj);


    tmp_obj       = Py_BuildValue("i", buf.msg_perm.uid);
    PyDict_SetItemString(dict_ipc_perm, "uid", tmp_obj);
    Py_DECREF(tmp_obj);


    tmp_obj       = Py_BuildValue("i", buf.msg_perm.gid);
    PyDict_SetItemString(dict_ipc_perm, "gid", tmp_obj);
    Py_DECREF(tmp_obj);

    tmp_obj       = Py_BuildValue("i", buf.msg_perm.cuid);
    PyDict_SetItemString(dict_ipc_perm, "cuid", tmp_obj);
    Py_DECREF(tmp_obj);


    tmp_obj       = Py_BuildValue("i", buf.msg_perm.cgid);
    PyDict_SetItemString(dict_ipc_perm, "cgid", tmp_obj);
    Py_DECREF(tmp_obj);


    tmp_obj       = Py_BuildValue("i", buf.msg_perm.cgid);
    PyDict_SetItemString(dict_ipc_perm, "cgid", tmp_obj);
    Py_DECREF(tmp_obj);


    tmp_obj       = Py_BuildValue("i", buf.msg_perm.mode);
    PyDict_SetItemString(dict_ipc_perm, "mode", tmp_obj);
    Py_DECREF(tmp_obj);

    /* Build our result dict */
    dict_msqid_ds = PyDict_New();

    /* Set our permissions */
    PyDict_SetItemString(dict_msqid_ds, "perms", dict_ipc_perm);
    Py_DECREF(dict_ipc_perm);

    tmp_obj = Py_BuildValue("i", buf.msg_stime);
    PyDict_SetItemString(dict_msqid_ds, "msg_stime",
                         tmp_obj);
    Py_DECREF(tmp_obj);

    tmp_obj = Py_BuildValue("i", buf.msg_ctime);
    PyDict_SetItemString(dict_msqid_ds, "msg_ctime",
                         tmp_obj);
    Py_DECREF(tmp_obj);

    tmp_obj = Py_BuildValue("i", buf.msg_qnum);
    PyDict_SetItemString(dict_msqid_ds, "msg_qnum",
                         tmp_obj);
    Py_DECREF(tmp_obj);

    tmp_obj = Py_BuildValue("i", buf.msg_qbytes);
    PyDict_SetItemString(dict_msqid_ds, "msg_qbytes",
                         tmp_obj);
    Py_DECREF(tmp_obj);

    tmp_obj = Py_BuildValue("i", buf.msg_lspid);
    PyDict_SetItemString(dict_msqid_ds, "msg_lspid",
                         tmp_obj);
    Py_DECREF(tmp_obj);

    tmp_obj = Py_BuildValue("i", buf.msg_lrpid);
    PyDict_SetItemString(dict_msqid_ds, "msg_lrpid",
                         tmp_obj);
    Py_DECREF(tmp_obj);

    return dict_msqid_ds;
} /* msgq_msgctl */

static PyMethodDef msgq_methods[] = {
    { "ftok",   msgq_ftok,   METH_VARARGS,
      "ftok - convert a pathname and a project identifier to a System V IPC key" },
    { "msgget", msgq_msgget, METH_VARARGS,
      "msgget - get a message queue identifier" },
    { "msgsnd", msgq_msgsnd, METH_VARARGS,
      "msgrcv, msgsnd - message operations" },
    { "msgrcv", msgq_msgrcv, METH_VARARGS,
      "msgrcv, msgsnd - message operations" },
    { "msgctl", msgq_msgctl, METH_VARARGS,
      "msgctl - message control operations" },
    { NULL,     NULL,        0,           NULL } /* Sentinel */
};

PyDoc_STRVAR(msgq_doc,
             "This module provides access to System V messages queues (IPC) by\n\
wrapping relevant system calls:\n\
ftok, msgget, msgsnd, msgrcv and msgctl\n\n\
See man-pages for further information."                                                                                                                                                                  );

#if PY_MAJOR_VERSION >= 3
#define MOD_ERROR_VAL NULL
#define MOD_SUCCESS_VAL(val) val
#define MOD_INIT(name)       PyMODINIT_FUNC PyInit_ ## name(void)
#define MOD_DEF(ob, name, doc, methods)               \
    static struct PyModuleDef moduledef = {           \
        PyModuleDef_HEAD_INIT, name, doc, -1, methods \
    };                                                \
                                                      \
    ob = PyModule_Create(&moduledef);
#else
#define MOD_ERROR_VAL
#define MOD_SUCCESS_VAL(val)
#define MOD_INIT(name)       void init ## name(void)
#define MOD_DEF(ob, name, doc, methods) \
    ob = Py_InitModule3(name, methods, doc);
#endif


MOD_INIT(msgqueue) {
    PyObject *m;

    MOD_DEF(m, "msgq", msgq_doc, msgq_methods);

    if (m == NULL) {
        return;
    }

    PyModule_AddIntConstant(m, "IPC_CREAT", IPC_CREAT);
    PyModule_AddIntConstant(m, "IPC_EXCL", IPC_EXCL);
    PyModule_AddIntConstant(m, "IPC_NOWAIT", IPC_NOWAIT);
    PyModule_AddIntConstant(m, "IPC_RMID", IPC_RMID);
    PyModule_AddIntConstant(m, "IPC_SET", IPC_SET);
    PyModule_AddIntConstant(m, "IPC_STAT", IPC_STAT);
    PyModule_AddIntConstant(m, "IPC_INFO", IPC_INFO);
    PyModule_AddIntConstant(m, "IPC_PRIVATE", IPC_PRIVATE);

    return MOD_SUCCESS_VAL(m);
}
