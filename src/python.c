/* GNOME-Mud - A simple Mud CLient
 * Copyright (C) 1998-2002 Robin Ericsson <lobbin@localhost.nu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"

#ifdef USE_PYTHON
#include <sys/types.h>
#include <locale.h>
#include <gnome.h>
#include <pwd.h>
#include <signal.h>
#include <dirent.h>

#include <Python.h>
#include <structmember.h>

#ifdef USE_PYGTK
#include <pygtk/pygtk.h>
#include <gtk/gtk.h>
#endif

#include "gnome-mud.h"

static char const rcsid[] =
    "$Id$";

/* Local functions */

/* Global Variables */
GList *pymud_input_callbacks = NULL;
GList *pymud_output_callbacks = NULL;
static PyObject *ConnectionError;
extern CONNECTION_DATA *main_connection;
extern CONNECTION_DATA *connections[MAX_CONNECTIONS];
extern GtkWidget *main_notebook;

#ifdef USE_PYGTK
extern GtkWidget *box_user;
#endif

/* Interface to the CONNECTION_DATA structure */
staticforward PyTypeObject pyConnection_ConnectionType;

typedef struct
{
    PyObject_HEAD
    CONNECTION_DATA *connection;
} pyConnection_ConnectionObject;

/* Connection objects are never constructed inside Python code,
 * this constructor is to be called from C only.
 */
static pyConnection_ConnectionObject* pyConnection_new(CONNECTION_DATA *data)
{
	pyConnection_ConnectionObject* conn;

	conn = PyObject_NEW(pyConnection_ConnectionObject, &pyConnection_ConnectionType);
	conn->connection = data;

	return conn;
}

static void pyConnection_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyObject *pyConnection_repr(pyConnection_ConnectionObject *self)
{
	gchar *foo = g_strdup_printf("<GnomeMud.Connection host:%s port:%s profile:%s>",
								 self->connection->host, self->connection->port,
								 self->connection->profile->name);
	PyObject *rv = Py_BuildValue("s",foo);
	g_free(foo);
	
	return rv;
}

static PyObject *pyConnection_send(pyConnection_ConnectionObject *self, PyObject *args)
{
	gchar *data;
	int echo = 1;

	if (!PyArg_ParseTuple(args, "s|i", &data, &echo))
		return NULL;

    if (!self->connection->connected)
	{
		PyErr_Format(ConnectionError, "not connected to a socket");
		return NULL;
	}
	connection_send_data(self->connection, data, echo, FALSE);

	Py_INCREF(Py_None);
    
	return Py_None;
}

static PyObject *pyConnection_print(pyConnection_ConnectionObject *self, PyObject *args)
{
	gchar *data;

	if (!PyArg_ParseTuple(args, "s", &data))
		return NULL;

	textfield_add (self->connection, data, MESSAGE_NORMAL);

	Py_INCREF(Py_None);
    
	return Py_None;
}

static PyMethodDef pyConnection_methods[] =
{
    { "send",  (PyCFunction) pyConnection_send,  METH_VARARGS },
    { "write", (PyCFunction) pyConnection_print, METH_VARARGS },
    {NULL, NULL}
};

static struct memberlist pyConnection_memberlist[] = 
{
    { "connected", T_INT,    offsetof(struct connection_data,connected), RO },
    { "host",      T_STRING, offsetof(struct connection_data,host),      RO },
    { "port",      T_STRING, offsetof(struct connection_data,port),      RO },
    {NULL}
};

static PyObject *pyConnection_getattr(pyConnection_ConnectionObject *self, char *name)
{
	PyObject *res;

	res = Py_FindMethod(pyConnection_methods, (PyObject *)self, name);
    
	if (res != NULL)
		return res;
	
    PyErr_Clear();
    
	if (!strcmp(name,"profile"))
		return Py_BuildValue("s",self->connection->profile->name);
	
	return PyMember_Get((char *)self->connection, pyConnection_memberlist, name);
}

static int pyConnection_setattr(pyConnection_ConnectionObject *c, char *name, PyObject *v)
{
	if (v == NULL)
	{
		PyErr_SetString(PyExc_AttributeError, "can't delete connection attributes");
		return -1;
    }

    return PyMember_Set((char *)c->connection, pyConnection_memberlist, name, v);
}

static PyTypeObject pyConnection_ConnectionType = 
{
    PyObject_HEAD_INIT(NULL)
    0,
    "Connection",
    sizeof(pyConnection_ConnectionObject),
    0,
    pyConnection_dealloc,              /* tp_dealloc     */
    0,                                 /* tp_print       */
    (getattrfunc)pyConnection_getattr, /* tp_getattr     */
    (setattrfunc)pyConnection_setattr, /* tp_setattr     */
    0,                                 /* tp_compare     */
    (reprfunc) pyConnection_repr,      /* tp_repr        */
    0,                                 /* tp_as_number   */
    0,                                 /* tp_as_sequence */
    0,                                 /* tp_as_mapping  */
    0,                                 /* tp_hash        */
};

static void init_pyConnection(void) 
{
	PyObject *m, *d;
	pyConnection_ConnectionType.ob_type = &PyType_Type;

	m = Py_InitModule("Connection", pyConnection_methods);
	d = PyModule_GetDict(m);
	ConnectionError = PyErr_NewException("GnomeMud.Connection.ConnectionError", NULL, NULL);
	PyDict_SetItemString(d, "ConnectionError", ConnectionError);
}



