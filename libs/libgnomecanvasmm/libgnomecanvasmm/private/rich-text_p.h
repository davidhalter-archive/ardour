// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _LIBGNOMECANVASMM_RICH_TEXT_P_H
#define _LIBGNOMECANVASMM_RICH_TEXT_P_H


#include <libgnomecanvasmm/private/item_p.h>

#include <glibmm/class.h>

namespace Gnome
{

namespace Canvas
{

class RichText_Class : public Glib::Class
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef RichText CppObjectType;
  typedef GnomeCanvasRichText BaseObjectType;
  typedef GnomeCanvasRichTextClass BaseClassType;
  typedef Item_Class CppClassParent;
  typedef GnomeCanvasItemClass BaseClassParent;

  friend class RichText;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  const Glib::Class& init();

  static void class_init_function(void* g_class, void* class_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.
  static void tag_changed_callback(GnomeCanvasRichText* self, GtkTextTag* p0);

  //Callbacks (virtual functions):
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED
};


} // namespace Canvas

} // namespace Gnome


#endif /* _LIBGNOMECANVASMM_RICH_TEXT_P_H */

