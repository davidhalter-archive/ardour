// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_TREEDRAGSOURCE_P_H
#define _GTKMM_TREEDRAGSOURCE_P_H
#include <glibmm/private/interface_p.h>

#include <glibmm/private/interface_p.h>

namespace Gtk
{

class TreeDragSource_Class : public Glib::Interface_Class
{
public:
  typedef TreeDragSource CppObjectType;
  typedef GtkTreeDragSource BaseObjectType;
  typedef GtkTreeDragSourceIface BaseClassType;
  typedef Glib::Interface_Class CppClassParent;

  friend class TreeDragSource;

  const Glib::Interface_Class& init();

  static void iface_init_function(void* g_iface, void* iface_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.

  //Callbacks (virtual functions):
    static gboolean drag_data_get_vfunc_callback(GtkTreeDragSource* self, GtkTreePath* path, GtkSelectionData* selection_data);
    static gboolean row_draggable_vfunc_callback(GtkTreeDragSource* self, GtkTreePath* path);
  static gboolean drag_data_delete_vfunc_callback(GtkTreeDragSource* self, GtkTreePath* path);
};


} // namespace Gtk

#endif /* _GTKMM_TREEDRAGSOURCE_P_H */

