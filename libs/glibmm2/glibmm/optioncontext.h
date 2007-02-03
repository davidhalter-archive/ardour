// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _GLIBMM_OPTIONCONTEXT_H
#define _GLIBMM_OPTIONCONTEXT_H


/* $Id$ */

/* Copyright (C) 2004 The glibmm Development Team
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


#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/error.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" { typedef struct _GOptionContext GOptionContext; }
#endif


namespace Glib
{

/** Exception class for options.
 */
class OptionError : public Glib::Error
{
public:
  enum Code
  {
    UNKNOWN_OPTION,
    BAD_VALUE,
    FAILED
  };

  OptionError(Code error_code, const Glib::ustring& error_message);
  explicit OptionError(GError* gobject);
  Code code() const;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
private:

#ifdef GLIBMM_EXCEPTIONS_ENABLED
  static void throw_func(GError* gobject);
#else
  //When not using exceptions, we just pass the Exception object around without throwing it:
  static std::auto_ptr<Glib::Error> throw_func(GError* gobject);
#endif //GLIBMM_EXCEPTIONS_ENABLED

  friend void wrap_init(); // uses throw_func()
#endif
};


/** An OptionContext defines which options are accepted by the commandline option parser.
 */
class OptionContext
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef OptionContext CppObjectType;
  typedef GOptionContext BaseObjectType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

private:

public:

  /** Creates a new option context.
   * @param parameter_string A string which is displayed in the first line of --help output, after programname [OPTION...]
   */
  explicit OptionContext(const Glib::ustring& parameter_string = Glib::ustring());
  
  //Note that, unlike Glib::Wrap(), this would create a second C++ instance for the same C instance,
  //so it should be used carefully. For instance you could not access data in a derived class via this second instance.
  explicit OptionContext(GOptionContext* castitem, bool take_ownership = false);
  virtual ~OptionContext();
  
  
  /** Enables or disables automatic generation of &lt;option&gt;--help&lt;/option&gt; 
   * output. By default, g_option_context_parse() recognizes
   * &lt;option&gt;--help&lt;/option&gt;, &lt;option&gt;-?&lt;/option&gt;, &lt;option&gt;--help-all&lt;/option&gt;
   * and &lt;option&gt;--help-&lt;/option&gt;&lt;replaceable&gt;groupname&lt;/replaceable&gt; and creates
   * suitable output to stdout. 
   * 
   * @newin2p6
   * @param help_enabled <tt>true</tt> to enable &lt;option&gt;--help&lt;/option&gt;, <tt>false</tt> to disable it.
   */
  void set_help_enabled(bool help_enabled = true);
  
  /** Returns whether automatic &lt;option&gt;--help&lt;/option&gt; generation
   * is turned on for @a context . See g_option_context_set_help_enabled().
   * @return <tt>true</tt> if automatic help generation is turned on.
   * 
   * @newin2p6.
   */
  bool get_help_enabled() const;
  
  /** Sets whether to ignore unknown options or not. If an argument is 
   * ignored, it is left in the @a argv  array after parsing. By default, 
   * g_option_context_parse() treats unknown options as error.
   * 
   * This setting does not affect non-option arguments (i.e. arguments 
   * which don't start with a dash). But note that GOption cannot reliably
   * determine whether a non-option belongs to a preceding unknown option.
   * 
   * @newin2p6
   * @param ignore_unknown <tt>true</tt> to ignore unknown options, <tt>false</tt> to produce
   * an error when unknown options are met.
   */
  void set_ignore_unknown_options(bool ignore_unknown = true);
  
  /** Returns whether unknown options are ignored or not. See
   * g_option_context_set_ignore_unknown_options().
   * @return <tt>true</tt> if unknown options are ignored.
   * 
   * @newin2p6.
   */
  bool get_ignore_unknown_options() const;

 
  /** Parses the command line arguments, recognizing options
   * which have been added to @a context . A side-effect of 
   * calling this function is that g_set_prgname() will be
   * called.
   * 
   * If the parsing is successful, any parsed arguments are
   * removed from the array and @a argc  and @a argv  are updated 
   * accordingly. A '--' option is stripped from @a argv 
   * unless there are unparsed options before and after it, 
   * or some of the options after it start with '-'. In case 
   * of an error, @a argc  and @a argv  are left unmodified. 
   * 
   * If automatic &lt;option&gt;--help&lt;/option&gt; support is enabled
   * (see g_option_context_set_help_enabled()), and the 
   *  @a argv  array contains one of the recognized help options,
   * this function will produce help output to stdout and
   * call <tt>exit (0)</tt>.
   * @param argc A pointer to the number of command line arguments.
   * @param argv A pointer to the array of command line arguments.
   * @param error A return location for errors.
   * @return <tt>true</tt> if the parsing was successful, 
   * <tt>false</tt> if an error occurred
   * 
   * @newin2p6.
   */
#ifdef GLIBMM_EXCEPTIONS_ENABLED
  bool parse(int& argc, char**& argv);
#else
  bool parse(int& argc, char**& argv, std::auto_ptr<Glib::Error>& error);
#endif //GLIBMM_EXCEPTIONS_ENABLED


  //g_option_context_add_main_entries(), just creates a group internally, adds them to it, and does a set_main_group()
  //- a group without callbacks seems to do some simple default parsing.
  
  
  /** Adds an OptionGroup to the context, so that parsing with context will recognize the options in the group. 
   * Note that the group will not be copied, so it should exist for as long as the context exists.
   *
   * @param group The group to add.
   */
  void add_group(OptionGroup& group);
  
  
  /** Sets an OptionGroup as the main group of the context. This has the same effect as calling add_group(), the only 
   * difference is that the options in the main group are treated differently when generating --help output.
   * Note that the group will not be copied, so it should exist for as long as the context exists.
   *
   * @param group The group to add.
   */
  void set_main_group(OptionGroup& group);
  
  
  //We don't need this (hopefully), and the memory management would be really awkward.
  //OptionGroup& get_main_group();
  //const OptionGroup& get_main_group() const;
  

  GOptionContext*       gobj()       { return gobject_; }
  const GOptionContext* gobj() const { return gobject_; }

protected:

  GOptionContext* gobject_;
  bool has_ownership_;


};


} // namespace Glib


#endif /* _GLIBMM_OPTIONCONTEXT_H */

