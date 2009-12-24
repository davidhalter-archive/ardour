/*
    Copyright (C) 2009 Paul Davis 

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

*/

#ifndef __pbd_event_loop_h__
#define __pbd_event_loop_h__

#include <boost/function.hpp>
#include <boost/bind.hpp> /* we don't need this here, but anything calling call_slot() probably will, so this is convenient */
#include <glibmm/thread.h>

namespace PBD
{

class EventLoop 
{
  public:
	EventLoop() {}
	virtual ~EventLoop() {}

	virtual void call_slot (const boost::function<void()>&) = 0;

	static EventLoop* get_event_loop_for_thread();
	static void set_event_loop_for_thread (EventLoop* ui);

  private:
	static Glib::StaticPrivate<EventLoop> thread_event_loop;

};

}

#endif /* __pbd_event_loop_h__ */