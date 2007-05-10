// Generated by gtkmmproc -- DO NOT MODIFY!

#include <atkmm/editabletext.h>
#include <atkmm/private/editabletext_p.h>

// -*- c++ -*-
/* $Id$ */

/* Copyright 1998-2002 The gtkmm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <atkmm/object.h>
#include <atk/atkeditabletext.h>


namespace
{
} // anonymous namespace


namespace Glib
{

Glib::RefPtr<Atk::EditableText> wrap(AtkEditableText* object, bool take_copy)
{
  return Glib::RefPtr<Atk::EditableText>( dynamic_cast<Atk::EditableText*> (Glib::wrap_auto ((GObject*)(object), take_copy)) );
  //We use dynamic_cast<> in case of multiple inheritance.
}

} // namespace Glib


namespace Atk
{


/* The *_Class implementation: */

const Glib::Interface_Class& EditableText_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Interface_Class has to know the interface init function
    // in order to add interfaces to implementing types.
    class_init_func_ = &EditableText_Class::iface_init_function;

    // We can not derive from another interface, and it is not necessary anyway.
    gtype_ = atk_editable_text_get_type();
  }

  return *this;
}

void EditableText_Class::iface_init_function(void* g_iface, void*)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_iface);

  //This is just to avoid an "unused variable" warning when there are no vfuncs or signal handlers to connect.
  //This is a temporary fix until I find out why I can not seem to derive a GtkFileChooser interface. murrayc
  g_assert(klass != 0); 

  klass->set_run_attributes = &set_run_attributes_vfunc_callback;
  klass->set_text_contents = &set_text_contents_vfunc_callback;
  klass->insert_text = &insert_text_vfunc_callback;
  klass->copy_text = &copy_text_vfunc_callback;
  klass->cut_text = &cut_text_vfunc_callback;
  klass->delete_text = &delete_text_vfunc_callback;
  klass->paste_text = &paste_text_vfunc_callback;
}

gboolean EditableText_Class::set_run_attributes_vfunc_callback(AtkEditableText* self, AtkAttributeSet* attrib_set, gint start_offset, gint end_offset)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      return static_cast<int>(obj->set_run_attributes_vfunc(attrib_set, start_offset
, end_offset
));
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->set_run_attributes)
      return (*base->set_run_attributes)(self, attrib_set, start_offset, end_offset);
  }

  typedef gboolean RType;
  return RType();
}

void EditableText_Class::set_text_contents_vfunc_callback(AtkEditableText* self, const gchar* string)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      obj->set_text_contents_vfunc(Glib::convert_const_gchar_ptr_to_ustring(string)
);
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->set_text_contents)
      (*base->set_text_contents)(self, string);
  }
}

void EditableText_Class::insert_text_vfunc_callback(AtkEditableText* self, const gchar* string, gint length, gint* position)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      obj->insert_text_vfunc(Glib::convert_const_gchar_ptr_to_ustring(string)
, length
, *(position)
);
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->insert_text)
      (*base->insert_text)(self, string, length, position);
  }
}

void EditableText_Class::copy_text_vfunc_callback(AtkEditableText* self, gint start_pos, gint end_pos)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      obj->copy_text_vfunc(start_pos
, end_pos
);
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->copy_text)
      (*base->copy_text)(self, start_pos, end_pos);
  }
}

void EditableText_Class::cut_text_vfunc_callback(AtkEditableText* self, gint start_pos, gint end_pos)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      obj->cut_text_vfunc(start_pos
, end_pos
);
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->cut_text)
      (*base->cut_text)(self, start_pos, end_pos);
  }
}

void EditableText_Class::delete_text_vfunc_callback(AtkEditableText* self, gint start_pos, gint end_pos)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      obj->delete_text_vfunc(start_pos
, end_pos
);
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->delete_text)
      (*base->delete_text)(self, start_pos, end_pos);
  }
}

