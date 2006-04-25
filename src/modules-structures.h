#ifndef MODULES_STRUCTURES
#define MODULES_STRUCTURES

#include "mud-connection-view.h"

/*
 * Typedefs
 */
typedef struct _plugin_object PLUGIN_OBJECT;
typedef struct _plugin_info   PLUGIN_INFO;
typedef struct _plugin_data   PLUGIN_DATA;

typedef void      (*plugin_initfunc) (PLUGIN_OBJECT *,   gint   );
typedef void      (*plugin_menufunc) (GtkWidget *,       gint   );
typedef void      (*plugin_datafunc) (PLUGIN_OBJECT *, gchar *, MudConnectionView *);

typedef enum { PLUGIN_DATA_IN, PLUGIN_DATA_OUT } PLUGIN_DATA_DIRECTION;

/*
 * Structures
 */
struct _plugin_data {
  PLUGIN_OBJECT         *plugin;
  plugin_datafunc        datafunc;
  PLUGIN_DATA_DIRECTION  dir;
};

struct _plugin_info {
  gchar            *plugin_name;
  gchar            *plugin_author;
  gchar            *plugin_version;
  gchar            *plugin_descr;
  plugin_initfunc   init_function;
};

struct _plugin_object {
  void     *handle;
  gchar    *name;
  gchar    *filename;
  gboolean  enabled;
  PLUGIN_INFO *info;
};

#endif
