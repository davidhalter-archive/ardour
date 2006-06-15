/* 
   Copyright (C) 2002 Brett Viren & Paul Davis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id$
*/

#ifndef __lib_pbd_undo_h__
#define __lib_pbd_undo_h__

#include <string>
#include <list>
#include <map>
#include <sigc++/slot.h>
#include <sigc++/bind.h>
#include <sys/time.h>
#include <pbd/xml++.h>

using std::string;
using std::list;

typedef sigc::slot<void> UndoAction;

// TODO stick this in its own file, and make the arguments multiply-inherit it
class Serializable 
{
public:
    XMLNode &serialize();
};

class UndoCommand
{
public:
    UndoCommand(id_t object_id, std::string method_name);
    void operator() () { return _slot(); }
    XMLNode &serialize();
protected:
    sigc::slot<void> _slot;
};

template <class T1=void, class T2=void, class T3=void, class T4=void>
class SlotCommand;

template <>
class SlotCommand <> : public UndoCommand {};

template <class T1>
class SlotCommand <T1> : public UndoCommand
{
    T1 _arg1;
public:
    SlotCommand(id_t object_id, std::string key, T1 arg1) 
	: UndoCommand(object_id, key), _arg1(arg1)
    {
	_slot = sigc::bind(_slot, arg1);
    }
};

class UndoTransaction 
{
  public:
	UndoTransaction ();
	UndoTransaction (const UndoTransaction&);
	UndoTransaction& operator= (const UndoTransaction&);

	void clear ();

	void add_undo (const UndoAction&);
	void add_redo (const UndoAction&);
	void add_redo_no_execute (const UndoAction&);

	void undo();
	void redo();
	
	void set_name (const string& str) {
		_name = str;
	}
	const string& name() const { return _name; }

	void set_timestamp (struct timeval &t) {
		_timestamp = t;
	}

	const struct timeval& timestamp() const {
		return _timestamp;
	}

  private:
	list<UndoAction> redo_actions;
	list<UndoAction> undo_actions;
	struct timeval   _timestamp;
	string           _name;
};

class UndoHistory
{
  public:
	UndoHistory() {}
	~UndoHistory() {}
	
	void add (UndoTransaction ut);
	void undo (unsigned int n);
	void redo (unsigned int n);
	
	unsigned long undo_depth() const { return UndoList.size(); }
	unsigned long redo_depth() const { return RedoList.size(); }
	
	string next_undo() const { return (UndoList.empty() ? string("") : UndoList.back().name()); }
	string next_redo() const { return (RedoList.empty() ? string("") : RedoList.back().name()); }

	void clear ();
	void clear_undo ();
	void clear_redo ();

  private:
	list<UndoTransaction> UndoList;
	list<UndoTransaction> RedoList;
};


#endif /* __lib_pbd_undo_h__ */
