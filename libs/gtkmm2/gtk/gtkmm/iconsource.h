// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GTKMM_ICONSOURCE_H
#define _GTKMM_ICONSOURCE_H

#include <glibmm.h>

/* $Id$ */

/* iconsource.h
 *
 * Copyright(C) 1998-2002 The gtkmm Development Team
 *
 * This library is free software, ) you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation, ) either
 * version 2 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, ) without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library, ) if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gdkmm/pixbuf.h>
#include <gtkmm/enums.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" { typedef struct _GtkIconSource GtkIconSource; }
#endif

namespace Gtk
{

class IconSource
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef IconSource CppObjectType;
  typedef GtkIconSource BaseObjectType;

  static GType get_type() G_GNUC_CONST;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  IconSource();

  explicit IconSource(GtkIconSource* gobject, bool make_a_copy = true);

  IconSource(const IconSource& other);
  IconSource& operator=(const IconSource& other);

  ~IconSource();

  void swap(IconSource& other);

  ///Provides access to the underlying C instance.
  GtkIconSource*       gobj()       { return gobject_; }

  ///Provides access to the underlying C instance.
  const GtkIconSource* gobj() const { return gobject_; }

  ///Provides access to the underlying C instance. The caller is responsible for freeing it. Use when directly setting fields in structs.
  GtkIconSource* gobj_copy() const;

protected:
  GtkIconSource* gobject_;

private:

  
public:

  
  /** Sets the name of an image file to use as a base image when creating
   * icon variants for Gtk::IconSet. The filename must be absolute.
   * @param filename Image file to use.
   */
  void set_filename(const Glib::ustring& filename);
  
  /** Sets a pixbuf to use as a base image when creating icon variants
   * for Gtk::IconSet.
   * @param pixbuf Pixbuf to use as a source.
   */
  void set_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>&pixbuf);

  
  /** Retrieves the source filename, or <tt>0</tt> if none is set. The
   * filename is not a copy, and should not be modified or expected to
   * persist beyond the lifetime of the icon source.
   * @return Image filename. This string must not be modified
   * or freed.
   */
  Glib::ustring get_filename() const;
  
  /** Retrieves the source pixbuf, or <tt>0</tt> if none is set.
   * In addition, if a filename source is in use, this
   * function in some cases will return the pixbuf from
   * loaded from the filename. This is, for example, true
   * for the GtkIconSource passed to the GtkStyle::render_icon()
   * virtual function. The reference count on the pixbuf is
   * not incremented.
   * @return Source pixbuf.
   */
  Glib::RefPtr<Gdk::Pixbuf> get_pixbuf();
  
  /** Retrieves the source pixbuf, or <tt>0</tt> if none is set.
   * In addition, if a filename source is in use, this
   * function in some cases will return the pixbuf from
   * loaded from the filename. This is, for example, true
   * for the GtkIconSource passed to the GtkStyle::render_icon()
   * virtual function. The reference count on the pixbuf is
   * not incremented.
   * @return Source pixbuf.
   */
  Glib::RefPtr<const Gdk::Pixbuf> get_pixbuf() const;
  
  
  /** If the text direction is wildcarded, this source can be used
   * as the base image for an icon in any Gtk::TextDirection.
   * If the text direction is not wildcarded, then the
   * text direction the icon source applies to should be set
   * with gtk_icon_source_set_direction(), and the icon source
   * will only be used with that text direction.
   * 
   * Gtk::IconSet prefers non-wildcarded sources (exact matches) over
   * wildcarded sources, and will use an exact match when possible.
   * @param setting <tt>true</tt> to wildcard the text direction.
   */
  void set_direction_wildcarded(bool setting = true);
  
  /** If the widget state is wildcarded, this source can be used as the
   * base image for an icon in any Gtk::StateType.  If the widget state
   * is not wildcarded, then the state the source applies to should be
   * set with gtk_icon_source_set_state() and the icon source will
   * only be used with that specific state.
   * 
   * Gtk::IconSet prefers non-wildcarded sources (exact matches) over
   * wildcarded sources, and will use an exact match when possible.
   * 
   * Gtk::IconSet will normally transform wildcarded source images to
   * produce an appropriate icon for a given state, for example
   * lightening an image on prelight, but will not modify source images
   * that match exactly.
   * @param setting <tt>true</tt> to wildcard the widget state.
   */
  void set_state_wildcarded(bool setting = true);
  
  /** If the icon size is wildcarded, this source can be used as the base
   * image for an icon of any size.  If the size is not wildcarded, then
   * the size the source applies to should be set with
   * gtk_icon_source_set_size() and the icon source will only be used
   * with that specific size.
   * 
   * Gtk::IconSet prefers non-wildcarded sources (exact matches) over
   * wildcarded sources, and will use an exact match when possible.
   * 
   * Gtk::IconSet will normally scale wildcarded source images to produce
   * an appropriate icon at a given size, but will not change the size
   * of source images that match exactly.
   * @param setting <tt>true</tt> to wildcard the widget state.
   */
  void set_size_wildcarded (bool setting = true);
  
  /** Gets the value set by gtk_icon_source_set_size_wildcarded().
   * @return <tt>true</tt> if this icon source is a base for any icon size variant.
   */
  bool get_size_wildcarded() const;
  
  /** Gets the value set by gtk_icon_source_set_state_wildcarded().
   * @return <tt>true</tt> if this icon source is a base for any widget state variant.
   */
  bool get_state_wildcarded() const;
  
  /** Gets the value set by gtk_icon_source_set_direction_wildcarded().
   * @return <tt>true</tt> if this icon source is a base for any text direction variant.
   */
  bool get_direction_wildcarded() const;
  
  /** Sets the text direction this icon source is intended to be used
   * with.
   * 
   * Setting the text direction on an icon source makes no difference
   * if the text direction is wildcarded. Therefore, you should usually
   * call gtk_icon_source_set_direction_wildcarded() to un-wildcard it
   * in addition to calling this function.
   * @param direction Text direction this source applies to.
   */
  void set_direction(TextDirection direction);
  
  /** Sets the widget state this icon source is intended to be used
   * with.
   * 
   * Setting the widget state on an icon source makes no difference
   * if the state is wildcarded. Therefore, you should usually
   * call gtk_icon_source_set_state_wildcarded() to un-wildcard it
   * in addition to calling this function.
   * @param state Widget state this source applies to.
   */
  void set_state(Gtk::StateType state);
  
  /** Sets the icon size this icon source is intended to be used
   * with.
   * 
   * Setting the icon size on an icon source makes no difference
   * if the size is wildcarded. Therefore, you should usually
   * call gtk_icon_source_set_size_wildcarded() to un-wildcard it
   * in addition to calling this function.
   * @param size Icon size this source applies to.
   */
  void set_size(IconSize size);
  
  /** Obtains the text direction this icon source applies to. The return
   * value is only useful/meaningful if the text direction is <em>not</em> 
   * wildcarded.
   * @return Text direction this source matches.
   */
  TextDirection get_direction() const;
  
  /** Obtains the widget state this icon source applies to. The return
   * value is only useful/meaningful if the widget state is <em>not</em>
   * wildcarded.
   * @return Widget state this source matches.
   */
  Gtk::StateType get_state() const;
  
  /** Obtains the icon size this source applies to. The return value
   * is only useful/meaningful if the icon size is <em>not</em> wildcarded.
   * @return Icon size this source matches.
   */
  IconSize get_size() const;

  
  /** Sets the name of an icon to look up in the current icon theme
   * to use as a base image when creating icon variants for Gtk::IconSet.
   * @param icon_name Name of icon to use.
   */
  void set_icon_name(const Glib::ustring& icon_name);
  
  /** Retrieves the source icon name, or <tt>0</tt> if none is set. The
   * icon_name is not a copy, and should not be modified or expected to
   * persist beyond the lifetime of the icon source.
   * @return Icon name. This string must not be modified or freed.
   */
  Glib::ustring get_icon_name() const;


};

} /* namespace Gtk */


namespace Gtk
{

/** @relates Gtk::IconSource
 * @param lhs The left-hand side
 * @param rhs The right-hand side
 */
inline void swap(IconSource& lhs, IconSource& rhs)
  { lhs.swap(rhs); }

} // namespace Gtk

namespace Glib
{

/** @relates Gtk::IconSource
 * @param object The C instance
 * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
 * @result A C++ instance that wraps this C instance.
 */
Gtk::IconSource wrap(GtkIconSource* object, bool take_copy = false);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
template <>
class Value<Gtk::IconSource> : public Glib::Value_Boxed<Gtk::IconSource>
{};
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

} // namespace Glib

#endif /* _GTKMM_ICONSOURCE_H */