/* Global functions */
static PyObject *pymud_register_input_handler (PyObject *self, PyObject *args)
{
	PyObject *callback;

	if (!PyArg_ParseTuple(args, "O", &callback))
		return NULL;

    if (!PyCallable_Check(callback))
	{
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF(callback);
	pymud_input_callbacks = g_list_append(pymud_input_callbacks, (gpointer)callback);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *pymud_register_output_handler (PyObject *self, PyObject *args)
{
	PyObject *callback;

	if (!PyArg_ParseTuple(args, "O", &callback))
		return NULL;

	if (!PyCallable_Check(callback))
	{
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return NULL;
	}

	Py_XINCREF(callback);
	pymud_output_callbacks = g_list_append(pymud_output_callbacks, (gpointer)callback);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *pymud_connection (PyObject *self, PyObject *args)
{
	gint number;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	number = gtk_notebook_get_current_page (GTK_NOTEBOOK (main_notebook));

	return (PyObject *)pyConnection_new(connections[number]);
}

static PyObject *pymud_version (PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	return Py_BuildValue("s",VERSION);
}

#ifdef USE_PYGTK
static PyObject *pymud_pygtk_add_widget (PyObject *self, PyObject *args)
{
	PyGObject *widget;
	GtkWidget *gwidget;
	int expand = TRUE, fill = TRUE, padding = 5;

	if (!PyArg_ParseTuple(args, "O|iii", &widget, &expand, &fill, &padding))
		return NULL;

	Py_INCREF(widget);

	gwidget = GTK_WIDGET(widget->obj);
	gtk_box_pack_start (GTK_BOX (box_user), gwidget, (gboolean)expand, (gboolean)fill, (guint)padding);
	gtk_widget_show(gwidget);

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

/* System functions */
gchar *python_process_input(CONNECTION_DATA *connection, gchar *input)
{
	GList *i = pymud_input_callbacks;
	PyObject *res, *arg;
	pyConnection_ConnectionObject *conn;
	gchar *flarp = input;

	conn = pyConnection_new(connection);
	
	if (!conn) return flarp;

	while (i)
	{
		arg = Py_BuildValue("(Os)",conn,(char *)flarp);
	
		if (!arg)
		{
			Py_DECREF(conn);
			return flarp;
		}
		
		res = PyEval_CallObject((PyObject *)i->data, arg);
		Py_DECREF(arg);
	
		if (res)
		{
			if (PyString_Check(res))
			{
				flarp = g_strdup(PyString_AsString(res));
				g_free(input);
	    	}
	    
			Py_DECREF(res);
		}
		else
		{
			PyErr_Print();
		}
		
		i = g_list_next(i);
    }
	
    Py_DECREF(conn);
    
	return flarp;
}

gchar *python_process_output(CONNECTION_DATA *connection, gchar *output)
{
	GList *i = pymud_output_callbacks;
	PyObject *res, *arg;
	pyConnection_ConnectionObject *conn;
	gchar *flarp = g_strdup(output);

	conn = pyConnection_new(connection);
	if (!conn) return flarp;

	while (i)
	{
		arg = Py_BuildValue("(Os)",conn,(char *)flarp);
	
		if (!arg)
		{
			Py_DECREF(conn);
			return flarp;
		}
		
		res = PyEval_CallObject((PyObject *)i->data, arg);
		Py_DECREF(arg);
		
		if (res)
		{
			if (PyString_Check(res))
			{
				g_free(flarp);
				flarp = g_strdup(PyString_AsString(res));
			}
	    
			Py_DECREF(res);
		}
		else
		{
			PyErr_Print();
		}

		i = g_list_next(i);
	}
	
	Py_DECREF(conn);
	return flarp;
}

/* Python method definitions */
static PyMethodDef GnomeMudMethods[] =
{
    {"connection",              pymud_connection,              METH_VARARGS},
    {"version",                 pymud_version,                 METH_VARARGS},
    {"register_input_handler",  pymud_register_input_handler,  METH_VARARGS},
    {"register_output_handler", pymud_register_output_handler, METH_VARARGS},
#ifdef USE_PYGTK
    {"add_user_widget",         pymud_pygtk_add_widget,        METH_VARARGS},
#endif
    {NULL, NULL}
};

void python_init()
{
	DIR *dir;
	FILE *fh;
	gchar *foo, *bar;
	struct dirent *flarp;

	(void) Py_InitModule("GnomeMud", GnomeMudMethods);
	init_pyConnection();

	foo = g_strdup_printf("%s/.gnome-mud/plugins/", g_get_home_dir());
	dir = opendir(foo);
	if (!dir) { g_free(foo); return; }

	while ((flarp = readdir(dir)))
	{
		if (strlen(flarp->d_name) < 4)
			continue;
		
		if (strcmp(".py",flarp->d_name + strlen(flarp->d_name) - 3))
			continue;
		
		bar = g_strdup_printf("%s%s",foo,flarp->d_name);
		fh = fopen(bar, "r");
		if (fh)
		{
			PyRun_AnyFile(fh, bar);
			fclose(fh);
		}

		g_free(bar);
	}

	closedir(dir);
	g_free(foo);
}

void python_end()
{
	GList *i = pymud_input_callbacks;
	
	while (i)
	{
		Py_XDECREF((PyObject *)i->data);
		i = g_list_next(i);
	}
	
	g_list_free(pymud_input_callbacks);
}

#endif /* USE_PYTHON */

