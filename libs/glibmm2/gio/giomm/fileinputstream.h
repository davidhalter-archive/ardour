// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GIOMM_FILEINPUTSTREAM_H
#define _GIOMM_FILEINPUTSTREAM_H


#include <glibmm.h>

// -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*-

/* Copyright (C) 2007 The gtkmm Development Team
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

#include <giomm/fileinfo.h>
#include <giomm/inputstream.h>
#include <giomm/seekable.h>
#include <glibmm/iochannel.h>
#include <glibmm/object.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GFileInputStream GFileInputStream;
typedef struct _GFileInputStreamClass GFileInputStreamClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gio
{ class FileInputStream_Class; } // namespace Gio
namespace Gio
{

/** FileInputStream provides input streams that take their content from a file.
 *
 * FileInputStream implements Seekable, which allows the input stream to jump to arbitrary positions in the file, 
 * provided the file system of the file allows it.
 * Use the methods of the Seekable base class for seeking and positioning.
 *
 * @ingroup Streams
 *
 * @newin2p16
 */

class FileInputStream 
: public Gio::InputStream, 
  public Seekable
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef FileInputStream CppObjectType;
  typedef FileInputStream_Class CppClassType;
  typedef GFileInputStream BaseObjectType;
  typedef GFileInputStreamClass BaseClassType;

private:  friend class FileInputStream_Class;
  static CppClassType fileinputstream_class_;

private:
  // noncopyable
  FileInputStream(const FileInputStream&);
  FileInputStream& operator=(const FileInputStream&);

protected:
  explicit FileInputStream(const Glib::ConstructParams& construct_params);
  explicit FileInputStream(GFileInputStream* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~FileInputStream();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;
  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GFileInputStream*       gobj()       { return reinterpret_cast<GFileInputStream*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GFileInputStream* gobj() const { return reinterpret_cast<GFileInputStream*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GFileInputStream* gobj_copy();

private:

  
public:

  /** Queries a file input stream the given @a attributes. This function blocks 
   * while querying the stream. For the asynchronous (non-blocking) version 
   * of this function, see query_info_async(). While the 
   * stream is blocked, the stream will set the pending flag internally, and 
   * any other operations on the stream will throw a Gio::Error with PENDING.
   *
   * @param attributes A file attribute query string.
   * @param cancellable A Cancellable object.
   * @return A FileInfo, or an empty RefPtr on error.
   */
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  Glib::RefPtr<FileInfo> query_info(const Glib::RefPtr<Cancellable>& cancellable, const std::string& attributes = "*");
#else
  Glib::RefPtr<FileInfo> query_info(const Glib::RefPtr<Cancellable>& cancellable, const std::string& attributes, std::auto_ptr<Glib::Error>& error);
#endif //GLIBMM_EXCEPTIONS_ENABLED

  /** Queries a file input stream the given @a attributes. This function blocks 
   * while querying the stream. For the asynchronous (non-blocking) version 
   * of this function, see query_info_async(). While the 
   * stream is blocked, the stream will set the pending flag internally, and 
   * any other operations on the stream will throw a Gio::Error with PENDING.
   *
   * @param attributes A file attribute query string.
   * @return A FileInfo, or an empty RefPtr on error.
   */
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  Glib::RefPtr<FileInfo> query_info(const std::string& attributes = "*");
#else
  Glib::RefPtr<FileInfo> query_info(const std::string& attributes, std::auto_ptr<Glib::Error>& error);
#endif //GLIBMM_EXCEPTIONS_ENABLED
  

  /** Queries the stream information asynchronously. For the synchronous version of this function, see query_info().
   *
   * The operation can be cancelled by triggering the cancellable object from another thread. If the operation was cancelled, 
   * a Gio::Error with CANCELLED will be thrown.
   *
   * When the operation is finished, @a slot will be called. You can then call query_info_finish() to get the result of the operation.
   *
   * @param slot A callback slot which will be called when the request is satisfied.
   * @param cancellable A Cancellable object which can be used to cancel the operation.
   * @param attributes A file attribute query string.
   * @param io_priority The I/O priority of the request.
   */
  void query_info_async(const SlotAsyncReady& slot, const Glib::RefPtr<Cancellable>& cancellable, const std::string& attributes = "*", int io_priority = Glib::PRIORITY_DEFAULT);

  /** Queries the stream information asynchronously. For the synchronous version of this function, see query_info().
   *
   * When the operation is finished, @a slot will be called. You can then call query_info_finish() to get the result of the operation.
   *
   * @param slot A callback slot which will be called when the request is satisfied.
   * @param attributes A file attribute query string.
   * @param io_priority The I/O priority of the request.
   */
  void query_info_async(const SlotAsyncReady& slot, const std::string& attributes = "*", int io_priority = Glib::PRIORITY_DEFAULT);

  
  /** Finishes an asynchronous info query operation.
   * @param result A AsyncResult.
   * @return FileInfo.
   */
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  Glib::RefPtr<FileInfo> query_info_finish(const Glib::RefPtr<AsyncResult>& result);
#else
  Glib::RefPtr<FileInfo> query_info_finish(const Glib::RefPtr<AsyncResult>& result, std::auto_ptr<Glib::Error>& error);
#endif //GLIBMM_EXCEPTIONS_ENABLED


  //These seem to be just C convenience functions - they are already in the Seekable base class:
  //See http://bugzilla.gnome.org/show_bug.cgi?id=509990
  
//  _WRAP_METHOD(goffset tell() const, g_file_input_stream_tell)
//  _WRAP_METHOD(bool can_seek() const, g_file_input_stream_can_seek)
//  _WRAP_METHOD(bool seek(goffset offset, Glib::SeekType type, const Glib::RefPtr<Cancellable>& cancellable),
//               g_file_input_stream_seek,
//               errthrow)


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

protected:
  //GTK+ Virtual Functions (override these to change behaviour):
#ifdef GLIBMM_VFUNCS_ENABLED
#endif //GLIBMM_VFUNCS_ENABLED

  //Default Signal Handlers::
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED


};

} // namespace Gio


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gio::FileInputStream
   */
  Glib::RefPtr<Gio::FileInputStream> wrap(GFileInputStream* object, bool take_copy = false);
}


#endif /* _GIOMM_FILEINPUTSTREAM_H */