void EditableText_Class::paste_text_vfunc_callback(AtkEditableText* self, gint position)
{
  CppObjectType *const obj = dynamic_cast<CppObjectType*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj && obj->is_derived_())
  {
    try // Trap C++ exceptions which would normally be lost because this is a C callback.
    {
      // Call the virtual member method, which derived classes might override.
      obj->paste_text_vfunc(position
);
    }
    catch(...)
    {
      Glib::exception_handlers_invoke();
    }
  }
  else
  {
    BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(self), CppObjectType::get_type()) // Get the interface.
)    );

    // Call the original underlying C function:
    if(base && base->paste_text)
      (*base->paste_text)(self, position);
  }
}


Glib::ObjectBase* EditableText_Class::wrap_new(GObject* object)
{
  return new EditableText((AtkEditableText*)(object));
}


/* The implementation: */

EditableText::EditableText()
:
  Glib::Interface(editabletext_class_.init())
{}

EditableText::EditableText(AtkEditableText* castitem)
:
  Glib::Interface((GObject*)(castitem))
{}

EditableText::~EditableText()
{}

// static
void EditableText::add_interface(GType gtype_implementer)
{
  editabletext_class_.init().add_interface(gtype_implementer);
}

EditableText::CppClassType EditableText::editabletext_class_; // initialize static member

GType EditableText::get_type()
{
  return editabletext_class_.init().get_type();
}

GType EditableText::get_base_type()
{
  return atk_editable_text_get_type();
}


bool EditableText::set_run_attributes(const AttributeSet& attrib_set, int start_offset, int end_offset)
{
  return atk_editable_text_set_run_attributes(gobj(), (attrib_set).data(), start_offset, end_offset);
}

void EditableText::set_text_contents(const Glib::ustring& string)
{
  atk_editable_text_set_text_contents(gobj(), string.c_str());
}

void EditableText::insert_text(const Glib::ustring& string, int length, int& position)
{
  atk_editable_text_insert_text(gobj(), string.c_str(), length, &position);
}

void EditableText::copy_text(int start_pos, int end_pos)
{
  atk_editable_text_copy_text(gobj(), start_pos, end_pos);
}

void EditableText::cut_text(int start_pos, int end_pos)
{
  atk_editable_text_cut_text(gobj(), start_pos, end_pos);
}

void EditableText::delete_text(int start_pos, int end_pos)
{
  atk_editable_text_delete_text(gobj(), start_pos, end_pos);
}

void EditableText::paste_text(int position)
{
  atk_editable_text_paste_text(gobj(), position);
}


bool Atk::EditableText::set_run_attributes_vfunc(AtkAttributeSet* attrib_set, int start_offset, int end_offset) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->set_run_attributes)
    return (*base->set_run_attributes)(gobj(),attrib_set,start_offset,end_offset);

  typedef bool RType;
  return RType();
}

void Atk::EditableText::set_text_contents_vfunc(const Glib::ustring& string) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->set_text_contents)
    (*base->set_text_contents)(gobj(),string.c_str());
}

void Atk::EditableText::insert_text_vfunc(const Glib::ustring& string, int length, int& position) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->insert_text)
    (*base->insert_text)(gobj(),string.c_str(),length,&position);
}

void Atk::EditableText::copy_text_vfunc(int start_pos, int end_pos) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->copy_text)
    (*base->copy_text)(gobj(),start_pos,end_pos);
}

void Atk::EditableText::cut_text_vfunc(int start_pos, int end_pos) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->cut_text)
    (*base->cut_text)(gobj(),start_pos,end_pos);
}

void Atk::EditableText::delete_text_vfunc(int start_pos, int end_pos) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->delete_text)
    (*base->delete_text)(gobj(),start_pos,end_pos);
}

void Atk::EditableText::paste_text_vfunc(int position) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_interface_peek_parent( // Get the parent interface of the interface (The original underlying C interface).
g_type_interface_peek(G_OBJECT_GET_CLASS(gobject_), CppObjectType::get_type()) // Get the interface.
)  );

  if(base && base->paste_text)
    (*base->paste_text)(gobj(),position);
}


} // namespace Atk


